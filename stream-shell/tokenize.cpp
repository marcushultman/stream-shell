#include "tokenize.h"

#include <variant>
#include "util/to_string_view.h"
#include "util/trim.h"

namespace {

using namespace std::string_view_literals;

struct Tokenizer {
  Tokenizer(std::string_view input) { !input.empty() && (*this)(Init(), input[0]); }

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
    } else if (op.first == '-' && !std::isdigit(c)) {
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
  bool operator()(char _, char b) { return std::visit(*this, _type, std::variant<char>{b}); }

 private:
  Type _type;
};

}  // namespace

auto tokenize(std::string_view input) -> ranges::any_view<std::string_view> {
  auto tokenizer = std::make_shared<Tokenizer>(input);
  return input | ranges::views::chunk_by([tokenizer](auto... s) { return (*tokenizer)(s...); }) |
         ranges::views::transform([](auto &&s) { return trim(toStringView(s)); }) |
         ranges::views::transform([](std::string_view &&s) -> std::vector<std::string_view> {
           if (auto op = s.find(".."); op != std::string_view::npos) {
             return {s.substr(0, op), s.substr(op, 2), s.substr(op + 2)};
           }
           return {s};
         }) |
         ranges::views::join | ranges::views::filter([](const auto &s) { return !s.empty(); });
}
