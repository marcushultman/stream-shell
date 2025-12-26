#include "stream_parser.h"

#include <expected>
#include <filesystem>
#include <functional>
#include <stack>
#include <fcntl.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/all.hpp>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <utmp.h>
#include "builtin.h"
#include "lift.h"
#include "operand.h"
#include "operand_op.h"
#include "scope.h"
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

inline auto exec(std::ranges::range auto args) {
  auto args_vec = args | ranges::to<std::vector<std::string>>;
  if (args_vec[0].starts_with('^')) {
    args_vec[0].erase(args_vec[0].begin());
  }
  auto argv =
      ranges::views::concat(args_vec | ranges::views::transform([](auto &s) { return s.data(); }),
                            ranges::views::single(nullptr)) |
      ranges::to<std::vector>;
  return execvp(args_vec[0].c_str(), argv.data());
}

auto errorStream(Error err) -> Stream {
  return ranges::views::single(std::unexpected(err));
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
  Scope scope;

  StreamFactory upstream;
  std::vector<Operand> operands;

  StreamFactory closure;

  // todo: mutex with closure?
  int record_level = 0;

  StreamFactory factory(Env &env) && {
    if (closure) {
      assert(upstream);
      return [upstream = std::move(upstream), closure = std::move(closure)](Stream input) {
        return upstream(std::move(input)) | ranges::views::for_each([=](Result<Value> result) {
                 return result ? closure(ranges::yield(*result)) : ranges::yield(result);
               });
      };
    }
    return [&env,
            upstream = std::move(upstream),
            scope = std::move(scope),
            operands = std::move(operands)](Stream input) -> Stream {
      return ranges::yield(upstream ? upstream(std::move(input)) : std::move(input)) |
             ranges::views::for_each([&env, scope, operands](Stream input) -> Stream {
               if (operands.empty()) {
                 return {};
               }

               if (auto cmd = frontCommand(scope, operands)) {
                 if (auto stream = runBuiltin(
                         *cmd, env, scope, std::move(input), operands | ranges::views::drop(1))) {
                   return *stream;
                 } else if (isExecutableInPath(Token(cmd->value) | ranges::to<std::string>)) {
                   return runChildProcess(env, scope, operands);
                 }
               }

               // Stream expression (ignoring input)
               return operands | ranges::views::for_each(ToStream(env, scope));
             });
    };
  }

  Stream build(Env &env) && { return std::move(*this).factory(env)(Stream()); }
  Operand operand(Env &env) && {
    if (operands.size() == 1) {
      if (auto operand = std::visit(ValueVisitor<Operand>(), operands[0])) {
        return *operand;
      }
    }
    return std::move(*this).build(env);
  }

 private:
  static const Word *frontCommand(const Scope &scope, const std::vector<Operand> &operands) {
    if (auto *word = std::get_if<Word>(&operands[0])) {
      return scope.vars.contains(*word) ? nullptr : word;
    }
    return nullptr;
  }

  static Stream runChildProcess(Env &env,
                                const Scope &scope,
                                const std::vector<Operand> &operands) {
#if !__EMSCRIPTEN__
    auto args = lift(operands | ranges::views::transform(ToString::Operand(env, scope)));

    if (!args) {
      return ranges::yield(std::unexpected(args.error()));
    }

    auto pty_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty_fd < 0 || grantpt(pty_fd) < 0 || unlockpt(pty_fd) < 0) {
      return ranges::yield(std::unexpected(Error::kExecPipeError));
    }

    const char *tty_name = ptsname(pty_fd);
    if (!tty_name) {
      close(pty_fd);
      return ranges::yield(std::unexpected(Error::kExecPipeError));
    }

    if (auto pid = fork(); pid == 0) {
      // Child processq
      int tty_fd = open(tty_name, O_RDWR);
      if (tty_fd < 0) _exit(1);

      if (setsid() == -1 || ioctl(tty_fd, TIOCSCTTY, 0) == -1 ||
          tcsetpgrp(tty_fd, getpid()) == -1) {
        close(tty_fd);
        _exit(1);
      }

      dup2(tty_fd, STDIN_FILENO);
      dup2(tty_fd, STDOUT_FILENO);
      dup2(tty_fd, STDERR_FILENO);

      close(pty_fd);
      close(tty_fd);

      exec(*args);
      _exit(1);

    } else if (pid > 0) {
      tcsetpgrp(pty_fd, pid);

      // Parent process
      return ranges::views::generate(
                 [&env, pty_fd, pid, bytes = google::protobuf::BytesValue()] mutable
                 -> std::optional<Result<Value>> {
                   if (auto n = env.read(pty_fd, bytes); n == 0) {
                     close(pty_fd);

                     int status = 0;
                     waitpid(pid, &status, 0);
                     tcsetpgrp(STDIN_FILENO, getpgrp());

                     if (status != 0) {
                       return std::unexpected(Error::kExecNonZeroStatus);
                     }
                     return std::nullopt;
                   } else if (n < 0) {
                     return std::unexpected(Error::kExecReadError);
                   } else {
                     return bytes;
                   }
                 }) |
             ranges::views::take_while([](const auto &value) { return value.has_value(); }) |
             ranges::views::transform([](auto &&value) { return std::move(*value); });
    }
    return ranges::yield(std::unexpected(Error::kExecForkError));
#else
    return ranges::yield(std::unexpected(Error::kExecError));
#endif
  }

  static bool isExecutable(const std::filesystem::path &p) {
    return std::filesystem::is_regular_file(p) && (access(p.c_str(), X_OK) == 0);
  }

  static bool isExecutableInPath(const std::filesystem::path &path) {
    if (ranges::contains(path.string(), '/')) {
      return isExecutable(path);
    }

    const char *path_env = std::getenv("PATH");
    if (!path_env) {
      return false;
    }
    return ranges::count_if(std::string_view(path_env) | ranges::views::split(':') |
                                ranges::views::transform([&](auto dir) {
                                  return std::filesystem::path{dir | ranges::to<std::string>} /
                                         path;
                                }),
                            isExecutable);
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
  // if (op == "??") return ??;
  if (op == "||" || op == "&&") return 5;
  if (op == "==" || op == "!=") return 6;
  if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "..") return 7;
  if (op == "+" || op == "-") return 8;
  if (op == "*" || op == "/" || op == "%") return 9;
  return 0;
}

