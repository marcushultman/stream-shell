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
    bool is_interpolating = false;
  };
  using Type = std::variant<Init, Word, Operator, Number, String>;

  bool operator()(const Init &, char c) {
    if (ranges::contains("\"'`"sv, c)) {
      _type = String(c);
      return false;
    } else if (ranges::contains("!&%+-/<=>|"sv, c)) {
      _type = Operator(c);
      return false;
    } else if (std::isdigit(c)) {
      _type = Number(c == '0' ? Number::kUnknown : Number::kDec);
      return false;
    }
    return (*this)(Word(), c) && false;
  }

  bool operator()(const Word &, char c) {
    if (ranges::contains("(){}[]\"';:>`,"sv, c) || std::isspace(c)) {
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
    } else if (op.first == '-') {
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
    } else if (str.type == '`' && c == '$') {
      str.is_interpolating = true;
      return false;
    } else if (str.is_interpolating && std::isspace(c)) {
      str.is_interpolating = false;
      return false;
    }
    return true;
  }

 public:
  bool operator()(char _, char b) { return std::visit(*this, _type, std::variant<char>{b}); }

 private:
  Type _type;
};

auto is_str(const std::string_view &s) {
  constexpr auto quotes = "\"'`"sv;
  return ranges::contains(quotes, s.front()) || ranges::contains(quotes, s.back());
}

}  // namespace

auto tokenize(std::string_view input) -> ranges::any_view<std::string_view> {
  auto tokenizer = std::make_shared<Tokenizer>(input);
  return input | ranges::views::chunk_by([tokenizer](auto... s) { return (*tokenizer)(s...); }) |
         ranges::views::transform([](auto &&s) { return toStringView(s); }) |
         ranges::views::transform([](auto &&s) { return is_str(s) ? s : trim(s); }) |
         ranges::views::filter([](const auto &s) { return !s.empty(); });
}
