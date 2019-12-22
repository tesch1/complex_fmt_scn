#include <complex>
#include <iostream>

// The cfmt library version in the form major * 10000 + minor * 100 + patch.
#define CFMT_VERSION 60101

/// for complex<T>:
///
/// complex-fmt-spec  := [ [real-fmt-spec] join-spec ] [real-and-or-imag-fmt-spec [imag-unit] ] [width]
/// real-fmt-spec     := '{' format-spec-for-T '}'
/// join-spec         := '+' | ','
/// real-fmt-spec     := '{' format-spec-for-T '}'
/// imag-unit         := '[^{}0-9]'*
///
template <typename T, typename Char>
struct fmt::formatter<std::complex<T>,Char>
{
  typedef fmt::formatter<T,Char> base;
  base f_real_, f_imag_;
  bool _no_neg_zero = false;
  char join_;
  string_view im_str_;

  FMT_CONSTEXPR auto parse(format_parse_context & ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    bool got_real = false;
    im_str_ = "i";
    join_ = 0;

    // embedded real format
    if (*it == '{') {
      got_real = true;
      auto real_begin = it;
      ctx.advance_to(++it);
      it = f_real_.parse(ctx);
      ++it;
    }

    // type of join
    switch (*it) {
    case '+': join_ = '+'; ++it; break;
    case ',': join_ = ','; ++it; im_str_ = ""; break;
    default: break;
    }

    // embedded imag format
    if (*it == '{') {
      ctx.advance_to(++it);
      it = f_imag_.parse(ctx);
      assert(*it == '}');
      ++it;
      if (!got_real)
        f_real_ = f_imag_;
    }
    else {
      if (!got_real) {
        // in case the parse function needs to set anything up
        basic_string_view<Char> empty;
        format_parse_context dummy(empty);
        f_real_.parse(dummy);
      }
      f_imag_ = f_real_;
    }

    // capture imag part string, if any
    if (*it != '}' && join_ != ',') {
      auto im_end = it;
      while (im_end != end && *im_end != '}')
        ++im_end;
      im_str_ = string_view(it, im_end - it);
      it = im_end;
    }
    return it;
  }
  template <typename FormatCtx>
  auto format(const std::complex<T> & x, FormatCtx & ctx) -> decltype(ctx.out()) {
    if (join_ == ',') {
      format_to(ctx.out(), "(");
      f_real_.format(_no_neg_zero ? (x.real() + T(0)) : x.real(), ctx);
      format_to(ctx.out(), ",");
      f_imag_.format(x.imag(), ctx);
      return format_to(ctx.out(), ")");
    }
    else {
      bool drop_real = x.real() == 0 && !std::signbit(x.real());
      if (!drop_real) {
        format_to(ctx.out(), "(");
        f_real_.format(_no_neg_zero ? (x.real() + T(0)) : x.real(), ctx);
      }
      else
        format_to(ctx.out(), "{:{}}", "", 9);
      format_to(ctx.out(), "{}", std::signbit(x.imag()) ? "-" : "+");
      f_imag_.format(std::signbit(x.imag()) ? -x.imag() : x.imag(), ctx);
      format_to(ctx.out(), "{}", im_str_);
      if (!drop_real)
        format_to(ctx.out(), ")");
      return ctx.out();
    }
  }
};
