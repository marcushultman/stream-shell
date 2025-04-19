#include "stream_parser.h"

#include <expected>
#include <functional>
#include <stack>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "builtin.h"
#include "closure.h"
#include "operand.h"
#include "operand_op.h"
#include "to_stream.h"
#include "to_string.h"
#include "util/trim.h"

namespace {

using namespace std::string_view_literals;

#if 0

inline auto lookupType(std::string_view name) {
  auto *pool = google::protobuf::DescriptorPool::generated_pool();
  return pool ? pool->FindMessageTypeByName(name) : nullptr;
}

#endif

inline std::string readAll(int fd) {
  std::ostringstream all;
  std::vector<char> buffer(4096);
  for (ssize_t n; (n = read(fd, buffer.data(), buffer.size())) > 0;) {
    all.write(buffer.data(), n);
  }
  return std::move(all).str();
}

inline auto exec(std::ranges::range auto args) {
  auto args_vec = args | ranges::to<std::vector<std::string>>;
  auto argv =
      ranges::views::concat(args_vec | ranges::views::transform([](auto &s) { return s.data(); }),
                            ranges::views::single(nullptr)) |
      ranges::to<std::vector>;
  return execvp(args_vec[0].c_str(), argv.data());
}

auto errorStream(Error err) -> PrintableStream {
  return {ranges::views::single(std::unexpected(err)), Print::Pull{.full = true}};
}

enum class InputMode {
  kStream,
  kValue,
};

template <typename T>
struct ValueVisitor {
  Result<T> operator()(const google::protobuf::BytesValue &value) { return value; }
  Result<T> operator()(const google::protobuf::Value &value) { return value; }
  Result<T> operator()(const google::protobuf::Any &value) { return value; }
  Result<T> operator()(const auto &) { return std::unexpected(Error::kParseError); }
};

struct CommandBuilder {
  InputMode input_mode = InputMode::kStream;
  Stream input;
  Closure closure;
  std::vector<Operand> operands;

  int record_level = 0;
  bool freeze_operands = false;
  Print::Mode print_mode;

  StreamFactory factory(Env &env) && {
    return [&env,
            input_mode = input_mode,
            input = std::move(input),
            closure = std::move(closure),
            operands = std::move(operands)] {
      auto build_stream = [&](auto &&input) -> Stream {
        return std::move(input) |
               ranges::views::for_each([&env, closure, operands](auto &&input) -> Stream {
                 if (operands.empty()) {
                   return {};
                 }

                 if (auto cmd = frontCommand(closure, operands)) {
                   if (auto stream = runBuiltin(env,
                                                closure,
                                                std::forward<decltype(input)>(input),
                                                *cmd,
                                                operands | ranges::views::drop(1))) {
                     return *stream;
                   }
                   return runChildProcess(env, closure, operands);
                 }

                 // Stream expression (ignoring input)
                 return operands | ranges::views::for_each([&env, closure](const Operand &value) {
                          return std::visit(ToStream(env, closure), value);
                        });
               });
      };
      return input_mode == InputMode::kValue ? build_stream(Stream(input))
                                             : build_stream(ranges::views::single(input));
    };
  }

  Stream build(Env &env) && { return std::move(*this).factory(env)(); }
  Operand operand(Env &env) && {
    if (operands.size() == 1) {
      if (auto operand = std::visit(ValueVisitor<Operand>(), operands[0])) {
        return *operand;
      }
    }
    return std::move(*this).build(env);
  }
  PrintableStream print(Env &env) && {
    auto mode = std::move(print_mode);
    return {std::move(*this).build(env), std::move(mode)};
  }

 private:
  static const Word *frontCommand(const Closure &closure, const std::vector<Operand> &operands) {
    if (auto *word = std::get_if<Word>(&operands[0])) {
      return closure.vars.contains(*word) ? nullptr : word;
    }
    return nullptr;
  }

