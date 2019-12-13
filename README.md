# complex_fmt_scn
playground for std::complex fmt / scn implementations

# Motivation

The advent of `{fmt}`, `scnlib` and chevron-free type-safe io offers
the opportunity to update c++'s default representation of complex
numbers.  A notation such as `(3+4i)` that includes the imaginary unit
is more friendly, and easier to use than the current iostreams
convention of displaying complex numbers as an ordered pair separated
by a comma: `(3,4)`.

We propose the imaginary-unit notation to be preferrable to the
ordered-pair notation for a number of reasons: it is easier for humans
accustomed to mathematics to read, it is unambiguously an object with
complex type (whereas an ordered pair is just an ordered pair), it
aligns with the c++14 `std::literals::complex_literals` notation, it
only adds one extra character symbol per object (the imaginary unit
specifier, if even that) rather than the three of iostreams (the
opening and closing parens and separating comma), and it allows for
omission of one of the parts if it can be determined by its omission
(is zero - more on that below).

As an added bonus, removal of the comma from complex number formatting
also removes the current possibility in iostreams of silent generation
of ambiguous output, which could happen, ie, when the locale's decimal
separator is set to comma.

# Considerations

Undertaking this fundamental change in how a standard library object
is represented textually should be done carefully so that, while
adding new functionality, the existing widely-used implementation is
faithfully replicated.  The following points are considered:

## Numeric form

The question of how to represent the underlying `std::complex<T>`\'s
`value_type` (`T`) is delegated to `formatter<T>` for that type.
Special alignment and fill rules apply for `T` \$ \in \$ {`float`,
`double`, `long double`}, but other custom value types are
accomodated. This is done by forwarding a designated portion of the
formatting specification to `T`'s formatter.

Although the standard does not specify behavior of `std::complex<T>`
for other types, it is not un-common to use a type for `T` which
provides other functionality such as extended precision or automatic
differentiation.  The formatting specification should therefore be
hierarchical, so that an arbitrary type `T` can be properly and
automatically formatted.

## Imaginary unit

Eliminating the separating comma could be done by simply specifying
that either a `+` or `-` should come between two numbers, and the
second number is imaginary, but then both parts must always be
printed.  Alternately, as is commonly done in mathematics, an
*imaginary unit* is appended (or prepended) to the imaginary numerical
part.  This allows omission of one of the parts, as an imaginary part
can be distinguished from a real part.

## Dropping real part

Because the complex number is always a pair, it is not necessary to
print the entire number if one of the parts is identical to a known
quantity: typically (positive) zero.  Either the real or the imaginary
part can be omitted if it satisfies this, although clearly not both.

Should a part be dropped?

Which part should be dropped?

What are the conditions under which real part can be dropped?

### Bijection

In introducing a new conversion format, we should assure that in the
default format a round-trip conversion produces the bit-wise exact
object, in as much as the `formatter<T>` allows.

Of important note is that in the implementation, deciding whether or
not to print the real part of a complex number, a simple comparison
with zero is USUALLY NOT SUFFICIENT.  While C++ does not specify the
underlying floating-point support, the decision to omit a real part
should be made based on whether the omitted real part is binary
equivalent to `T(0)`, not whether it compares equal.

This nuance is demonstrated by the result of `sqrt(-1. + 0i)` vs
`sqrt(-1. - 0i)`.

## Parentheses

Should parentheses be mandatory?

Are parentheses always neccesary to unambiguously specify a complex
number?

Do mandatory parentheses significantly improve ease or speed of
complex number parsing?

If parentheses are not mandatory, when should they be omitted?

## Backwards Compatibility

To maintain backward compatibility we include an easy-to-use format
specifier that exactly reproduces the legacy iostreams output format.

The `ios` specifiers that affect complex number output are `precision`
and `width`.  In the compatibilty format, the values should produce
the same output (modulo locale) that iostreams would produce.

## Parsing

Should parsing accept old or new complex format?



# Survey of other languages

The following programming languages/environments have similarly chosen
to use an imaginary-unit notation as their default: python, julia, R,
matlab, mathematica, go, .  These languages do all have round-trip
conversion from complex -> text -> complex, IF you know the type of
the data, but because some of them drop the complex part in their
textual output when the complex part is zero (or even negative zero!
yikes!)  the

Haskell `a :+ b`, this choice does not need much commentary, it is
quite unique.

idris

c# (does not, but the doc page for complex includes (only) example code for creating an appropriate fomatter)


# Proposed Wording

# Acknowledgements

# References

