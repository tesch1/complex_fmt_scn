// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <stdint.h>
#include <cctype>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstring>
#include <list>
#include <memory>
#include <string>
#include <complex>

// Check if fmt/format.h compiles with windows.h included before it.
#ifdef _WIN32
#  include <windows.h>
#endif

// Check if fmt/format.h compiles with the X11 index macro defined.
#define index(x, y) no nice things

#include "fmt/color.h"
#include "fmt/format.h"
#include "cfmt/complex_fmt.h"

#undef index

#include "gmock/gmock.h"

typedef std::complex<float> complexf;
typedef std::complex<double> complexd;
typedef std::complex<long double> complexld;

#undef ERROR
#undef min
#undef max
using fmt::internal::max_value;

TEST(WriterTest, WriteDouble) {
  auto min = std::numeric_limits<double>::min();
  auto max = max_value<double>();
  complexd a(min,max);
  EXPECT_EQ("2.2250738585072014e-308", fmt::format("{}", min));
  EXPECT_EQ("1.7976931348623157e+308", fmt::format("{}", max));
  EXPECT_EQ("1.7976931348623157e+308", fmt::format("{}", a));
}

