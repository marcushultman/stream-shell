
#include "stream-shell/config.h"

#include <boost/test/unit_test.hpp>
#include "test_env.h"

using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(config_test)

TestEnv env;

BOOST_AUTO_TEST_CASE(empty) {
  BOOST_TEST(toConfig(env, {}).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
