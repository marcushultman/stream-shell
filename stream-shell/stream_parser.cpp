#include "stream_parser.h"

#include <charconv>
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
#include "util/to_string_view.h"

namespace {

#if 0

inline auto lookupType(std::string_view name) {
  auto *pool = google::protobuf::DescriptorPool::generated_pool();
  return pool ? pool->FindMessageTypeByName(name) : nullptr;
}

#endif

auto errorStream(Error err) -> PrintableStream {
  return {ranges::views::single(std::unexpected(err)), Print::Pull{.full = true}};
}

struct ToStream {
  ToStream(const Env &env, const Closure &closure) : _env{env}, _closure{closure} {}

  auto operator()(google::protobuf::Value val) -> Stream {
    if (val.has_list_value()) {
      auto size = val.list_value().values().size();
      return ranges::views::iota(0, size) |
             ranges::views::transform(
                 [list = std::move(*val.mutable_list_value())](auto i) { return list.values(i); });
    }
    return ranges::views::single(std::move(val));
  }
  auto operator()(Value val) -> Stream { return ranges::views::single(std::move(val)); }
  auto operator()(Stream stream) { return stream; }
  auto operator()(const StreamRef &ref) -> Stream {
    auto stream = _env.getEnv(ref);
    return stream ? stream() : Stream();
  }
  auto operator()(const Word &word) -> Stream {
    google::protobuf::Value value;
    value.set_string_value(word.value);
    return ranges::views::single(std::move(value));
  }
  auto operator()(const ClosureValue &value) -> Stream {
    if (auto result = value(_closure)) {
      return std::visit(*this, std::move(*result));
    } else {
      return ranges::views::single(std::unexpected(result.error()));
    }
  }

  const Env &_env;
  const Closure &_closure;
};

enum class InputMode {
  kStream,
  kValue,
};

struct CommandBuilder {
  InputMode input_mode = InputMode::kStream;
  Stream input;
  Closure closure;
  int record_level = 0;
  std::vector<Operand> operands;

  Print::Mode print_mode;

