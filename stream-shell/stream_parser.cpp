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
#include "command.h"
#include "config.h"
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

#if !__EMSCRIPTEN__

inline auto exec(std::string cmd, std::vector<std::string> args) {
  if (cmd.starts_with('^')) {
    cmd.erase(cmd.begin());
  }
  auto argv =
      ranges::views::concat(ranges::views::single(cmd.data()),
                            args | ranges::views::transform([](auto &s) { return s.data(); }),
                            ranges::views::single(nullptr)) |
      ranges::to<std::vector>;
  return execvp(cmd.data(), argv.data());
}

#endif

auto errorStream(Error err) -> Stream {
  return ranges::yield(std::unexpected(err));
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

/**
 * A Command implementation always emitting an empty Stream.
 */
struct EmptyStreamCommand final : StreamConstantCommand {
  EmptyStreamCommand() : StreamConstantCommand({}) {}
};

/**
 * A Command implementation always emits a Stream with a single Error.
 */
struct ErrorStreamCommand final : StreamConstantCommand {
  ErrorStreamCommand(Error error) : StreamConstantCommand(ranges::yield(std::unexpected(error))) {}
};

/**
 * A Command implementation projecting an output Stream from a vector of Operands.
 */
struct StreamLiteralCommand final : StreamConstantCommand {
  StreamLiteralCommand(Env &env, Scope &scope, std::vector<Operand> operands)
      : StreamConstantCommand(ranges::yield(operands) | ranges::views::join |
                              ranges::views::for_each(ToStream(env, scope))) {}
};

#if !__EMSCRIPTEN__

/**
 * A Command implementation that forks and exec an executable.
 */
struct ExecutableCommand final : Command {
  ExecutableCommand(Env &env,
                    std::string cmd,
                    google::protobuf::Struct config,
                    std::unique_ptr<Command> upstream)
      : _env{env},
        _cmd{std::move(cmd)},
        _config{std::move(config)},
        _upstream{std::move(upstream)} {}

  Stream process(Stream in) override {
    std::array<int, 2> in_pipe, out_pipe, err_pipe;

    if (pipe(in_pipe.data()) != 0 || pipe(out_pipe.data()) != 0 || pipe(err_pipe.data()) != 0) {
      return errorStream(Error::kExecPipeError);
    }

    if (_upstream) {
      return unsink(_env,
                    exec(_upstream->process(in, in_pipe[1], err_pipe[1]),
                         in_pipe[0],
                         out_pipe[1],
                         err_pipe[1]),
                    out_pipe[0],
                    err_pipe[0]);

    } else {
      return unsink(_env,
                    exec(sink(in, in_pipe[1], err_pipe[1]), in_pipe[0], out_pipe[1], err_pipe[1]),
                    out_pipe[0],
                    err_pipe[0]);
    }
  }

  Stream process(int in, int err) override {
    if (_upstream) {
      std::array<int, 2> in_pipe, out_pipe;

      if (pipe(in_pipe.data()) != 0 || pipe(out_pipe.data()) != 0) {
        return errorStream(Error::kExecPipeError);
      }

      return unsink(_env,
                    exec(_upstream->process(in, in_pipe[1], err), in_pipe[0], out_pipe[1], err),
                    out_pipe[0],
                    err);

    } else {
      std::array<int, 2> out_pipe;

      if (pipe(out_pipe.data()) != 0) {
        return errorStream(Error::kExecPipeError);
      }

      return unsink(_env, exec({}, in, out_pipe[1], err), out_pipe[0], err);
    }
  }

  std::unique_ptr<Process> process(Stream in, int out, int err) override {
    std::array<int, 2> in_pipe;

    if (pipe(in_pipe.data()) != 0) {
      return sink(errorStream(Error::kExecPipeError), out, err);
    }

    if (_upstream) {
      return exec(_upstream->process(in, in_pipe[1], err), in_pipe[0], out, err);

    } else {
      return exec(sink(in, in_pipe[1], err), in_pipe[0], out, err);
    }
  }

  std::unique_ptr<Process> process(int in, int out, int err) override {
    if (_upstream) {
      std::array<int, 2> in_pipe;

      if (pipe(in_pipe.data()) != 0) {
        return sink(errorStream(Error::kExecPipeError), out, err);
      }

      return exec(_upstream->process(in, in_pipe[1], err), in_pipe[0], out, err);

    } else {
      return exec({}, in, out, err);
    }
  }

 private:
  std::unique_ptr<Process> exec(std::unique_ptr<Process> upstream_process,
                                int child_read,
                                int child_write,
                                int child_err) {
    struct ProcessImpl : Process {
      ProcessImpl(std::string cmd,
                  google::protobuf::Struct config,
                  std::unique_ptr<Process> upstream_process,
                  int child_read,
                  int child_write,
                  int child_err)
          : _cmd{std::move(cmd)},
            _config{std::move(config)},
            _upstream_process{std::move(upstream_process)},
            _child_read{child_read},
            _child_write{child_write},
            _child_err{child_err} {}

      void run() override {
        if (_upstream_process) {
          _upstream_process->run();
        }

        if (!_pid) {
          if (auto pid = fork(); pid == 0) {
            dup2(_child_read, STDIN_FILENO);
            dup2(_child_write, STDOUT_FILENO);
            dup2(_child_err, STDERR_FILENO);

            ::exec(_cmd, toArgs(_config));
            _exit(1);

          } else {
            _pid = pid;
          }
        }
      }
      bool wait() override {
        return _pid
            .transform([&](auto pid) {
              int status = 0;
              waitpid(*_pid, &status, 0);
              return status == 0;
            })
            .value_or(true);
      }

     private:
      std::string _cmd;
      google::protobuf::Struct _config;
      const std::unique_ptr<Process> _upstream_process;
      const int _child_read;
      const int _child_write;
      const int _child_err;

      std::optional<int> _pid;
    };

    return std::make_unique<ProcessImpl>(
        _cmd, _config, std::move(upstream_process), child_read, child_write, child_err);
  }

  Env &_env;
  std::string _cmd;
  google::protobuf::Struct _config;
  std::unique_ptr<Command> _upstream;
};

#endif

struct CommandBuilder {
  Scope scope;

  std::unique_ptr<Command> upstream;
  std::vector<Operand> operands;
  std::shared_ptr<Value> closure_var;

  Closure closure;

  // todo: mutex with closure?
  std::string record_literal;

  [[nodiscard]] auto build(Env &env) && -> std::unique_ptr<Command> {
    if (closure) {
      assert(upstream);
      assert(operands.empty());
      return std::make_unique<ClosureCommand>(env, std::move(upstream), std::move(closure));
    }

    if (operands.empty()) {
      return std::make_unique<EmptyStreamCommand>();
    }

    if (auto cmd = frontCommand(scope, operands[0])) {
      if (auto config = toConfig(env, std::span{operands}.subspan(1)); !config) {
        return std::make_unique<ErrorStreamCommand>(config.error());

      } else if (auto command = buildBuiltin(env, *cmd, *config, upstream)) {
        return command;

#if !__EMSCRIPTEN__
      } else if (isExecutableInPath(*cmd)) {
        return std::make_unique<ExecutableCommand>(env, *cmd, *config, std::move(upstream));
#endif
      }
    }

    return std::make_unique<StreamLiteralCommand>(env, scope, std::move(operands));
  }

  auto buildShared(Env &env) -> std::shared_ptr<Command> { return std::move(*this).build(env); }

  Closure buildClosure(Env &env) && {
    return [cmd = std::move(*this).buildShared(env)](auto value) {
      return cmd->process(ranges::yield(value));
    };
  }

  EnvStream buildEnv(Env &env) && {
    return [cmd = std::move(*this).buildShared(env)] { return cmd->process(Stream{}); };
  }

  Operand operand(Env &env) && {
    if (operands.size() == 1) {
      if (auto operand = std::visit(ValueVisitor<Operand>(), operands[0])) {
        return *operand;
      }
    }
    return std::move(*this).build(env)->process(Stream{});
  }

  [[nodiscard]] std::unique_ptr<Process> run(Env &env,
                                             int in = STDIN_FILENO,
                                             int out = STDOUT_FILENO,
                                             int err = STDERR_FILENO) && {
    auto process = std::move(*this).build(env)->process(in, out, err);
    process->run();
    return process;
  }

 private:
  static const std::string *frontCommand(const Scope &scope, const Operand &operand) {
    if (auto *word = getIfString(operand)) {
      return scope.vars.contains(*word) ? nullptr : word;
    }
    return nullptr;
  }

  static bool isExecutable(const std::filesystem::path &p) {
    return std::filesystem::is_regular_file(p) && (access(p.c_str(), X_OK) == 0);
  }

  static bool isExecutableInPath(std::string_view cmd) {
    auto path = std::filesystem::path{cmd};

    if (std::ranges::contains(path.string(), '/')) {
      return isExecutable(path);
    }

    const char *path_env = std::getenv("PATH");
    if (!path_env) {
      return false;
    }
    return ranges::count_if(std::string_view(path_env) | ranges::views::split(':') |
                                ranges::views::transform(std::views::common) |
                                ranges::views::transform([&](auto dir) {
                                  return std::filesystem::path{dir.begin(), dir.end()} / path;
                                }),
                            isExecutable);
  }
};

//

auto isStreamRefAssignment(const CommandBuilder &lhs,
                           std::ranges::range auto op) -> const StreamRef * {
  return op == "=" && lhs.operands.size() == 1 ? std::get_if<StreamRef>(&lhs.operands[0]) : nullptr;
}

auto isEnvOverride(const CommandBuilder &lhs, std::ranges::range auto op) -> const std::string * {
  return op == "=" && lhs.operands.size() == 1 ? getIfString(lhs.operands[0]) : nullptr;
}

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
  if (op == "=") return 3;
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
  if (op == "|") return 3;
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

  auto operator()(const Operand &operand) const -> Result<std::string> {
    return std::visit(*this, operand);
  }

  ToString::Operand _to_str;
};

