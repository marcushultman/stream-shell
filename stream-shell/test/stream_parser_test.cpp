
#include "stream-shell/stream_parser.h"
#include <boost/test/unit_test.hpp>
#include <google/protobuf/util/json_util.h>
#include <range/v3/all.hpp>
#include "stream-shell/tokenize.h"

using namespace std::string_view_literals;

constexpr auto each = boost::test_tools::per_element();

bool operator==(const Result<Value> &value, double n) {
  return std::get<google::protobuf::Value>(*value).number_value() == n;
}

bool operator==(const Result<Value> &value, const std::string &s) {
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
  auto [stream, print_mode] = parser->parse(tokenize(input) | ranges::to<std::vector>());
  return stream | ranges::to<std::vector<Result<Value>>>();
}

BOOST_AUTO_TEST_CASE(empty) {
  BOOST_TEST(parse("").empty());
}

BOOST_AUTO_TEST_CASE(stream_expr) {
  BOOST_TEST(parse("1 2 3") == std::vector<double>({1, 2, 3}), each);
  BOOST_TEST(parse("\"foo\" + \"bar\"") == std::vector<std::string>({"foobar"}), each);
}

BOOST_AUTO_TEST_SUITE_END()
