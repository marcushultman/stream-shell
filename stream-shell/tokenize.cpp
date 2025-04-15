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
  using Type = std::variant<Init, Word, Operator, Number, String>;

  bool operator()(const Init &, char c) {
    if (ranges::contains("\"'`"sv, c)) {
      _type = String(c);
      return false;
    } else if (ranges::contains("!&%+-*/<=>|"sv, c)) {
      _type = Operator(c);
      return false;
    } else if (std::isdigit(c)) {
      _type = Number(c == '0' ? Number::kUnknown : Number::kDec);
      return false;
    }
    return (*this)(Word(), c) && false;
  }

  bool operator()(const Word &, char c) {
    if (ranges::contains("(){}[]\"'`;:<=>,"sv, c) || std::isspace(c)) {
      _type = Init();
      return false;
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

 public:
  bool operator()(auto t1, auto t2) {
    auto [a, i] = t1;
    if (!i) {
      (*this)(Init(), a);
    }
    auto [b, _] = t2;
    return std::visit(*this, _type, std::variant<char>{b});
  }

 private:
  Type _type;
};

}  // namespace

auto tokenize(ranges::any_view<const char, ranges::category::bidirectional> input)
    -> ranges::any_view<ranges::any_view<const char, ranges::category::bidirectional>> {
  auto tokenizer = std::make_shared<Tokenizer>();
  return ranges::views::zip(input, ranges::views::iota(0)) |
         ranges::views::chunk_by([tokenizer](auto... t) { return (*tokenizer)(t...); }) |
         ranges::views::transform([](auto &&t) { return t | ranges::views::keys; }) |
         ranges::views::transform([](ranges::bidirectional_range auto &&s) {
           return trim(std::forward<decltype(s)>(s));
         }) |
         ranges::views::for_each(
             [](ranges::bidirectional_range auto &&s)
                 -> ranges::any_view<
                     ranges::any_view<const char, ranges::category::bidirectional>> {
               auto [db, de] = ranges::search(s, ".."sv);
               if (db == ranges::end(s)) {
                 return ranges::yield(s);
               }
               ranges::any_view<const char, ranges::category::bidirectional> a =
                   ranges::subrange<decltype(db), decltype(db)>(ranges::begin(s), db);
               ranges::any_view<const char, ranges::category::bidirectional> b = ".."sv;
               ranges::any_view<const char, ranges::category::bidirectional> c =
                   ranges::subrange<decltype(db), decltype(db)>(de, ranges::end(s));

               return ranges::views::concat(
                   ranges::views::single(a), ranges::views::single(b), ranges::views::single(c));
             }) |
         ranges::views::filter([](auto &&s) { return !ranges::empty(s); });
}
