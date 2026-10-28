#include "process_stream.h"

#include <thread>
#include "to_string.h"

auto sink(Stream stream, int out, int err) -> std::unique_ptr<Process> {
  struct StreamProcess final : Process {
    StreamProcess(Stream stream, int out, int err)
        : _stream{std::move(stream)}, _out(out), _err(err) {}
    ~StreamProcess() {
      if (_runner.joinable()) {
        _aborted = true;
        _runner.join();
      }
    }

    void run() override {
      if (_finished) {
        return;
      }
      _runner = std::thread([&] {
        char delimeter = 0;
        ranges::for_each(_stream | ranges::views::take_while([&](auto &&) { return !_aborted; }),
                         [&](auto result) {
                           if (result) {
                             if (auto str = std::visit(_to_str, *result)) {
                               if (delimeter) {
                                 write(_out, &delimeter, 1);
                               }
                               write(_out, str->data(), str->size());
                               delimeter = '\n';
                             }
                           } else {
                             auto err = std::to_string(std::to_underlying(result.error()));
                             write(_err, err.data(), err.size());
                           }
                         });
        if (delimeter) {
          write(_out, &delimeter, 1);
        }
        _finished = true;
        _cv.notify_all();
      });
    }
    bool wait() override {
      auto lock = std::unique_lock{_mutex};
      _cv.wait(lock, [&] { return _finished.load(); });
      return true;
    }

    Stream _stream;
    const int _out;
    const int _err;

    ToString::Value _to_str;
    std::thread _runner;

    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic_bool _aborted = false;
    std::atomic_bool _finished = false;
  };

  return std::make_unique<StreamProcess>(stream, out, err);
}

auto unsink(Env &env, std::unique_ptr<Process> process, int in, int err) -> Stream {
  return ranges::views::generate(
             [&env,
              process = std::shared_ptr{std::move(process)},
              in,
              err,
              bytes = google::protobuf::BytesValue()] mutable -> std::optional<Result<Value>> {
               process->run();

               if (env.read(err, bytes)) {
                 return std::unexpected(Error::kExecError);

               } else if (auto n = env.read(in, bytes); n == 0) {
                 close(in);

                 if (!process->wait()) {
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
