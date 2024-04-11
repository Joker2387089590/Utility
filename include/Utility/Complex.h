#pragma once
#include <cmath>
#include <complex>
#if __has_include(<numbers>)
#include <numbers>
namespace Numbers = std::numbers;
#else
namespace Numbers
{
inline constexpr double pi = 3.14159265358979323846264338327950218;
}
#endif

namespace Complexs
{
using Real = double;
struct Imag;
struct Complex;

struct Imag
{
	constexpr explicit Imag(double v) noexcept : value(v) {}
	constexpr Imag& operator=(double v) noexcept { value = v; return *this; }

	constexpr Imag(const Imag&) noexcept = default;
	constexpr Imag(Imag&&) noexcept = default;
	constexpr Imag& operator=(const Imag&) & noexcept = default;
	constexpr Imag& operator=(Imag&&) & noexcept = default;
	~Imag() noexcept = default;

	constexpr operator Complex() const noexcept;
	constexpr const double& operator()() const noexcept { return value; }

	constexpr Imag operator-() const noexcept { return Imag(-value); }
	constexpr Imag operator~() const noexcept { return Imag(-value); }

	double value;
};

struct Complex
{
	constexpr Complex(Real r, double i = 0) noexcept : real(r), imag(i) {}
	constexpr Complex(Real r, Imag i) noexcept : real{r}, imag(i) {}
	constexpr Complex(Imag i) noexcept : real(0.), imag(i) {}

	constexpr Complex(const Complex&) noexcept = default;
	constexpr Complex(Complex&&) noexcept = default;
	constexpr Complex& operator=(const Complex&) & noexcept = default;
	constexpr Complex& operator=(Complex&&) & noexcept = default;
	~Complex() noexcept = default;

	constexpr Complex operator-() const noexcept { return {-real, -imag}; }
	constexpr Complex operator~() const noexcept { return {real, -imag}; }
	constexpr double abs() const noexcept { return std::sqrt(real * real + imag() * imag()); }

	constexpr std::complex<double> toStd() const { return { real, imag.value }; }

	Real real;
	Imag imag;
};

struct PolarComplex
{
	constexpr PolarComplex(Real r, Real theta) : r(r), theta(theta) {}
	constexpr PolarComplex(Complex c) : r(c.abs()), theta(std::atan2(c.imag(), c.real)) {}

	constexpr PolarComplex(const PolarComplex&) noexcept = default;
	constexpr PolarComplex(PolarComplex&&) noexcept = default;
	constexpr PolarComplex& operator=(const PolarComplex&) noexcept = default;
	constexpr PolarComplex& operator=(PolarComplex&&) noexcept = default;
	~PolarComplex() noexcept = default;

	constexpr operator Complex() const noexcept { return {r * std::cos(theta), Imag(r * std::sin(theta))}; }

	constexpr PolarComplex operator-() const noexcept { return {r, theta + Numbers::pi}; }
	constexpr PolarComplex operator~() const noexcept { return {r, -theta}; }
	constexpr double abs() const noexcept { return std::abs(r); }

