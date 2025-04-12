#define BOOST_TEST_MODULE tokenize_test
#include <boost/test/included/unit_test.hpp>

#include <boost/test/unit_test.hpp>
#include <range/v3/all.hpp>
#include "stream-shell/tokenize.h"

using namespace std::string_view_literals;

const auto kEach = boost::test_tools::per_element();

auto tokens(auto str) {
  return tokenize(str) | ranges::to<std::vector>();
}

using Tokens = decltype(tokens(""));

BOOST_AUTO_TEST_SUITE(tokenize_test)

BOOST_AUTO_TEST_CASE(tokenize_empty) {
  BOOST_TEST(tokens("").empty());
}

BOOST_AUTO_TEST_CASE(tokenize_numbers) {
  BOOST_TEST(tokens("1 2 3") == Tokens({"1", "2", "3"}), kEach);
  BOOST_TEST(tokens("1.2 23.45") == Tokens({"1.2", "23.45"}), kEach);
  BOOST_TEST(tokens("0xDe 0xAd 0xBE 0xaf") == Tokens({"0xDe", "0xAd", "0xBE", "0xaf"}), kEach);
  BOOST_TEST(tokens("0xABBA 42") == Tokens({"0xABBA", "42"}), kEach);
  BOOST_TEST(tokens("π ∞") == Tokens({"π", "∞"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_command) {
  BOOST_TEST(tokens("echo") == Tokens({"echo"}), kEach);
  BOOST_TEST(tokens("git status") == Tokens({"git", "status"}), kEach);
  BOOST_TEST(tokens("foo -b --baz") == Tokens({"foo", "-b", "--baz"}), kEach);
  BOOST_TEST(tokens("my_script.stsh") == Tokens({"my_script.stsh"}), kEach);
  BOOST_TEST(tokens("my-script.stsh") == Tokens({"my-script.stsh"}), kEach);
  BOOST_TEST(tokens("cd ~/some/dir") == Tokens({"cd", "~/some/dir"}), kEach);
  BOOST_TEST(tokens("foo -- 1 2") == Tokens({"foo", "--", "1", "2"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_stream_variable) {
  BOOST_TEST(tokens("$HOME") == Tokens({"$HOME"}), kEach);
  BOOST_TEST(tokens("$MY_VAR") == Tokens({"$MY_VAR"}), kEach);
  BOOST_TEST(tokens("$1234") == Tokens({"$1234"}), kEach);
  BOOST_TEST(tokens("$DASH-VAR") == Tokens({"$DASH-VAR"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_number_arithmetic) {
  BOOST_TEST(tokens("1 + 2") == Tokens({"1", "+", "2"}), kEach);
  BOOST_TEST(tokens("1 - 2") == Tokens({"1", "-", "2"}), kEach);
  BOOST_TEST(tokens("1 * 2") == Tokens({"1", "*", "2"}), kEach);
  BOOST_TEST(tokens("1 / 2") == Tokens({"1", "/", "2"}), kEach);
  BOOST_TEST(tokens("1 % 2") == Tokens({"1", "%", "2"}), kEach);
  BOOST_TEST(tokens("1+ 2") == Tokens({"1", "+", "2"}), kEach);
  BOOST_TEST(tokens("1 +2") == Tokens({"1", "+", "2"}), kEach);
  BOOST_TEST(tokens("1+2") == Tokens({"1", "+", "2"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_number_comp) {
  BOOST_TEST(tokens("1 < 2") == Tokens({"1", "<", "2"}), kEach);
  BOOST_TEST(tokens("1 > 2") == Tokens({"1", ">", "2"}), kEach);
  BOOST_TEST(tokens("1 == 2") == Tokens({"1", "==", "2"}), kEach);
  BOOST_TEST(tokens("1 != 2") == Tokens({"1", "!=", "2"}), kEach);
  BOOST_TEST(tokens("1 >= 2") == Tokens({"1", ">=", "2"}), kEach);
  BOOST_TEST(tokens("1 <= 2") == Tokens({"1", "<=", "2"}), kEach);
  BOOST_TEST(tokens("1== 2") == Tokens({"1", "==", "2"}), kEach);
  BOOST_TEST(tokens("1 ==2") == Tokens({"1", "==", "2"}), kEach);
  BOOST_TEST(tokens("1==2") == Tokens({"1", "==", "2"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_bool) {
  BOOST_TEST(tokens("true") == Tokens({"true"}), kEach);
  BOOST_TEST(tokens("true false") == Tokens({"true", "false"}), kEach);
  BOOST_TEST(tokens("true || false") == Tokens({"true", "||", "false"}), kEach);
  BOOST_TEST(tokens("true && false") == Tokens({"true", "&&", "false"}), kEach);
  BOOST_TEST(tokens("garbage |&&") == Tokens({"garbage", "|", "&&"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_string) {
  BOOST_TEST(tokens("\"foo's bar\"") == Tokens({"\"foo's bar\""}), kEach);
  BOOST_TEST(tokens("'foo \"bar\"'") == Tokens({"'foo \"bar\"'"}), kEach);
  BOOST_TEST(tokens("`just backtick`") == Tokens({"`just backtick`"}), kEach);

  BOOST_TEST(tokens("1 \"foo's bar\" baz") == Tokens({"1", "\"foo's bar\"", "baz"}), kEach);
  BOOST_TEST(tokens("2 'foo \"bar\"' baz") == Tokens({"2", "'foo \"bar\"'", "baz"}), kEach);
  BOOST_TEST(tokens("3 `just backtick` baz") == Tokens({"3", "`just backtick`", "baz"}), kEach);

  BOOST_TEST(tokens("`foo $BAR baz`") == Tokens({"`foo $BAR baz`"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_assign) {
  BOOST_TEST(tokens("FOO = 1") == Tokens({"FOO", "=", "1"}), kEach);
  BOOST_TEST(tokens("FOO =1") == Tokens({"FOO", "=", "1"}), kEach);
  BOOST_TEST(tokens("FOO= 1") == Tokens({"FOO", "=", "1"}), kEach);
  BOOST_TEST(tokens("FOO=1") == Tokens({"FOO", "=", "1"}), kEach);
}

BOOST_AUTO_TEST_CASE(tokenize_complex) {
  BOOST_TEST(
      tokens(
          R"cmd(1 2 3 | 1..3 | user.proto.Person { name: "Albert" } | 1 (-2 + 3) | git add (ls src) | { i -> $i * 2 }; now :[-1]; repeat "na" | tail 3 ; "hello" > hello; 1 2 3 > lines.log; now &; $HOME ; foo<bar )cmd") ==
          Tokens(
              {"1",         "2",         "3",      "|",          "1..3", "|", "user.proto.Person",
               "{",         "name",      ":",      "\"Albert\"", "}",    "|", "1",
               "(",         "-",         "2",      "+",          "3",    ")", "|",
               "git",       "add",       "(",      "ls",         "src",  ")", "|",
               "{",         "i",         "->",     "$i",         "*",    "2", "}",
               ";",         "now",       ":",      "[",          "-",    "1", "]",
               ";",         "repeat",    "\"na\"", "|",          "tail", "3", ";",
               "\"hello\"", ">",         "hello",  ";",          "1",    "2", "3",
               ">",         "lines.log", ";",      "now",        "&",    ";", "$HOME",
               ";",         "foo",       "<",      "bar"}),
      kEach);
}

BOOST_AUTO_TEST_SUITE_END()
