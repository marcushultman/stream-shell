#include "stream_parser.h"

#include <charconv>
#include <expected>
#include <format>
#include <functional>
#include <stack>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
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

struct JsonList {
  JsonList &&operator+(google::protobuf::Value &&v) && {
    *_list.add_values() = std::move(v);
    return std::move(*this);
  }
  friend auto &operator<<(std::ostream &os, JsonList &&list) {
    (void)google::protobuf::util::MessageToJsonString(list._list, &list._json);
    return os << list._json;
  }

 private:
  google::protobuf::ListValue _list;
  std::string _json;
};

#endif

const Word *checkCommand(const Closure &closure, const std::vector<Operand> &operands) {
  if (auto *word = std::get_if<Word>(operands.empty() ? nullptr : &operands.front())) {
    return closure.vars.contains(*word) ? nullptr : word;
  }
  return nullptr;
}

enum class InputMode {
  kStream,
  kValue,
};

struct CommandBuilder {
  InputMode input_mode = InputMode::kStream;
  Stream input;
  Closure closure;
  std::vector<Operand> operands;
  Print::Mode print_mode;

  StreamFactory factory(Env &env) && {
    struct ToStream {
      ToStream(Env &env, const Closure &closure) : _env{env}, _closure{closure} {}

      auto varOp(const Word &var) -> Value {
        if (auto it = _closure.vars.find(var); it != _closure.vars.end()) {
          return *it->second;
        }
        return StreamError(std::format("'{}' is not a variable", var.value));
      }

      auto operator()(Stream stream) { return stream; }
      auto operator()(Value val) -> Stream { return ranges::views::single(std::move(val)); }
      auto operator()(StreamRef ref) -> Stream {
        return _env.getEnv(ref).transform([](auto &&f) { return f(); }).value_or(Stream());
      }
      auto operator()(Word word) -> Stream { return ranges::views::single(varOp(word)); }

      Env &_env;
      const Closure &_closure;
    };

    return [&env,
            input_mode = input_mode,
            input = std::move(input),
            closure = std::move(closure),
            operands = std::move(operands)] {
      auto build_stream = [&](auto &&input) -> Stream {
        return std::move(input) |
               ranges::views::for_each(
                   [closure, operands](const auto &input) -> ranges::any_view<Operand> {
                     // assume StreamExpression, ignoring input

                     if (auto cmd = checkCommand(closure, operands)) {
                       if (auto builtin = findBuiltin(cmd->value)) {
                         return ranges::views::single(*builtin);
                       }

                       for (auto [i, o] : ranges::views::enumerate(operands)) {
                         auto *w = std::get_if<Word>(&o);
                         std::cerr << (i ? " " : "") << (w ? w->value : "?");
                       }
                       std::cerr << "\n";
                       return {};
                     }
                     return operands;
                   }) |
               ranges::views::for_each([&env, closure](const auto &value) {
                 return std::visit(ToStream(env, closure), value);
               });
      };
      return input_mode == InputMode::kValue ? build_stream(Stream(input))
                                             : build_stream(ranges::views::single(input));
    };
  }

  std::expected<Stream, std::string> build(Env &env) && { return std::move(*this).factory(env)(); }

 private:
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
  using Pair = std::expected<std::pair<int64_t, std::optional<int64_t>>, std::string_view>;
  return ranges::views::zip(*from, to) |
         ranges::views::transform([from_str, to_str](auto from_to) -> Pair {
           auto [from, to] = from_to;
           if (!from) return std::unexpected(from_str);
           if (!to && !to_str.empty()) return std::unexpected(to_str);
           return std::pair{*from, to};
         }) |
         ranges::views::for_each([](auto pair) -> Stream {
           if (!pair) {
             return ranges::views::single(
                 StreamError(std::format("'{}' is not a number", pair.error())));
           }
           auto [from, to] = *pair;
           return to ? Stream(ranges::views::iota(from, *to + 1) | toNumber)
                     : Stream(ranges::views::iota(from) | toNumber);
         });
}