auto ternaryOp(std::ranges::range auto op) {
  if (op == ":") return 4;
  return 0;
}

auto precedence(const CommandBuilder &lhs, std::ranges::range auto op) {
  if (auto p = unaryLeftOp(lhs.operands.empty(), op)) return p;
  if (auto p = unaryRightOp(true, op)) return p;
  if (auto p = binaryOp(op)) return p;
  if (auto p = ternaryOp(op)) return p;
  if (op == ";") return 2;
  if (op == "=" || op == "|") return 3;
  return 0;
}

auto isOperator(const CommandBuilder &lhs, std::ranges::range auto op) {
  return precedence(lhs, op) > 0;
}

std::optional<std::string_view> ternaryMatch(std::ranges::range auto op) {
  if (op == ":") return "?"sv;
  return {};
}

/**
 * Used at record-end evaluation to join all operands into a range of json-parsable tokens. These
 * are joined and parsed into a Value struct.
 */
struct ToJSON {
  ToJSON(Env &env, Scope scope) : _to_str{env, std::move(scope)} {}

  auto operator()(const google::protobuf::Message &val) const -> Result<std::string> {
    return _to_str(val);
  }
  auto operator()(const Stream &stream) const -> Result<std::string> {
    return ranges::fold_left(
               Stream(stream),
               google::protobuf::Value(),
               [&](Result<google::protobuf::Value> &&list, auto &&result) {
                 return std::move(list).and_then([&](auto &&list) {
                   return std::move(result).transform([&](auto &&value) -> google::protobuf::Value {
                     auto &item = *list.mutable_list_value()->add_values();

                     if (auto *bytes = std::get_if<google::protobuf::BytesValue>(&value)) {
                       // todo: base64 encode
                       item.set_string_value(bytes->Utf8DebugString());
                     } else if (auto *pvalue = std::get_if<google::protobuf::Value>(&value)) {
                       item = std::move(*pvalue);
                     } else if (auto *any = std::get_if<google::protobuf::Any>(&value)) {
                       // todo: something
                       item.set_string_value(any->Utf8DebugString());
                     }
                     return list;
                   });
                 });
               })
        .and_then([&](auto &&list) {
          if (list.list_value().values_size() == 1) {
            return (*this)(list.list_value().values(0));
          }
          return (*this)(std::forward<decltype(list)>(list));
        });
  }
  auto operator()(const StreamRef &ref) const -> Result<std::string> { return _to_str(this, ref); }
  auto operator()(const Word &word) const -> Result<std::string> { return _to_str(word); }

