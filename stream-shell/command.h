#pragma once

#include "process.h"
#include "process_stream.h"
#include "stream_parser.h"

struct Command {
  virtual ~Command() = default;
  virtual Stream process(Stream in) = 0;
  virtual Stream process(int in, int err) = 0;
  virtual std::unique_ptr<Process> process(Stream in, int out, int err) = 0;
  virtual std::unique_ptr<Process> process(int in, int out, int err) = 0;
};

/**
 * Base Command that enforces output Stream conversion.
 */
struct StreamOutputCommand : Command {
  virtual Stream process(Stream in) override = 0;
  virtual Stream process(int in, int err) override = 0;
  std::unique_ptr<Process> process(Stream in, int out, int err) final {
    return sink(process(std::move(in)), out, err);
  }
  std::unique_ptr<Process> process(int in, int out, int err) final {
    return sink(process(in, err), out, err);
  }
};

/**
 * A Command that emits a pre-built Stream.
 */
struct StreamConstantCommand : StreamOutputCommand {
  StreamConstantCommand(Stream stream) : _stream{std::move(stream)} {}

  Stream process(Stream) final { return _stream; }
  Stream process(int, int) final { return _stream; }

 private:
  Stream _stream;
};

/**
 * A Command implementation that flat maps Values using a Closure.
 * May be used by builtins, where upstream is optional.
 */
struct ClosureCommand final : StreamOutputCommand {
  ClosureCommand(Env &env, std::unique_ptr<Command> upstream, Closure closure)
      : _env{env}, _upstream{std::move(upstream)}, _closure{std::move(closure)} {
    assert(_closure);
  }

 private:
  auto closure() {
    return ranges::views::for_each([closure = _closure](auto result) {
      return result ? closure(*result) : ranges::yield(result);
    });
  }

 public:
  Stream process(Stream in) override {
    return _upstream ? (_upstream->process(std::move(in)) | closure()) : (in | closure());
  }
  Stream process(int in, int err) override {
    return _upstream ? (_upstream->process(in, err) | closure())
                     : (unsink(_env, {}, in, err) | closure());
  }

  Env &_env;
  std::unique_ptr<Command> _upstream;
  Closure _closure;
};