auto toOperand(const Env &env, Closure &closure, std::string_view token) -> Operand {
  if (token.starts_with('$')) {
    return StreamRef(token.substr(1));
  }
  if (auto stream = parseNumericRange(closure, token)) {
    return *stream;
  }
  if (auto val = google::protobuf::Value(); token.starts_with('`')) {
    val.set_string_value(token.substr(1, token.size() - 2) | ranges::views::split(' ') |
                         ranges::views::transform([&](auto &&s) {
                           if (auto stream = env.getEnv({toStringView(s)})) {
                             throw stream;
                           }
                           return s;
                         }) |
                         ranges::views::join(' ') | ranges::to<std::string>());
    return val;
  } else if (token.starts_with("'")) {
    val.set_string_value(token.substr(1, token.size() - 2));
    return val;
  } else if (google::protobuf::util::JsonStringToMessage(token, &val).ok()) {
    return val;
  }
  return Word(token);
}

//

auto unaryOp(const CommandBuilder &lhs, std::string_view op) {
  if (op == "-" && lhs.operands.empty()) return 9;
  if (op == "!") return 9;
  return 0;
}

auto binaryOp(std::string_view op) {
  if (op == "||" || op == "&&") return 4;
  if (op == "==" || op == "!=") return 5;
  if (op == "<" || op == "<=" || op == ">" || op == ">=") return 6;
  if (op == "+" || op == "-") return 7;
  if (op == "*" || op == "/" || op == "%") return 8;
  return 0;
}

auto precedence(std::string_view op) {
  if (op == "=" || op == ":" || op == "->") return 1;
  if (op == ";") return 2;
  if (op == "|") return 3;
  if (op == "!") return 9;
  if (auto p = binaryOp(op)) return p;
  return 0;
}

auto isOperator(std::string_view op) {
  return precedence(op) > 0;
}

//

struct StreamParserImpl final : StreamParser {
  StreamParserImpl(Env &env) : env{env} {}

  auto parse(std::vector<std::string_view> &&)
      -> std::expected<PrintableStream, std::string> override;

 private:
  auto performOp(auto &&pred) -> std::expected<void, std::string>;

  Env &env;
  std::stack<CommandBuilder> cmds;
  std::stack<std::string_view> ops;
};

