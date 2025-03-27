
#include "stream-shell/stream_parser.h"
#include <boost/test/unit_test.hpp>
#include <range/v3/all.hpp>

using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(rpn_test)

struct TestEnv : Env {
  std::optional<StreamFactory> getEnv(StreamRef) const override { return {}; }
  void setEnv(StreamRef, StreamFactory) override {}
};

TestEnv env;

auto parse(std::vector<std::string_view> &&tokens) {
  return makeStreamParser(env)->parse(std::move(tokens));
}

// BOOST_AUTO_TEST_CASE(rpn_empty) {
//   // rpn({}, {"$FOO", "=", "1", "2", "+", "3"});
//   BOOST_TEST(rpn(TestEnv(), {"'foo'", "+", "'bar'"}).has_value());
// }

BOOST_AUTO_TEST_CASE(eval_pipe) {
  // rpn({}, {"$FOO", "=", "1", "2", "+", "3"});
  BOOST_CHECK(parse({"1", "2", "3", "|", "4", "+", "5"}));
  BOOST_CHECK(parse({"1", "2", "3", ":"}));
}

BOOST_AUTO_TEST_SUITE_END()