  auto operator()(const Operand &operand) const -> Result<std::string> {
    return std::visit(*this, operand);
  }

  ToString::Operand _to_str;
};

Stream toJSON(Env &env, Scope scope, std::vector<Operand> &&operands) {
  return ranges::yield(
      lift(ranges::yield(0) | ranges::views::for_each([operands = std::move(operands)](auto) {
             return ranges::views::concat(
                 ranges::yield(Word("{"sv)), operands, ranges::yield(Word("}"sv)));
           }) |
           ranges::views::transform(ToJSON(env, std::move(scope))))
          .transform([](auto &&s) { return s | ranges::views::join | ranges::to<std::string>; })
          .and_then([](auto &&str) -> Result<Value> {
            auto value = google::protobuf::Value();
            if (google::protobuf::json::JsonStringToMessage(str, value.mutable_struct_value())
                    .ok()) {
              assert(!value.has_list_value());
              return value;
            }
            return std::unexpected(Error::kJsonError);
          }));
}

//

struct StreamParserImpl final : StreamParser {
  StreamParserImpl(Env &env) : env{env} {}

  auto parse(ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>>)
      -> Stream override;

 private:
  using OpPred = std::function<bool(Token)>;

  auto toOperand(ranges::bidirectional_range auto token) -> Operand;

  auto performOp(const OpPred &pred) -> Result<void>;

  Env &env;
  std::stack<CommandBuilder> cmds;
  std::stack<Token> ops;
};

