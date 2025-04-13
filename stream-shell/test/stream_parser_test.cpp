
#include "stream-shell/stream_parser.h"
#include <boost/test/unit_test.hpp>
#include <google/protobuf/util/json_util.h>
#include <range/v3/all.hpp>
#include "stream-shell/tokenize.h"

using namespace std::string_view_literals;

constexpr auto each = boost::test_tools::per_element();

bool operator==(const Result<Value> &value, bool b) {
  return std::get<google::protobuf::Value>(*value).bool_value() == b;
}

bool operator==(const Result<Value> &value, double n) {
  return std::get<google::protobuf::Value>(*value).number_value() == n;
}
bool operator==(const Result<Value> &value, int n) {
  return value == double(n);
}

bool operator==(const Result<Value> &value, std::string_view s) {
  return std::get<google::protobuf::Value>(*value).string_value() == s;
}

auto &operator<<(std::ostream &os, const Result<Value> &value) {
  if (value) {
    std::string str;
    std::visit(
        [&](auto &value) {
          BOOST_TEST(google::protobuf::util::MessageToJsonString(value, &str).ok());
        },
        *value);
    return os << str;
  }
  return os << "Error " << int(value.error());
}

BOOST_AUTO_TEST_SUITE(strean_parser_test)

struct TestEnv : Env {
  StreamFactory getEnv(StreamRef) const override { return {}; }
  void setEnv(StreamRef, StreamFactory) override {}
};

TestEnv env;

auto parse(std::string input) {
  auto parser = makeStreamParser(env);
  auto [stream, print_mode] = parser->parse(tokenize(input));
  return stream | ranges::to<std::vector<Result<Value>>>();
}

BOOST_AUTO_TEST_CASE(empty) {
  BOOST_TEST(parse("").empty());
}

BOOST_AUTO_TEST_CASE(stream_bools) {
  BOOST_TEST(parse("true") == std::vector({true}), each);
  BOOST_TEST(parse("false || false") == std::vector({false}), each);
  BOOST_TEST(parse("true && false") == std::vector<bool>({false}), each);
  BOOST_TEST(parse("true && !false") == std::vector<bool>({true}), each);
}

BOOST_AUTO_TEST_CASE(stream_numbers) {
  BOOST_TEST(parse("1 + 2") == std::vector({3}), each);
  BOOST_TEST(parse("1.5 - 0.75") == std::vector({.75}), each);
  BOOST_TEST(parse("10 % 3") == std::vector({1}), each);
  BOOST_TEST(parse("1 2 3") == std::vector({1, 2, 3}), each);
  BOOST_TEST(parse("1 2..4 5") == std::vector({1, 2, 3, 4, 5}), each);
}

BOOST_AUTO_TEST_CASE(stream_strings) {
  BOOST_TEST(parse("\"foo\" + \"bar\"") == std::vector({"foobar"sv}), each);
}

BOOST_AUTO_TEST_SUITE_END()
