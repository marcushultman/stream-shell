#include "stream-shell/config.h"

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include "test_env.h"

namespace {

// Construct operands directly: these tests exercise config conversion, not tokenization.
google::protobuf::Value jsonValue(const std::string &json) {
  google::protobuf::Value value;
  BOOST_REQUIRE(google::protobuf::util::JsonStringToMessage(json, &value).ok());
  return value;
}

auto jsonOperands(const std::string &json) {
  auto values = jsonValue(json);
  BOOST_REQUIRE(values.has_list_value());
  std::vector<Operand> operands;
  for (const auto &value : values.list_value().values()) {
    operands.emplace_back(value);
  }
  return operands;
}

void checkConfig(Env &env, const std::vector<Operand> &operands, const std::string &expected) {
  auto actual = toConfig(env, operands);
  BOOST_REQUIRE_MESSAGE(actual.has_value(), "toConfig returned an error");
  auto value = jsonValue(expected);
  BOOST_REQUIRE(value.has_struct_value());
  std::string difference;
  google::protobuf::util::MessageDifferencer differencer;
  differencer.ReportDifferencesToString(&difference);
  bool equal = differencer.Compare(*actual, value.struct_value());
  BOOST_CHECK_MESSAGE(equal, difference);
}

void checkArgs(const std::string &config, std::vector<std::string> expected,
               bool unordered = false) {
  auto value = jsonValue(config);
  BOOST_REQUIRE(value.has_struct_value());
  auto actual = toArgs(value.struct_value());
  if (unordered) {
    for (const auto &arg : expected) {
      if (arg.starts_with("--") && arg.ends_with("=false")) {
        const auto negated = "--no-" + arg.substr(2, arg.size() - 8);
        std::ranges::replace(actual, negated, arg);
      }
    }
    std::ranges::sort(actual);
    std::ranges::sort(expected);
  }
  BOOST_TEST(actual == expected, boost::test_tools::per_element());
}

}  // namespace

BOOST_AUTO_TEST_SUITE(config_test)

BOOST_AUTO_TEST_CASE(to_config_records_strings_and_positionals) {
  // Expected records follow README: strings descend into subcommands; other
  // primitives stay positional at the current level. The generated "@" field
  // is present only when its positional list is non-empty.
  struct Case {
    const char *name;
    const char *operands;
    const char *expected;
  };
  const Case cases[] = {
      {"empty", "[]", "{}"},
      {"empty records", "[{}, {}]", "{}"},
      {"flags", R"([{"verbose":true}, {"a":true,"b":true}, {"c":false}])",
       R"({"verbose":true,"a":true,"b":true,"c":false})"},
      {"option values", R"([{"count":42}, {"name":"Cedric"}])",
       R"({"count":42,"name":"Cedric"})"},
      {"merge disjoint records", R"([{"a":1}, {"b":2}])", R"({"a":1,"b":2})"},
      // Struct fields are protobuf map entries: the incoming entry replaces an
      // existing entry, including a nested record or list stored as its Value.
      {"replace scalar", R"([{"a":1}, {"a":2}])", R"({"a":2})"},
      {"replace nested map value", R"([{"x":{"a":1}}, {"x":{"b":2}}])",
       R"({"x":{"b":2}})"},
      {"replace list map value", R"([{"x":[1,2]}, {"x":[3]}])", R"({"x":[3]})"},
      {"replace kind of map value", R"([{"x":1}, {"x":{"a":2}}])",
       R"({"x":{"a":2}})"},
      {"one subcommand", R"(["subcommand"])", R"({"subcommand":{}})"},
      {"subcommand options", R"([{"verbose":true}, "subcommand", {"count":42}])",
       R"({"verbose":true,"subcommand":{"count":42}})"},
      {"nested subcommands", R"(["remote", {"verbose":true}, "add", {"force":false}])",
       R"({"remote":{"verbose":true,"add":{"force":false}}})"},
      {"repeated subcommand name", R"(["a", "a", {"x":1}])",
       R"({"a":{"a":{"x":1}}})"},
      {"literal string keys", R"(["two words", "a.b", {"x":1}])",
       R"({"two words":{"a.b":{"x":1}}})"},
      {"empty string key", R"(["", {"x":1}])", R"({"":{"x":1}})"},
      {"non-string primitives", R"([42, -2.5, true, false, null])",
       R"({"@":[42,-2.5,true,false,null]})"},
      {"primitives do not change merge target", R"([42, {"verbose":true}, false, {"x":1}])",
       R"({"@":[42,false],"verbose":true,"x":1})"},
      {"positionals belong to current subcommand", R"([1, "sub", 2, {"x":3}])",
       R"({"@":[1],"sub":{"@":[2],"x":3}})"},
      {"sentinel is retained", R"(["--"])", R"({"@":["--"]})"},
      {"strings after sentinel are positional", R"(["--", "foo", "bar"])",
       R"({"@":["--","foo","bar"]})"},
      {"sentinel applies to every subsequent value",
       R"(["--", {"foo":true}, 42, false, null, [1,2], "--", "tail"])",
       R"({"@":["--",{"foo":true},42,false,null,[1,2],"--","tail"]})"},
      {"sentinel within subcommand", R"(["sub", 42, "--", "foo", {"x":1}])",
       R"({"sub":{"@":[42,"--","foo",{"x":1}]}})"},
      {"README example (no-c is boolean false)",
       R"([{"verbose":true},{"a":true,"b":true},"subcommand",42,{"c":false},{"long":1234},"--","foo"])",
       R"({"verbose":true,"a":true,"b":true,"subcommand":{"@":[42,"--","foo"],"c":false,"long":1234}})"},
  };
  TestEnv env;
  for (const auto &test : cases) {
    BOOST_TEST_CONTEXT(test.name) {
      checkConfig(env, jsonOperands(test.operands), test.expected);
    }
  }
}

