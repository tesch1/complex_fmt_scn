#include <complex>
#include <fmt/format.h>
#include <cfmt/complex_fmt.h>

typedef std::complex<float> complexf;
typedef std::complex<double> complexd;
typedef std::complex<long double> complexld;

int main(int argc, char *argv[])
{
  complexd a(3,-4);
  fmt::print("{}\n", a);
  fmt::print("{:+}\n", a);
  fmt::print("{:+ }\n", a);
  fmt::print("{: +}\n", a);
  fmt::print("{:,}\n", a);
  fmt::print("{:+}\n", a);
  fmt::print("{:{}j}\n", a);
  fmt::print("{:+{.3}}\n", a);
  fmt::print("{:{6.6}+{3.3}}\n", a);
}
