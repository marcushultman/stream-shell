
#include "stream-shell/stream_parser.h"
#include <string>
#include <vector>
#include <boost/test/unit_test.hpp>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <range/v3/all.hpp>
#include "stream-shell/operand_op.h"
#include "stream-shell/tokenize.h"

using namespace std::string_view_literals;

constexpr auto each = boost::test_tools::per_element();

namespace google::protobuf {

bool operator==(const IsValue auto &lhs, const IsValue auto &rhs) {
  return google::protobuf::util::MessageDifferencer::Equals(lhs, rhs);
}

auto &operator<<(std::ostream &os, const IsValue auto &value) {
  std::string str;
  BOOST_TEST(google::protobuf::util::MessageToJsonString(value, &str).ok());
  return os << str;
}

auto &operator<<(std::ostream &os, const ::Value &value) {
  return std::visit([&](auto &value) -> auto & { return os << value; }, value);
}

auto &operator<<(std::ostream &os, const Result<::Value> &value) {
  return value ? (os << *value) : (os << "Error " << int(value.error()));
}

}  // namespace google::protobuf

auto &operator<<(std::ostream &os, const Print::Pull &mode) {
  return os << "Pull { full=" << mode.full << " }";
}
auto &operator<<(std::ostream &os, const Print::Slice &mode) {
  return os << "Slice { window=" << mode.window << " }";
}
auto &operator<<(std::ostream &os, const Print::WriteFile &mode) {
  return os << "WriteFile { filename="
            << (decltype(Print::WriteFile::filename)(mode.filename) | ranges::to<std::string>)
            << " }";
}
auto &operator<<(std::ostream &os, const Print::Mode &mode) {
  return std::visit([&](const auto &mode) -> auto & { return os << mode; }, mode);
}

BOOST_AUTO_TEST_SUITE(stream_parser_test)

google::protobuf::Value makeValue(bool b) {
  google::protobuf::Value v;
  v.set_bool_value(b);
  return v;
}

google::protobuf::Value makeValue(int d) {
  google::protobuf::Value v;
  v.set_number_value(d);
  return v;
}

google::protobuf::Value makeValue(double d) {
  google::protobuf::Value v;
  v.set_number_value(d);
  return v;
}

google::protobuf::Value makeValue(std::string_view s) {
  google::protobuf::Value v;
  v.set_string_value(s);
  return v;
}

struct JSON {
  std::string str;
};
google::protobuf::Value makeValue(JSON json) {
  google::protobuf::Value v;
  BOOST_TEST(google::protobuf::json::JsonStringToMessage(json.str, v.mutable_struct_value()).ok());
  return v;
}

template <typename... Args>
std::vector<Value> makeValues(Args &&...args) {
  return {makeValue(std::forward<Args>(args))...};
}

struct TestEnv : Env {
  StreamFactory getEnv(StreamRef) const override { return {}; }
  void setEnv(StreamRef, StreamFactory) override {}
  bool sleepUntil(std::chrono::steady_clock::time_point) override { return true; }
  ssize_t read(int fd, google::protobuf::BytesValue &bytes) override { return -1; }
};

TestEnv env;

auto parse(std::string input, Print::Mode *out_mode = nullptr) {
  auto [stream, print_mode] = makeStreamParser(env)->parse(tokenize(input));
  if (out_mode) *out_mode = print_mode;
  return stream | ranges::to<std::vector<Result<Value>>>();
}

BOOST_AUTO_TEST_CASE(empty) {
  BOOST_TEST(parse("").empty());
}

BOOST_AUTO_TEST_CASE(bools) {
  BOOST_TEST(parse("true") == makeValues(true), each);
  BOOST_TEST(parse("false || false") == makeValues(false), each);
  BOOST_TEST(parse("true && false") == makeValues(false), each);
  BOOST_TEST(parse("true && !false") == makeValues(true), each);
  BOOST_TEST(parse("true !false true || true") == makeValues(true, true, true), each);
}

BOOST_AUTO_TEST_CASE(numbers) {
  BOOST_TEST(parse("1 + 2") == makeValues(3), each);
  BOOST_TEST(parse("1.5 - 0.75") == makeValues(.75), each);
  BOOST_TEST(parse("10 % 3") == makeValues(1), each);
  BOOST_TEST(parse("1 2 3") == makeValues(1, 2, 3), each);
  BOOST_TEST(parse("1 2..4 5") == makeValues(1, 2, 3, 4, 5), each);
}

BOOST_AUTO_TEST_CASE(strings) {
  BOOST_TEST(parse("'foo' + 'bar'") == makeValues("foobar"sv), each);
  BOOST_TEST(parse("\"foo\" + \"bar\"") == makeValues("foobar"sv), each);
  BOOST_TEST(parse("`foo` + `bar`") == makeValues("foobar"sv), each);
  // todo: fix double negation
  // BOOST_TEST(parse("!!`foo` !`foo`") == makeValues(true, false), each);
  // todo: stop using json for string interpolation
  // BOOST_TEST(parse("FOO=(1..3) `foo $FOO`") == makeValues("foo 1 2 3"sv), each);
}

BOOST_AUTO_TEST_CASE(parentheses) {
  BOOST_TEST(parse("1 -2 + 3") == makeValues(2), each);
  BOOST_TEST(parse("1 (-2 + 3)") == makeValues(1, 1), each);
  BOOST_TEST(parse("(1 -2)(+3)") == makeValues(-1, 3), each);
}

BOOST_AUTO_TEST_CASE(pipe) {
  BOOST_TEST(parse("1 | 2") == makeValues(2), each);
  BOOST_TEST(parse("1.. | 2 3") == makeValues(2, 3), each);
  BOOST_TEST(parse("1 | ({ name: `Bernard` })") == makeValues(JSON("{ name: \"Bernard\" }")), each);
}

BOOST_AUTO_TEST_CASE(print) {
  Print::Mode mode;
  BOOST_TEST(parse("1 | 2 :42", &mode) == makeValues(2), each);
  BOOST_TEST(mode == Print::Mode(Print::Slice(42)));
}

BOOST_AUTO_TEST_CASE(file) {
  Print::Mode mode;
  BOOST_TEST(parse("true && false > mjau", &mode) == makeValues(false), each);
  BOOST_TEST(mode == Print::Mode(Print::WriteFile{"mjau"sv}));
}

BOOST_AUTO_TEST_CASE(json) {
  BOOST_TEST(parse("{ name: `Bernard` }") == makeValues(JSON("{ name: \"Bernard\" }")), each);
}

BOOST_AUTO_TEST_CASE(closure) {
  // todo: add builtin
  // BOOST_TEST(parse("1..2 | { add 1 }") == makeValues(2, 3), each);
  BOOST_TEST(parse("1..2 | { i -> i i * 2 }") == makeValues(1, 2, 2, 4), each);
  BOOST_TEST(parse(R"cmd(
    { name: `Albert` }
    { name: `Bernard` }
    | { person -> person.name })cmd") == makeValues("Albert"sv, "Bernard"sv),
             each);
  BOOST_TEST(parse("{ numbers: [1, 2] } { numbers: [3, 4] } | { e -> e.numbers }") ==
                 makeValues(1, 2, 3, 4),
             each);
}

BOOST_AUTO_TEST_CASE(builtins) {
  // todo: fake exit
  // BOOST_TEST(parse("exit") == makeValues(2, 3), each);
}

BOOST_AUTO_TEST_SUITE_END()