auto StreamParserImpl::parse(std::vector<std::string_view> &&tokens)
    -> std::expected<PrintableStream, std::string> {
  // reset state
  cmds = {};
  ops = {};
  cmds.emplace();

  for (auto token : tokens) {
    if (isOperator(token)) {
      if (auto res = performOp([token](auto &op) { return precedence(op) >= precedence(token); });
          !res.has_value()) {
        return std::unexpected(res.error());
      }
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.closure = lhs.closure;

      if (token == "->") {
        if (lhs.operands.size() != 1) {
          return std::unexpected(lhs.operands.empty() ? "Missing closure variable"
                                                      : "Expected 1 closure variable");
        } else if (auto *closure_var = std::get_if<Word>(&lhs.operands[0])) {
          rhs.input_mode = lhs.input_mode;
          rhs.input = lhs.input | ranges::views::transform(
                                      [var = rhs.closure.add(*closure_var)](const Value &value) {
                                        *var = value;
                                        return Value();
                                      });
        } else {
          return std::unexpected("Invalid closure signature");
        }
      }

    } else if (token == "(") {
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.closure = lhs.closure;

    } else if (token == ")") {
      if (auto res = performOp([&](auto &op) { return op != "("; }); !res.has_value()) {
        return std::unexpected(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      auto stream = std::move(rhs).build(env);
      if (!stream) {
        return std::unexpected(stream.error());
      }
      cmds.top().operands.push_back(std::move(*stream));

    } else if (token == "{" && (ops.top() == "(" || !cmds.top().operands.empty())) {
      // NOT A CLOSURE
      return std::unexpected("not a closure");
      // ops.push(token);

    } else if (token == "{") {
      if (auto res = performOp([&](auto &op) { return op != "{"; }); !res.has_value()) {
        return std::unexpected(res.error());
      }
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.input_mode = InputMode::kValue;
      rhs.input = std::move(lhs.input);
      rhs.closure = lhs.closure;

    } else if (token == "}") {
      if (ops.empty() || cmds.size() < 2) {
        return std::unexpected("Misaligned '}'");
      } else if (auto res = performOp([&](auto &op) { return op != "{"; }); !res.has_value()) {
        return std::unexpected(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();
      cmds.top() = std::move(rhs);

    } else {
      cmds.top().operands.push_back(toOperand(env, cmds.top().closure, token));
    }
  }

  if (auto res = performOp([&](auto &) { return true; }); !res.has_value()) {
    return std::unexpected(res.error());
  }
  if (cmds.empty()) {
    return {};
  }
  if (cmds.size() != 1) {
    return std::unexpected("Could not parse command");
  }
  auto print_mode = cmds.top().print_mode;
  return std::move(cmds.top()).build(env).transform([&](auto &&stream) {
    return std::pair{std::move(stream), std::move(print_mode)};
  });
}

auto StreamParserImpl::performOp(auto &&pred) -> std::expected<void, std::string> {
  while (!ops.empty() && pred(ops.top())) {
    if (cmds.size() < 2) {
      return std::unexpected("Operator expected Operands");
    }
    auto rhs = std::move(cmds.top());
    cmds.pop();
    auto lhs = std::move(cmds.top());
    cmds.pop();

    if (unaryOp(lhs, ops.top())) {
      if (rhs.operands.empty()) {
        return std::unexpected("Operator expected Value");
      }

      auto result = std::visit(OperandOp(env, rhs.closure, ops.top()), rhs.operands.back());
      if (!result) {
        return std::unexpected(result.error());
      }
      lhs.operands.push_back(std::move(*result));
      cmds.push(std::move(lhs));

    } else if (binaryOp(ops.top())) {
      if (lhs.operands.empty() || rhs.operands.empty()) {
        return std::unexpected("Operator expected Value(s)");
      }

      auto result = std::visit(
          OperandOp(env, rhs.closure, ops.top()), lhs.operands.back(), rhs.operands.front());
      if (!result) {
        return std::unexpected(result.error());
      }
      lhs.operands.back() = std::move(*result);
      cmds.push(std::move(lhs));

    } else if (ops.top() == "=") {
      auto lhs_ref = std::get_if<StreamRef>(lhs.operands.empty() ? nullptr : &lhs.operands.back());

      if (!lhs_ref) {
        return std::unexpected("Expected a StreamRef");
      }
      env.setEnv(*lhs_ref, std::move(rhs).factory(env));
      cmds.push(std::move(lhs));

    } else if (ops.top() == ":") {
      std::optional<size_t> window;

      auto rhs_val = std::get_if<google::protobuf::Value>(
          rhs.operands.empty() ? nullptr : &rhs.operands.back());

      if (rhs_val && rhs_val->has_number_value()) {
        window = rhs_val->number_value();
      }

      if (window) {
        lhs.print_mode = Print::Slice{*window};
      } else {
        lhs.print_mode = Print::Pull{.full = true};
      }
      cmds.push(std::move(lhs));

    } else if (ops.top() == "->") {
      auto closure_var = std::get<Word>(lhs.operands[0]);
      assert(lhs.closure.vars.size() + 1 == rhs.closure.vars.size());
      assert(rhs.closure.vars.contains(closure_var));

      cmds.push(std::move(rhs));

    } else if (ops.top() == ";") {
      auto stream = std::move(lhs).build(env);
      if (!stream) {
        return std::unexpected(stream.error());
      }
      ranges::for_each(*stream, [](auto &&) {});
      cmds.push(std::move(rhs));

    } else if (ops.top() == "|") {
      auto stream = std::move(lhs).build(env);
      if (!stream) {
        return std::unexpected(stream.error());
      }
      rhs.input = std::move(*stream);
      cmds.push(std::move(rhs));
    }
    ops.pop();
  }
  return {};
}

}  // namespace

std::unique_ptr<StreamParser> makeStreamParser(Env &env) {
  return std::make_unique<StreamParserImpl>(env);
}