	Real r;
	Real theta;
};

inline constexpr Imag::operator Complex() const noexcept { return {0, value}; }

inline constexpr Imag operator""_i(long double imag) noexcept { return Imag(imag); }
inline constexpr Imag operator""_i(unsigned long long int imag) noexcept { return Imag(imag); }

inline constexpr Imag    operator+(Imag a, Imag b)       noexcept { return Imag{a.value + b.value}; }
inline constexpr Complex operator+(Real r, Imag i)       noexcept { return {r, i}; }
inline constexpr Complex operator+(Imag i, Real r)       noexcept { return {r, i}; }
inline constexpr Complex operator+(Complex c, Imag i)    noexcept { return {c.real, c.imag + i}; }
inline constexpr Complex operator+(Imag i, Complex c)    noexcept { return {c.real, c.imag + i}; }
inline constexpr Complex operator+(Real r, Complex c)    noexcept { return {c.real + r, c.imag}; }
inline constexpr Complex operator+(Complex c, Real r)    noexcept { return {c.real + r, c.imag}; }
inline constexpr Complex operator+(Complex a, Complex b) noexcept { return {a.real + b.real, a.imag + b.imag}; }

inline constexpr Imag    operator-(Imag a, Imag b)       noexcept { return Imag{a.value - b.value}; }
inline constexpr Complex operator-(Real r, Imag i)       noexcept { return {r, -i}; }
inline constexpr Complex operator-(Imag i, Real r)       noexcept { return {-r, i}; }
inline constexpr Complex operator-(Complex c, Imag i)    noexcept { return {c.real, c.imag - i}; }
inline constexpr Complex operator-(Imag i, Complex c)    noexcept { return -(c - i); }
inline constexpr Complex operator-(Real r, Complex c)    noexcept { return r + -c; }
inline constexpr Complex operator-(Complex c, Real r)    noexcept { return {c.real - r, c.imag }; }
inline constexpr Complex operator-(Complex a, Complex b) noexcept { return {a.real - b.real, a.imag - b.imag}; }

inline constexpr Real    operator*(Imag a, Imag b)       noexcept { return -a.value * b.value; }
inline constexpr Imag    operator*(Imag i, Real r)       noexcept { return Imag(i.value * r); }
inline constexpr Imag    operator*(Real r, Imag i)       noexcept { return Imag(i.value * r); }
inline constexpr Complex operator*(Complex c, Imag i)    noexcept { return c.real * i + c.imag * i; }
inline constexpr Complex operator*(Imag i, Complex c)    noexcept { return c.real * i + c.imag * i; }
inline constexpr Complex operator*(Complex c, Real r)    noexcept { return c.real * r + c.imag * r; }
inline constexpr Complex operator*(Real r, Complex c)    noexcept { return c.real * r + c.imag * r; }
inline constexpr Complex operator*(Complex a, Complex b) noexcept { return {a.real * b.real + a.imag * b.imag, a.real * b.imag + a.imag * b.real}; }

inline constexpr Real    operator/(Imag a, Imag b)       noexcept { return a.value / b.value; }
inline constexpr Imag    operator/(Imag i, Real r)       noexcept { return Imag(i.value / r); }
inline constexpr Imag    operator/(Real r, Imag i)       noexcept { return Imag(r / i.value); }
inline constexpr Complex operator/(Complex c, Real r)    noexcept { return { c.real / r, c.imag / r }; }
inline constexpr Complex operator/(Real r, Complex c)    noexcept { return (r * ~c) / (c.real * c.real + c.imag() * c.imag()); }
inline constexpr Complex operator/(Complex c, Imag i)    noexcept { return c * ~i / (i.value * i.value); }
inline constexpr Complex operator/(Imag i, Complex c)    noexcept { return i * ~c / (c.real * c.real + c.imag() * c.imag()); }
inline constexpr Complex operator/(Complex a, Complex b) noexcept { return a * ~b / (b.real * b.real + b.imag() * b.imag()); }

inline constexpr Imag&    operator+=(Imag& a, Imag b)       noexcept { return a = a + b; }
inline constexpr Complex& operator+=(Complex& c, Imag i)    noexcept { return c = c + i; }
inline constexpr Complex& operator+=(Complex& c, Real r)    noexcept { return c = c + r; }
inline constexpr Complex& operator+=(Complex& c, Complex o) noexcept { return c = c + o; }

inline constexpr Imag&    operator-=(Imag& a, Imag b)       noexcept { return a = a - b; }
inline constexpr Complex& operator-=(Complex& c, Imag i)    noexcept { return c = c - i; }
inline constexpr Complex& operator-=(Complex& c, Real r)    noexcept { return c = c - r; }
inline constexpr Complex& operator-=(Complex& c, Complex o) noexcept { return c = c - o; }

inline constexpr Imag&    operator*=(Imag& a, Real b)       noexcept { return a = a * b; }
inline constexpr Complex& operator*=(Complex& c, Imag i)    noexcept { return c = c * i; }
inline constexpr Complex& operator*=(Complex& c, Real r)    noexcept { return c = c * r; }
inline constexpr Complex& operator*=(Complex& c, Complex o) noexcept { return c = c * o; }

inline constexpr Imag&    operator/=(Imag& a, Real b)       noexcept { return a = a / b; }
inline constexpr Complex& operator/=(Complex& c, Imag i)    noexcept { return c = c / i; }
inline constexpr Complex& operator/=(Complex& c, Real r)    noexcept { return c = c / r; }
inline constexpr Complex& operator/=(Complex& c, Complex o) noexcept { return c = c / o; }

template<typename I, std::enable_if_t<std::is_integral_v<I> && !std::is_same_v<I, bool>, int> = 0>
inline constexpr Complex operator,(I r, Imag i) noexcept { return {double(r), i}; }
}