  static Stream runChildProcess(Env &env, const Closure &closure, ranges::range auto &&operands) {
#if !__EMSCRIPTEN__
    std::array<int, 2> out_pipe, err_pipe;

    if (pipe(out_pipe.data()) == -1 || pipe(err_pipe.data()) == -1) {
      return ranges::views::single(std::unexpected(Error::kExecPipeError));
    }

    auto args = operands | ranges::views::transform([&](auto &operand) {
                  return std::visit(ToString(env, closure), operand);
                });

    if (auto err = ranges::find_if(args, std::logical_not<>()); err != args.end()) {
      return ranges::views::single(std::unexpected((*err).error()));
    }

    if (auto pid = fork(); pid == 0) {
      close(out_pipe[0]);
      close(err_pipe[0]);
      dup2(out_pipe[1], STDOUT_FILENO);
      dup2(err_pipe[1], STDERR_FILENO);

      exec(args | ranges::views::transform([](auto &&r) { return std::move(*r); }));
      exit(1);

    } else if (pid > 0) {
      close(out_pipe[1]);
      close(err_pipe[1]);

      return ranges::views::generate_n(
          [=] {
            auto buffer = readAll(out_pipe[0]);
            close(out_pipe[0]);
            close(err_pipe[0]);

            int status = 0;
            waitpid(pid, &status, 0);

            google::protobuf::Value value;
            value.set_string_value("Child process exited with status " + std::to_string(status) +
                                   "\n" + buffer);
            return value;
          },
          1);
    }
    return ranges::views::single(std::unexpected(Error::kExecForkError));
#else
    return ranges::views::single(std::unexpected(Error::kExecError));
#endif
  }
};

//

auto unaryLeftOp(bool unary, std::ranges::range auto op) {
  if ((op == "+" || op == "-") && unary) return 10;
  if (op == "!") return 10;
  return 0;
}

auto unaryRightOp(bool unary, std::ranges::range auto op) {
  if ((op == "&") && unary) return 1;
  if ((op == "..") && unary) return 7;
  return 0;
}

auto binaryOp(std::ranges::range auto op) {
  if (op == "?" || op == "?:") return 4;
  // if (op == "??") return ??;
  if (op == "||" || op == "&&") return 5;
  if (op == "==" || op == "!=") return 6;
  if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "..") return 7;
  if (op == "+" || op == "-") return 8;
  if (op == "*" || op == "/" || op == "%") return 9;
  return 0;
}

auto rightAssociative(std::ranges::range auto op) {
  if (op == "?") return true;
  return false;
}

auto precedence(const CommandBuilder &lhs, std::ranges::range auto op) {
  if (auto p = unaryLeftOp(lhs.operands.empty(), op)) return p;
  if (auto p = unaryRightOp(true, op)) return p;
  if (auto p = binaryOp(op)) return p;
  if (op == ":" || op == "->") return 1;
  if (op == ";") return 2;
  if (op == "=" || op == "|") return 3;
  return 0;
}

auto isOperator(const CommandBuilder &lhs, std::ranges::range auto op) {
  return precedence(lhs, op) > 0;
}

struct ToJSON {
  ToJSON(Env &env, const Closure &closure) : _env{env}, _closure{closure} {}

  auto operator()(IsValue auto value) -> std::string {
    std::string str;
    (void)google::protobuf::json::MessageToJsonString(value, &str);
    return str;
  }
  auto operator()(Stream stream) -> std::string {
    return (*this)(
        ranges::fold_left(stream, google::protobuf::Value(), [&](auto &&list, auto &&result) {
          if (!result) {
            error = result.error();
            return list;
          }
          auto &value = *list.mutable_list_value()->add_values();
          if (auto *bytes = std::get_if<google::protobuf::BytesValue>(&*result)) {
            // todo: base64 encode
            value.set_string_value(bytes->Utf8DebugString());
          } else if (auto *pvalue = std::get_if<google::protobuf::Value>(&*result)) {
            value = std::move(*pvalue);
          } else if (auto *any = std::get_if<google::protobuf::Any>(&*result)) {
            // todo: something
            value.set_string_value(any->Utf8DebugString());
          }
          return list;
        }));
  }
  auto operator()(StreamRef ref) -> std::string {
    if (auto stream = _env.getEnv(ref)) {
      return (*this)(stream());
    }
    error = Error::kInvalidStreamRef;
    return {};
  }
  auto operator()(Word word) -> std::string { return word.value | ranges::to<std::string>; }
  auto operator()(ClosureValue value) -> std::string {
    if (auto result = value(_closure)) {
      return std::visit(*this, *result);
    } else {
      error = result.error();
      return {};
    }
  }

