#include "env_impl.h"

#include <fstream>
#include "tokenize.h"

EnvImpl::EnvImpl() {
  if (auto *xdg_data_home_dir = getenv("XDG_DATA_HOME")) {
    load(xdg_data_home_dir);
  } else if (auto *home_dir = getenv("HOME")) {
    load(std::string(home_dir) + "/.config");
  }
  setEnv({"STSH_VERSION"}, [](auto) {
    auto value = google::protobuf::Value();
    value.set_string_value(STSH_VERSION);
    return ranges::yield(value);
  });
}

StreamFactory EnvImpl::getEnv(StreamRef ref) const {
  if (auto it = _cache.find(ref); it != _cache.end()) {
    return it->second.stream;
  } else if (auto str = std::getenv(ref.name.c_str())) {
    return _cache[ref].stream = [sv = std::string_view(str)](auto) {
      return sv | ranges::views::split(':') | ranges::views::transform([](auto chunk) {
               google::protobuf::Value value;
               value.set_string_value(chunk | ranges::to<std::string>);
               return value;
             });
    };
  }
  return {};
}
void EnvImpl::setEnv(StreamRef ref, StreamFactory stream) {
  if (ref.name == "PWD") {
    std::optional<std::string> pwd;
    ranges::for_each(stream({}), [&](auto result) {
      if (auto value = result ? std::get_if<google::protobuf::Value>(&*result) : nullptr;
          value && value->has_string_value()) {
        pwd = std::move(*value->mutable_string_value());
      }
    });
    if (pwd) {
      chdir(pwd->c_str());
    }
  }
  auto &entry = _cache[ref];
  entry.stream = std::move(stream);
  entry.version++;
  _cv.notify_all();
}

bool EnvImpl::blockUntilChange(StreamRef ref) {
  std::unique_lock lock(_mutex);
  _stop = false;
  auto &entry = _cache[ref];
  _cv.wait(lock, [&, version = entry.version] { return entry.version != version || _stop; });
  return !_stop;
}

bool EnvImpl::sleepUntil(std::chrono::steady_clock::time_point t) {
  std::unique_lock lock(_mutex);
  _stop = false;
  for (; !_stop && _cv.wait_until(lock, t) != std::cv_status::timeout;);
  return !_stop;
}

ssize_t EnvImpl::read(int fd, google::protobuf::BytesValue &bytes) {
  std::unique_lock lock(_mutex);
  _stop = false;
  lock.unlock();
  bytes.mutable_value()->resize(4096);
  auto ret = ::read(fd, bytes.mutable_value()->data(), bytes.mutable_value()->size());
  if (ret > 0) {
    bytes.mutable_value()->resize(ret);
  }
  lock.lock();
  return !_stop ? ret : -1;
}

void EnvImpl::interrupt() {
  std::unique_lock lock(_mutex);
  _stop = true;
  _cv.notify_all();
}

void EnvImpl::load(std::string path) {
  std::ifstream config(path + "/stream-shell/config.st", std::ios::in);
  if (!config.is_open()) {
    return;
  }
  _config = (std::stringstream() << config.rdbuf()).view() | ranges::views::split('\n') |
            ranges::views::filter(ranges::distance) | ranges::to<std::vector<std::string>>;
  for (auto &line : _config) {
    (void)_parser->parse(tokenize(line));
  }
}