BOOST_AUTO_TEST_CASE(to_config_omits_empty_positional_fields_at_each_level) {
  TestEnv env;
  BOOST_TEST_CONTEXT("no arguments: no root @") {
    checkConfig(env, {}, "{}");
  }
  BOOST_TEST_CONTEXT("options only: no root @") {
    checkConfig(env, jsonOperands(R"([{"verbose":true}])"), R"({"verbose":true})");
  }
  BOOST_TEST_CONTEXT("no positionals at any subcommand level") {
    checkConfig(env, jsonOperands(R"(["remote", "add", {"force":true}])"),
                R"({"remote":{"add":{"force":true}}})");
  }
  BOOST_TEST_CONTEXT("root positionals do not add an empty child @") {
    checkConfig(env, jsonOperands(R"([0, "sub"])"), R"({"@":[0],"sub":{}})");
  }
  BOOST_TEST_CONTEXT("child positionals do not add an empty root @") {
    checkConfig(env, jsonOperands(R"(["sub", 0, false, null])"),
                R"({"sub":{"@":[0,false,null]}})");
  }
}

BOOST_AUTO_TEST_CASE(to_config_consumes_stream_arguments) {
  TestEnv env;
  auto operands = jsonOperands(R"([{"verbose":true}])");
  const auto values = jsonOperands(R"(["sub",42,{"count":2}])");
  std::vector<Result<Value>> results;
  for (const auto &operand : values) {
    results.emplace_back(std::get<google::protobuf::Value>(operand));
  }
  operands.emplace_back(Stream(ranges::views::all(results)));
  checkConfig(env, operands, R"({"verbose":true,"sub":{"@":[42],"count":2}})");
}

BOOST_AUTO_TEST_CASE(to_config_empty_stream_has_no_effect) {
  TestEnv env;
  auto operands = jsonOperands(R"([{"x":1}])");
  operands.emplace_back(Stream{});
  checkConfig(env, operands, R"({"x":1})");
}

BOOST_AUTO_TEST_CASE(to_config_sentinel_state_crosses_stream_boundaries) {
  TestEnv env;
  auto operands = jsonOperands(R"(["sub", "--"])");
  std::vector<Result<Value>> results = {Value(jsonValue(R"("foo")")),
                                        Value(jsonValue(R"({"x":1})"))};
  operands.emplace_back(Stream(ranges::views::all(results)));
  operands.emplace_back(jsonValue(R"("tail")"));
  checkConfig(env, operands, R"({"sub":{"@":["--","foo",{"x":1},"tail"]}})");
}