  auto from(const auto &operands) -> ClosureValue::result_type {
    auto str = operands | ranges::views::for_each([&](Operand operand) {
                 return std::visit(*this, std::move(operand));
               }) |
               ranges::to<std::string>();
    if (error != Error::kSuccess) {
      return std::unexpected(error);
    }
    auto value = google::protobuf::Value();
    auto *json = value.mutable_struct_value();
    if (google::protobuf::json::JsonStringToMessage(str, json).ok()) {
      return value;
    }
    return std::unexpected(Error::kJsonError);
  }

  Env &_env;
  const Closure &_closure;
  Error error = Error::kSuccess;
};

auto toJSON(Env &env, std::vector<Operand> &&operands) {
  return [&env, operands = std::move(operands)](const Closure &closure) {
    return ToJSON(env, closure).from(operands);
  };
}

auto lookupField(auto &value, auto &path) -> ClosureValue::result_type {
  if (path.size() > 1) {
    if (auto *json = std::get_if<google::protobuf::Value>(&value)) {
      return ranges::fold_left(
          path | ranges::views::drop(1), *json, [](const auto &json, auto &field) {
            if (json.has_struct_value()) {
              auto &fields = json.struct_value().fields();
              if (auto it = fields.find(field); it != fields.end()) {
                return it->second;
              }
            }
            return json;
          });
    }
  }
  return std::visit([](auto &value) -> ClosureValue::result_type { return value; }, value);
}

//

struct StreamParserImpl final : StreamParser {
  StreamParserImpl(Env &env) : env{env} {}

  auto parse(ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>>)
      -> PrintableStream override;

 private:
  auto toOperand(ranges::bidirectional_range auto token) -> Operand;
  auto isClosure() -> bool;
  auto operatorCanBeApplied(std::ranges::range auto op) -> bool;

  auto performOp(auto &&pred) -> Result<void>;

  Env &env;
  std::stack<CommandBuilder> cmds;
  std::stack<Token> ops;
};

auto setClosureVar(Closure &closure, const Word &var) {
  return ranges::views::transform([var = closure.add(var)](auto &&result) {
    return std::move(result).transform([&](auto &&value) { return (*var = value); });
  });
}