auto appendRecordLiteral(Env &env, CommandBuilder &cmd, Token token) -> Result<void> {
  if (auto str = lift(cmd.operands | ranges::views::transform(ToJSON(env, cmd.scope)))
                     .transform([](auto &&s) {
                       return s | ranges::views::join | ranges::to<std::string>;
                     })) {
    cmd.operands.clear();
    cmd.record_literal += *str;

  } else {
    return std::unexpected(str.error());
  }

  cmd.record_literal += token | ranges::to<std::string>;

  return {};
}

//

struct StreamParserImpl final : StreamParser {
  StreamParserImpl(Env &env) : env{env} {}

  auto parse(Tokens) -> Stream override;
  auto parseInteractive(Tokens) -> void override;

 private:
  auto parse(Tokens, bool is_interactive) -> Stream;

  using OpPred = std::function<bool(Token)>;

  auto toOperand(ranges::bidirectional_range auto token) -> std::optional<Operand>;

  auto performOp(const OpPred &pred) -> Result<void>;

  Env &env;
  std::stack<CommandBuilder> cmds;
  std::stack<Token> ops;
};

auto StreamParserImpl::parse(Tokens tokens, bool is_interactive) -> Stream {
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
      rhs.record_literal = lhs.record_literal;

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
        rhs.record_literal += "{";
      }

    } else if (token == "}" && cmds.top().record_literal.empty()) {
      if (auto res = performOp([&](const auto &op) { return op != "{"; }); !res.has_value()) {
        return errorStream(res.error());
      }
      ops.pop();
      auto rhs = std::move(cmds.top());
      cmds.pop();

      if (auto closure_var = rhs.closure_var) {
        cmds.top().closure = [closure_var, rhs = std::move(rhs).buildShared(env)](auto value) {
          *closure_var = value;
          return rhs->process(Stream{});
        };
      } else {
        cmds.top().closure = std::move(rhs).buildClosure(env);
      }

      // todo: generalize open ternary
    } else if (token == "?") {
      ops.push(token);
      auto &lhs = cmds.top();
      auto &rhs = cmds.emplace();

      rhs.scope = lhs.scope;
      rhs.record_literal = std::move(lhs.record_literal);

    } else if (token == "->") {
      auto &lhs = cmds.top();
      auto *var_name = lhs.operands.size() == 1 ? getIfString(lhs.operands[0]) : nullptr;
      if (!var_name || lhs.upstream) {
        return errorStream(Error::kInvalidClosureSignature);
      }
      lhs.closure_var = lhs.scope.add(*var_name);
      lhs.operands.clear();

    } else if (auto value = toOperand(token)) {
      cmds.top().operands.push_back(std::move(*value));

    } else if (!cmds.top().record_literal.empty()) {
      if (auto res = performOp([&](const auto &op) { return op != "{"; }).and_then([&] {
            return appendRecordLiteral(env, cmds.top(), token);
          });
          !res) {
        return errorStream(res.error());
      }

      if (token == "}") {
        auto value = google::protobuf::Value();
        if (!google::protobuf::json::JsonStringToMessage(cmds.top().record_literal,
                                                         value.mutable_struct_value())
                 .ok()) {
          return errorStream(Error::kJsonError);
        }
        ops.pop();
        cmds.pop();
        cmds.top().operands.push_back(std::move(value));
      }

    } else {
      auto value = google::protobuf::Value();
      value.set_string_value(token | ranges::to<std::string>);
      cmds.top().operands.push_back(std::move(value));
    }
  }

  if (auto res = performOp([&](const auto &) { return true; }); !res.has_value()) {
    return errorStream(res.error());
  }
  if (cmds.size() != 1) {
    return errorStream(Error::kParseError);
  }

  if (is_interactive) {
    if (auto process = std::move(cmds.top()).run(env); !process->wait()) {
      return errorStream(Error::kExecNonZeroStatus);
    }
    return Stream();
  } else {
    return std::move(cmds.top()).build(env)->process(Stream());
  }
}

