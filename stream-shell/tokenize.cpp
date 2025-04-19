#include "tokenize.h"

#include <variant>
#include "util/trim.h"

namespace {

using namespace std::string_view_literals;

struct Tokenizer {
  struct Init {};
  struct Word {};
  struct Operator {
    char first = 0;
  };
  struct Number {
    enum Base { kUnknown, kDec, kHex } base = {};
  };
  struct String {
    char type = 0;
  };
  struct StreamRef {};
  using Type = std::variant<Init, Word, Operator, Number, String, StreamRef>;

  bool operator()(const Init &, char c) {
    if (ranges::contains("\"'`"sv, c)) {
      _type = String(c);
    } else if (ranges::contains("!&%+-*/<=>|"sv, c)) {
      _type = Operator(c);
    } else if (std::isdigit(c)) {
      _type = Number(c == '0' ? Number::kUnknown : Number::kDec);
    } else if (c == '$') {
      _type = StreamRef();
    } else if (std::isspace(c) || ranges::contains("(){}[]\"'`;:,"sv, c)) {
      _type = Init();
    } else {
      _type = Word();
    }
    return false;
  }

  bool operator()(const Word &, char c) {
    if (std::isspace(c) || ranges::contains("(){}[]\"'`"sv, c)) {
      return (*this)(Init(), c);
    }
    _type = Word();
    return true;
  }

  bool operator()(Operator &op, char c) {
    constexpr auto ops = std::array{"<=", "==", ">=", "!=", "&&", "||", "->"};
    if (op.first && ranges::contains(ops, std::string{op.first, c})) {
      op.first = 0;
      return true;
    } else if (ranges::contains("-/"sv, op.first) && !std::isdigit(c)) {
      return (*this)(Word(), c);
    }
    return (*this)(Init(), c);
  }

  bool operator()(Number &num, char c) {
    switch (num.base) {
      case Number::kUnknown:
        if (c == 'x') {
          num.base = Number::kHex;
          return true;
        }
        [[fallthrough]];
      case Number::kDec:
        if (std::isdigit(c) || c == '.') return true;
        break;
      case Number::kHex:
        if (std::isxdigit(c)) return true;
        break;
    }
    return (*this)(Init(), c);
  }

  bool operator()(String &str, char c) {
    if (!str.type) {
      return (*this)(Init(), c);
    } else if (c == str.type) {
      str.type = 0;
    }
    return true;
  }
  bool operator()(StreamRef &, char c) {
    if (!std::isalnum(c) && !ranges::contains("-_"sv, c)) {
      return (*this)(Init(), c);
    }
    _type = StreamRef();
    return true;
  }

 public:
  bool operator()(auto t1, auto t2) {
    auto [i, a] = t1;
    if (!i) {
      (*this)(Init(), a);
    }
    auto [_, b] = t2;
    return std::visit(*this, _type, std::variant<char>{b});
  }

 private:
  Type _type;
};

using Token = ranges::any_view<const char, ranges::category::bidirectional>;

}  // namespace

auto tokenize(Token input) -> ranges::any_view<Token> {
  auto tokenizer = std::make_shared<Tokenizer>();
  return ranges::views::enumerate(input) |
         ranges::views::chunk_by([tokenizer](auto... t) { return (*tokenizer)(t...); }) |
         ranges::views::transform([](auto &&t) { return t | ranges::views::values; }) |
         ranges::views::transform([](auto &&s) { return trim(std::forward<decltype(s)>(s)); }) |
         ranges::views::for_each([](auto &&s) {
           auto head = Token(s), tail = Token();
           if (!ranges::empty(s) && ranges::back(s) == ';') {
             head = trim(s, 0, 1);
             tail = trim(s, ranges::distance(s) - 1);
           }
           return ranges::views::concat(ranges::yield(head), ranges::yield(tail));
         }) |
         ranges::views::for_each([](auto &&s) -> ranges::any_view<Token> {
           auto [db, de] = ranges::search(s, ".."sv);
           if (db == ranges::end(s)) {
             return ranges::yield(s);
           }
           return ranges::views::concat(
               ranges::yield(Token(ranges::make_subrange(ranges::begin(s), db))),
               ranges::yield(".."sv),
               ranges::yield(Token(ranges::make_subrange(de, ranges::end(s)))));
         }) |
         ranges::views::filter([](auto &&s) { return !ranges::empty(s); });
}