auto StreamParserImpl::parse(
    ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>> tokens)
    -> Stream {
  // reset state
  cmds = {};
  ops = {};
  cmds.emplace();

  for (auto token : tokens) {
    if (isOperator(cmds.top(), token)) {
      auto ternary_op = ternaryMatch(token);

      if (auto res = performOp([&](auto op) mutable {
            if (ternary_op) {
              return op != *ternary_op;
            }
            auto a = precedence(cmds.top(), op);
            auto b = precedence(cmds.top(), token);
            return a >= b;
          });
          !res.has_value()) {
        return errorStream(res.error());
      }

      if (ternary_op) {
        if (ops.empty()) {
          return errorStream(Error::kMissingTernary);
        }
        ops.pop();
      }
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.scope = lhs.scope;
      rhs.record_level = lhs.record_level;

    } else if (cmds.top().closure) {
      return errorStream(Error::kMissingOperator);

    } else if (token == "(") {
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.scope = lhs.scope;

    } else if (token == ")") {
      if (auto res = performOp([](const auto &op) { return op != "("; }); !res.has_value()) {
        return errorStream(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      cmds.top().operands.push_back(std::move(rhs).operand(env));

    } else if (token == "{") {
      // If it looks like a closure, assume it is
      auto is_closure = !ops.empty() && ops.top() == "|" && cmds.top().operands.empty();

      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.scope = lhs.scope;

      if (!is_closure) {
        rhs.record_level = lhs.record_level + 1;
      }

    } else if (token == "}") {
      if (auto res = performOp([&](const auto &op) { return op != "{"; }); !res.has_value()) {
        return errorStream(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      if (rhs.record_level == 0) {
        cmds.top().closure = std::move(rhs).factory(env);

      } else {
        cmds.top().operands.push_back(toJSON(env, rhs.scope, std::move(rhs.operands)));
      }

      // todo: generalize open ternary
    } else if (token == "?") {
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.scope = lhs.scope;
      rhs.record_level = lhs.record_level;

    } else if (token == "->") {
      auto &lhs = cmds.top();
      auto *var_name = lhs.operands.size() == 1 ? std::get_if<Word>(&lhs.operands[0]) : nullptr;
      if (!var_name || lhs.upstream) {
        return errorStream(Error::kInvalidClosureSignature);
      }
      lhs.upstream = [var = lhs.scope.add(std::move(*var_name))](Stream input) -> Stream {
        auto results = input | ranges::views::take(1) | ranges::to<std::vector>;
        if (results.empty() || !results.front()) {
          return results.empty() ? errorStream(Error::kInvalidClosureSignature)
                                 : ranges::yield(results.front());
        }
        *var = results.front().value();
        return {};
      };
      lhs.operands.clear();

    } else {
      cmds.top().operands.push_back(toOperand(token));
    }
  }

  if (auto res = performOp([&](const auto &) { return true; }); !res.has_value()) {
    return errorStream(res.error());
  }
  if (cmds.size() != 1) {
    return errorStream(Error::kParseError);
  }
  return std::move(cmds.top()).build(env);
}

auto StreamParserImpl::toOperand(ranges::bidirectional_range auto token) -> Operand {
  auto &scope = cmds.top().scope;

  if (ranges::starts_with(token, "$"sv)) {
    return StreamRef(token | ranges::views::drop(1));
  }
  if (ranges::starts_with(token, "`"sv)) {
    auto to_str = ToString::Operand(env, scope, true);
    return ranges::yield(lift(trim(token, 1, 1) | ranges::views::split(' ') |
                              ranges::views::transform([&](auto &&token) -> Result<std::string> {
                                if (ranges::starts_with(token, "$"sv)) {
                                  return to_str(StreamRef(token | ranges::views::drop(1)));
                                }
                                return token | ranges::to<std::string>;
                              }))
                             .transform([](auto &&parts) {
                               auto val = google::protobuf::Value();
                               val.set_string_value(parts | ranges::views::join(' ') |
                                                    ranges::to<std::string>());
                               return val;
                             }));
  }
  if (auto val = google::protobuf::Value(); ranges::starts_with(token, "'"sv)) {
    val.set_string_value(trim(token, 1, 1) | ranges::to<std::string>);
    return val;

  } else if (google::protobuf::util::JsonStringToMessage(token | ranges::to<std::string>, &val)
                 .ok()) {
    return val;
  }

  auto path = token | ranges::views::split('.');

  // todo: fix closure variable in record
  if (auto it = scope.vars.find(Word{ranges::front(path)}); it != scope.vars.end()) {
    return ranges::yield(0) | ranges::views::transform([var = it->second](auto) { return *var; }) |
           ranges::views::for_each([path = path | ranges::views::drop(1)](auto value) {
             return lookupField(std::move(value), path);
           });
  }
  return Word{token};
}

auto StreamParserImpl::performOp(const OpPred &pred) -> Result<void> {
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

    } else if (ternaryOp(ops.top())) {
      if (cmds.empty()) {
        return std::unexpected(Error::kMissingTernary);
      }
      auto llhs = std::move(cmds.top());
      cmds.pop();

      if (llhs.operands.empty() || lhs.operands.empty() || rhs.operands.empty()) {
        return std::unexpected(Error::kMissingOperand);
      }
      rhs.operands[0] = std::visit(OperandOp(ops.top()),
                                   std::move(llhs.operands.back()),
                                   std::move(lhs).operand(env),
                                   std::move(rhs.operands[0]));
      llhs.operands.pop_back();
      llhs.operands.append_range(rhs.operands);
      cmds.push(std::move(llhs));

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
        lhs.scope.env_overrides[*var] = std::move(rhs).factory(env);
        lhs.operands = operands;

      } else {
        return std::unexpected(Error::kInvalidStreamRef);
      }
      cmds.push(std::move(lhs));

    } else if (ops.top() == "|") {
      rhs.upstream = std::move(lhs).factory(env);
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