auto StreamParserImpl::parse(Tokens tokens) -> Stream {
  return parse(std::move(tokens), false);
}
auto StreamParserImpl::parseInteractive(Tokens tokens) -> void {
  parse(std::move(tokens), true);
}

auto StreamParserImpl::toOperand(ranges::bidirectional_range auto token) -> std::optional<Operand> {
  auto &scope = cmds.top().scope;

  if (ranges::starts_with(token, "$"sv)) {
    return StreamRef::fromToken(token);
  }
  if (ranges::starts_with(token, "`"sv)) {
    auto to_str = ToString::Operand(env, scope, true);
    return ranges::yield(lift(trim(token, 1, 1) | ranges::views::split(' ') |
                              ranges::views::transform([&](auto &&token) -> Result<std::string> {
                                if (ranges::starts_with(token, "$"sv)) {
                                  return to_str(StreamRef::fromToken(token));
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

  auto make_bool = [](bool b) {
    auto val = google::protobuf::Value();
    val.set_bool_value(b);
    return val;
  };

  if (ranges::starts_with(token, "--"sv)) {
    auto key = token | ranges::views::drop(2);

    google::protobuf::Value val;
    auto *fields = val.mutable_struct_value()->mutable_fields();

    if (ranges::starts_with(key, "no-"sv)) {
      fields->emplace(key | ranges::views::drop(3) | ranges::to<std::string>, make_bool(false));
    } else {
      fields->emplace(key | ranges::to<std::string>, make_bool(true));
    }
    return val;
  } else if (ranges::starts_with(token, "-"sv)) {
    google::protobuf::Value val;
    for (auto flag : token | ranges::views::drop(1)) {
      val.mutable_struct_value()->mutable_fields()->emplace(std::string(1, flag), make_bool(true));
    }
    return val;
  }

  auto path = token | ranges::views::split('.');

  // todo: fix closure variable in record
  if (auto it = scope.vars.find(ranges::front(path)); it != scope.vars.end()) {
    return ranges::yield(0) | ranges::views::transform([var = it->second](auto) { return *var; }) |
           ranges::views::for_each([path = path | ranges::views::drop(1)](auto value) {
             return lookupField(std::move(value), path);
           });
  }
  return {};
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

    if (auto *ref = isStreamRefAssignment(lhs, ops.top())) {
      env.setEnv(*ref, std::move(rhs).buildEnv(env));
      lhs.operands.clear();
      cmds.push(std::move(lhs));

    } else if (auto *var = isEnvOverride(lhs, ops.top())) {
      if (!(lhs.operands = rhs.operands).empty()) {
        lhs.operands.erase(lhs.operands.begin());
        rhs.operands.resize(1);
      }
      lhs.scope.env_overrides[*var] = std::move(rhs).buildEnv(env);
      cmds.push(std::move(lhs));

    } else if (unaryLeftOp(lhs.operands.empty(), ops.top())) {
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
      ranges::for_each(std::move(lhs).buildEnv(env)(), [](auto &&) {});
      cmds.push(std::move(rhs));

    } else if (ops.top() == "|") {
      rhs.upstream = std::move(lhs).build(env);
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