  StreamFactory factory(Env &env) && {
    return [&env,
            input_mode = input_mode,
            input = std::move(input),
            closure = std::move(closure),
            operands = std::move(operands)] {
      auto build_stream = [&](auto &&input) -> Stream {
        return std::move(input) |
               ranges::views::for_each([&env, closure, operands](const auto &) -> Stream {
                 if (operands.empty()) {
                   return {};
                 }

                 if (auto cmd = frontCommand(closure, operands)) {
                   if (auto stream = findBuiltin(*cmd)) {
                     return *stream;
                   }

                   //  DEMO print out the words
                   for (auto [i, operand] : ranges::views::enumerate(operands)) {
                     auto *word = std::get_if<Word>(&operand);
                     std::cerr << (i ? " " : "") << (word ? word->value : "?");
                   }
                   std::cerr << "\n";

                   return {};
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

  // void run() {
  //   if (!words.empty()) {
  //     // todo: external command
  //     int des_p[2];
  //     if (pipe(des_p) == -1) {
  //       return {};
  //     }

  //     if (auto pid = fork(); pid == 0) {
  //       close(STDOUT_FILENO);  // closing stdout
  //       dup(des_p[1]);         // replacing stdout with pipe write
  //       close(des_p[0]);       // closing pipe read
  //       close(des_p[1]);

  //       auto args = command | ranges::views::transform([](auto &str) { return str.data(); }) |
  //                   ranges::to<std::vector>;

  //       execvp(command[0].c_str(), args.data());
  //       perror("execvp");
  //       exit(1);

  //     } else if (pid > 0) {
  //       int status;
  //       waitpid(pid, &status, 0);
  //     } else {
  //       perror("fork");
  //     }
  //     return ranges::views::empty<Operand>;
  //   }
  // }
};

//

constexpr auto toNumber = ranges::views::transform([](auto i) {
  google::protobuf::Value value;
  value.set_number_value(i);
  return value;
});

// todo: implement as operator on Value
std::optional<Stream> parseNumericRange(const Closure &closure, std::string_view token) {
  auto delim = token.find(std::string_view(".."));
  if (delim == std::string_view::npos) {
    return {};
  }
  auto parse_number = [&](auto s) -> std::optional<ranges::any_view<std::optional<int64_t>>> {
    if (int64_t i; std::from_chars(s.begin(), s.end(), i, 10).ec == std::errc{}) {
      return ranges::views::single(i);
    } else if (auto it = closure.vars.find(Word{s}); it != closure.vars.end()) {
      return ranges::views::single(ranges::ref(*it->second)) |
             ranges::views::transform([](const Value &val) -> std::optional<int64_t> {
               if (auto *number_val = std::get_if<google::protobuf::Value>(&val);
                   number_val->has_number_value()) {
                 return number_val->number_value();
               }
               return {};
             });
    }
    return {};
  };

  auto from_str = token.substr(0, delim);
  auto to_str = token.substr(delim + 2);

  auto from = parse_number(from_str);
  auto to = parse_number(to_str).value_or(ranges::views::single(std::nullopt));

  if (!from) {
    return {};
  }
  using Pair = std::pair<int64_t, std::optional<int64_t>>;
  return ranges::views::zip(*from, to) | ranges::views::transform([](auto from_to) -> Pair {
           auto [from, to] = from_to;
           if (!from) return {};
           return std::pair{*from, to};
         }) |
         ranges::views::for_each([](auto pair) -> Stream {
           auto [from, to] = pair;
           return to ? Stream(ranges::views::iota(from, *to + 1) | toNumber)
                     : Stream(ranges::views::iota(from) | toNumber);
         });
}

//

auto unaryOp(bool unary, std::string_view op) {
  if ((op == "+" || op == "-") && unary) return 10;
  if (op == "!") return 10;
  return 0;
}

auto binaryOp(std::string_view op) {
  if (op == "?" || op == "?:") return 4;
  // if (op == "??") return ??;
  if (op == "||" || op == "&&") return 5;
  if (op == "==" || op == "!=") return 6;
  if (op == "<" || op == "<=" || op == ">" || op == ">=") return 7;
  if (op == "+" || op == "-") return 8;
  if (op == "*" || op == "/" || op == "%") return 9;
  return 0;
}

auto rightAssociative(std::string_view op) {
  if (op == "?") return true;
  return false;
}

auto precedence(const CommandBuilder &lhs, std::string_view op) {
  if (auto p = unaryOp(lhs.operands.empty(), op)) return p;
  if (auto p = binaryOp(op)) return p;
  if (op == "=" || op == ":" || op == "->") return 1;
  if (op == ";") return 2;
  if (op == "|") return 3;
  return 0;
}

auto isOperator(const CommandBuilder &lhs, std::string_view op) {
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
  auto operator()(Word word) -> std::string { return std::string(word.value); }
  auto operator()(ClosureValue value) -> std::string {
    if (auto result = value(_closure)) {
      return std::visit(*this, *result);
    } else {
      error = result.error();
      return {};
    }
  }

  auto from(const auto &operands) -> Result<google::protobuf::Value> {
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

auto toJSON(Env &env, std::vector<Operand> &&operands) -> Operand {
  return [&env, operands = std::move(operands)](const Closure &closure) -> Result<Value> {
    if (auto result = ToJSON(env, closure).from(operands)) {
      return *result;
    } else {
      return std::unexpected(result.error());
    }
  };
}

auto lookupField(auto &value, auto &path) -> Value {
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
  return value;
}

//

struct StreamParserImpl final : StreamParser {
  StreamParserImpl(Env &env) : env{env} {}

  auto parse(std::vector<std::string_view> &&) -> PrintableStream override;

 private:
  auto toOperand(std::string_view token) -> Operand;
  auto isClosure() -> bool;
  auto operatorCanBeApplied(std::string_view op) -> bool;

  auto performOp(auto &&pred) -> Result<void>;

  Env &env;
  std::stack<CommandBuilder> cmds;
  std::stack<std::string_view> ops;
};

auto setClosureVar(Closure &closure, const Word &var) {
  return ranges::views::transform([var = closure.add(var)](auto &&result) {
    return std::move(result).transform([&](auto &&value) { return (*var = value); });
  });
}

auto StreamParserImpl::parse(std::vector<std::string_view> &&tokens) -> PrintableStream {
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
              token = "?:";
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
      cmds.top().operands.push_back(std::move(rhs).build(env));

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

      cmds.top() = std::move(rhs);

    } else if (token == "{") {
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.operands.push_back(token);
      rhs.record_level = lhs.record_level + 1;
      rhs.closure = lhs.closure;

    } else if (token == "}") {
      if (auto res = performOp([&](auto &op) { return op != "{"; }); !res.has_value()) {
        return errorStream(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      rhs.operands.push_back(token);
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

auto StreamParserImpl::toOperand(std::string_view token) -> Operand {
  auto &closure = cmds.top().closure;

  if (token.starts_with('$')) {
    return StreamRef(token.substr(1));
  }
  if (auto stream = parseNumericRange(closure, token)) {
    return *stream;
  }
  if (token.starts_with('`')) {
    return [&env = env, token](const Closure &closure) {
      auto val = google::protobuf::Value();
      val.set_string_value(token.substr(1, token.size() - 2) | ranges::views::split(' ') |
                           ranges::views::transform([&](auto &&s) { return toStringView(s); }) |
                           ranges::views::transform([&env, &closure](auto &&token) -> std::string {
                             if (token.starts_with('$')) {
                               auto ref = StreamRef(token.substr(1));
                               if (auto it = closure.vars.find(ref.name);
                                   it != closure.vars.end()) {
                                 return std::visit(ToJSON(env, closure), *it->second);
                               }
                               auto stream = env.getEnv(ref);
                               return stream ? ToJSON(env, closure)(stream()) : std::string();
                             }
                             return std::string(token);
                           }) |
                           ranges::views::join(' ') | ranges::to<std::string>());
      return val;
    };
  }
  if (auto val = google::protobuf::Value(); token.starts_with("'")) {
    val.set_string_value(token.substr(1, token.size() - 2));
    return val;

  } else if (google::protobuf::util::JsonStringToMessage(token, &val).ok()) {
    return val;
  }

  auto path = token | ranges::views::split('.') | ranges::to<std::vector<std::string>>();

  if (auto it = closure.vars.find(path[0]);
      it != closure.vars.end() && (ops.top() != "{" || cmds.top().record_level)) {
    return [value = it->second, path = std::move(path)](const Closure &closure) -> Result<Value> {
      return lookupField(*value, path);
    };
  }
  return Word(token);
}

auto StreamParserImpl::isClosure() -> bool {
  return !(cmds.top().record_level || ops.empty() || ops.top() != "|" ||
           !cmds.top().operands.empty());
}

auto StreamParserImpl::operatorCanBeApplied(std::string_view op) -> bool {
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

    if (unaryOp(lhs.operands.empty(), ops.top())) {
      if (rhs.operands.empty()) {
        return std::unexpected(Error::kMissingOperand);
      }
      rhs.operands[0] = std::visit(OperandOp(ops.top()), std::move(rhs.operands[0]));
      lhs.operands.append_range(rhs.operands);
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

    } else if (ops.top() == "=") {
      if (lhs.operands.size() != 1) {
        return std::unexpected(Error::kMissingOperand);
      }

      auto ref = std::get_if<StreamRef>(&lhs.operands[0]);
      if (!ref) {
        return std::unexpected(Error::kInvalidStreamRef);
      }
      auto stream = std::move(rhs).factory(env);
      env.setEnv(*ref, stream);

      lhs.operands[0] = stream();
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