BOOST_AUTO_TEST_CASE(to_config_consumes_environment_stream_arguments) {
  struct ConfigEnv final : TestEnv {
    EnvStream getEnv(StreamRef ref) const override {
      if (ref.name != "ARGS") {
        return {};
      }
      return []() -> Stream {
        return ranges::yield(Value(jsonValue(R"({"verbose":true})")));
      };
    }
  } env;
  checkConfig(env, {StreamRef{"ARGS"}}, R"({"verbose":true})");
}

BOOST_AUTO_TEST_CASE(to_args_empty) {
  checkArgs("{}", {});
  checkArgs(R"({"@":[]})", {});
}

BOOST_AUTO_TEST_CASE(to_args_flags_and_assignments) {
  struct Case {
    const char *name;
    const char *config;
    std::vector<std::string> expected;
  };
  const Case cases[] = {
      {"short true flag", R"({"a":true})", {"-a"}},
      {"long true flag", R"({"verbose":true})", {"--verbose"}},
      {"number assignment", R"({"long":1234})", {"--long=1234"}},
      {"negative fraction", R"({"offset":-2.5})", {"--offset=-2.5"}},
      {"string assignment", R"({"name":"Cedric"})", {"--name=Cedric"}},
      {"empty string assignment", R"({"name":""})", {"--name="}},
      {"string argument is not shell quoted", R"({"name":"two words"})", {"--name=two words"}},
      {"independent flags", R"({"verbose":true,"a":true,"b":true})",
       {"--verbose", "-a", "-b"}},
  };
  for (const auto &test : cases) {
    BOOST_TEST_CONTEXT(test.name) {
      checkArgs(test.config, test.expected, true);
    }
  }
}

BOOST_AUTO_TEST_CASE(to_args_false_flags) {
  // The docs define --no-key but also support assignments. Either spelling
  // carries the same boolean value; do not require a canonical spelling.
  for (const std::string key : {"c", "verbose"}) {
    BOOST_TEST_CONTEXT(key) {
      auto value = jsonValue("{\"" + key + "\":false}");
      const auto actual = toArgs(value.struct_value());
      const std::vector<std::string> negated = {"--no-" + key};
      const std::vector<std::string> assigned = {"--" + key + "=false"};
      BOOST_CHECK(actual == negated || actual == assigned);
    }
  }
}

BOOST_AUTO_TEST_CASE(to_args_positionals_preserve_order_and_string_contents) {
  checkArgs(R"({"@":[42,-2.5,true,false,null]})", {"42", "-2.5", "true", "false", "null"});
  checkArgs(R"({"@":["--","foo","two words","","--flag","a\"b","雪"]})",
            {"--", "foo", "two words", "", "--flag", "a\"b", "雪"});
}

BOOST_AUTO_TEST_CASE(to_args_subcommands) {
  checkArgs(R"({"subcommand":{}})", {"subcommand"});
  checkArgs(R"({"subcommand":{"long":1234}})", {"subcommand", "--long=1234"});
  checkArgs(R"({"remote":{"add":{"force":true}}})", {"remote", "add", "--force"});
  checkArgs(R"({"@":[1],"sub":{"@":[2]}})", {"1", "sub", "2"});
  checkArgs(R"({"subcommand":{"@":[42,"--","foo"]}})",
            {"subcommand", "42", "--", "foo"});
}

BOOST_AUTO_TEST_CASE(to_args_readme_example_tokens) {
  // Independent option ordering and splitting -ab into -a -b do not change
  // argument meaning. Subcommand/positional ordering is tested above.
  checkArgs(
      R"({"verbose":true,"a":true,"b":true,"subcommand":{"@":[42,"--","foo"],"c":false,"long":1234}})",
      {"--verbose", "-a", "-b", "subcommand", "42", "--c=false", "--long=1234", "--", "foo"},
      true);
}

BOOST_AUTO_TEST_SUITE_END()