auto StreamParserImpl::parse(
    ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>> tokens)
    -> PrintableStream {
  // reset state
  cmds = {};
  ops = {};
  cmds.emplace();

  for (auto token : tokens) {
    if (isOperator(cmds.top(), token) && operatorCanBeApplied(token)) {
      if (auto res = performOp([&, ternary = false](auto &op) mutable {
            if (ternary) {
              return false;
            } else if (op == "?" && token == ":") {
              token = "?:"sv;
              return (ternary = true);
            }
            auto a = precedence(cmds.top(), op);
            auto b = precedence(cmds.top(), token);
            return rightAssociative(token) ? (a > b) : (a >= b);
          });
          !res.has_value()) {
        return errorStream(res.error());
      }
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.closure = lhs.closure;
      rhs.record_level = lhs.record_level;

      if (token == "->") {
        if (lhs.operands.size() != 1) {
          return errorStream(Error::kInvalidClosureSignature);
        }
        auto *closure_var = std::get_if<Word>(&lhs.operands[0]);
        if (!closure_var) {
          return errorStream(Error::kMissingVariable);
        }
        rhs.input_mode = lhs.input_mode;
        rhs.input = lhs.input | setClosureVar(rhs.closure, *closure_var);
      }

    } else if (cmds.top().freeze_operands) {
      return errorStream(Error::kMissingOperator);

    } else if (token == "(") {
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.closure = lhs.closure;

    } else if (token == ")") {
      if (auto res = performOp([](auto &op) { return op != "("; }); !res.has_value()) {
        return errorStream(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      // todo: build Value so that it can be used in ternary condition
      cmds.top().operands.push_back(std::move(rhs).operand(env));

    } else if (token == "{" && isClosure()) {
      // Produce the input to closure
      if (auto res = performOp([&](auto &op) { return op != "{" && op != "("; });
          !res.has_value()) {
        return errorStream(res.error());
      }
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.input_mode = InputMode::kValue;
      rhs.input = std::move(lhs.input);
      rhs.closure = lhs.closure;

    } else if (token == "}" && cmds.top().record_level == 0) {
      if (auto res = performOp([&](auto &op) { return op != "{"; }); !res.has_value()) {
        return errorStream(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      rhs.freeze_operands = true;
      cmds.top() = std::move(rhs);

    } else if (token == "{") {
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.operands.push_back(Word{token});
      rhs.closure = lhs.closure;
      rhs.record_level = lhs.record_level + 1;

    } else if (token == "}") {
      if (auto res = performOp([&](auto &op) { return op != "{"; }); !res.has_value()) {
        return errorStream(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      rhs.operands.push_back(Word{token});
      cmds.top().operands.push_back(toJSON(env, std::move(rhs.operands)));

    } else {
      cmds.top().operands.push_back(toOperand(token));
    }
  }

  if (auto res = performOp([&](auto &) { return true; }); !res.has_value()) {
    return errorStream(res.error());
  }
  if (cmds.size() != 1) {
    return errorStream(Error::kParseError);
  }
  return std::move(cmds.top()).print(env);
}

auto StreamParserImpl::toOperand(ranges::bidirectional_range auto token) -> Operand {
  auto &closure = cmds.top().closure;

  if (ranges::starts_with(token, "$"sv)) {
    return StreamRef(token | ranges::views::drop(1));
  }
  if (ranges::starts_with(token, "`"sv)) {
    return [&env = env, token = token](const Closure &closure) {
      auto val = google::protobuf::Value();
      val.set_string_value(trim(token, 1, 1) | ranges::views::split(' ') |
                           ranges::views::transform([&env, &closure](auto &&token) -> std::string {
                             if (ranges::starts_with(token, "$"sv)) {
                               auto ref = StreamRef(token | ranges::views::drop(1));
                               if (auto it = closure.env_overrides.find(ref.name);
                                   it != closure.env_overrides.end()) {
                                 return ToJSON(env, closure)(it->second());
                               }
                               if (auto it = closure.vars.find(ref.name);
                                   it != closure.vars.end()) {
                                 return std::visit(ToJSON(env, closure), *it->second);
                               }
                               auto stream = env.getEnv(ref);
                               return stream ? ToJSON(env, closure)(stream()) : std::string();
                             }
                             return token | ranges::to<std::string>;
                           }) |
                           ranges::views::join(' ') | ranges::to<std::string>());
      return val;
    };
  }
  if (auto val = google::protobuf::Value(); ranges::starts_with(token, "'"sv)) {
    val.set_string_value(trim(token, 1, 1) | ranges::to<std::string>);
    return val;

  } else if (google::protobuf::util::JsonStringToMessage(token | ranges::to<std::string>, &val)
                 .ok()) {
    return val;
  }

  auto path = token | ranges::views::split('.') | ranges::to<std::vector<std::string>>();

  if (auto it = closure.vars.find(Word{path[0]});
      it != closure.vars.end() && (ops.top() != "{" || cmds.top().record_level)) {
    return [value = it->second, path = std::move(path)](const Closure &closure) {
      return lookupField(*value, path);
    };
  }
  return Word{token};
}

auto StreamParserImpl::isClosure() -> bool {
  return !(cmds.top().record_level || ops.empty() || ops.top() != "|" ||
           !cmds.top().operands.empty());
}

auto StreamParserImpl::operatorCanBeApplied(std::ranges::range auto op) -> bool {
  if (cmds.top().record_level) {
    return binaryOp(op);
  }
  return true;
}

auto StreamParserImpl::performOp(auto &&pred) -> Result<void> {
  while (!ops.empty() && pred(ops.top())) {
    if (cmds.size() < 2) {
      return std::unexpected(Error::kMissingOperand);
    }
    auto rhs = std::move(cmds.top());
    cmds.pop();
    auto lhs = std::move(cmds.top());
    cmds.pop();

    if (unaryLeftOp(lhs.operands.empty(), ops.top())) {
      if (rhs.operands.empty()) {
        return std::unexpected(Error::kMissingOperand);
      }
      rhs.operands[0] = std::visit(OperandOp(ops.top()), std::move(rhs.operands[0]));
      lhs.operands.append_range(rhs.operands);
      cmds.push(std::move(lhs));

    } else if (unaryRightOp(rhs.operands.empty(), ops.top())) {
      if (lhs.operands.empty()) {
        return std::unexpected(Error::kMissingOperand);
      }
      lhs.operands.back() = std::visit(OperandOp(ops.top()), std::move(lhs.operands.back()));
      cmds.push(std::move(lhs));

    } else if (binaryOp(ops.top())) {
      if (lhs.operands.empty() || rhs.operands.empty()) {
        return std::unexpected(Error::kMissingOperand);
      }
      rhs.operands[0] = std::visit(
          OperandOp(ops.top()), std::move(lhs.operands.back()), std::move(rhs.operands[0]));
      lhs.operands.pop_back();
      lhs.operands.append_range(rhs.operands);
      cmds.push(std::move(lhs));

    } else if (ops.top() == ":") {
      lhs.print_mode = [&] -> Print::Mode {
        if (!rhs.operands.empty())
          if (auto arg = std::get_if<google::protobuf::Value>(&rhs.operands[0]))
            if (arg->has_number_value())
              return Print::Slice{static_cast<size_t>(arg->number_value())};
        return Print::Pull{.full = true};
      }();
      cmds.push(std::move(lhs));

    } else if (ops.top() == "->") {
      cmds.push(std::move(rhs));

    } else if (ops.top() == ";") {
      ranges::for_each(std::move(lhs).build(env), [](auto &&) {});
      cmds.push(std::move(rhs));

    } else if (ops.top() == "=") {
      if (lhs.operands.size() != 1) {
        return std::unexpected(Error::kMissingOperand);
      }

      if (auto *ref = std::get_if<StreamRef>(&lhs.operands[0])) {
        env.setEnv(*ref, std::move(rhs).factory(env));
        lhs.operands.clear();

      } else if (auto *var = std::get_if<Word>(&lhs.operands[0])) {
        auto operands = rhs.operands;
        if (!operands.empty()) {
          operands.erase(operands.begin());
          rhs.operands.resize(1);
        }
        lhs.closure.env_overrides[*var] = std::move(rhs).factory(env);
        lhs.operands = operands;

      } else {
        return std::unexpected(Error::kInvalidStreamRef);
      }
      cmds.push(std::move(lhs));

    } else if (ops.top() == "|") {
      rhs.input = std::move(lhs).build(env);
      cmds.push(std::move(rhs));

    } else {
      return std::unexpected(Error::kParseError);
    }
    ops.pop();
  }
  return {};
}

}  // namespace

std::unique_ptr<StreamParser> makeStreamParser(Env &env) {
  return std::make_unique<StreamParserImpl>(env);
}
