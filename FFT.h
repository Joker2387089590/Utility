#pragma once
#include <execution> // std::execution::par_unseq
#include <algorithm> // std::transform
#include <complex> // Need to be included before <fftw3.h>!
#include <fftw3.h>

class FFT
{
	using Complex = std::complex<double>;
public:
	FFT(std::size_t N) :
		N(N),
		in(fftw_malloc(sizeof(Complex) * N)),
		out(fftw_malloc(sizeof(Complex) * N)),
		p(fftw_plan_dft_1d(N,
						   static_cast<fftw_complex*>(in),
						   static_cast<fftw_complex*>(out),
						   FFTW_FORWARD, FFTW_PATIENT))
	{}

	~FFT()
	{
		fftw_destroy_plan(p);
		fftw_free(out);
		fftw_free(in);
	}

public:
	const Complex* input() const noexcept { return static_cast<Complex*>(in); }
	const Complex* output() const noexcept { return static_cast<Complex*>(out); }

	const FFT& calc(const double* input) noexcept
	{
		std::transform(std::execution::par_unseq, input, input + N,
					   this->input(), [](const double& i) noexcept { return Complex{i, 0}; });
		fftw_execute(p);
		return *this;
	}

	template<template<typename> typename Vec>
	Vec<double> abs() const noexcept
	{
		Vec<double> vec(N);
		auto o = this->output();
		std::transform(std::execution::par_unseq, o, o + N,
					   vec.data(), [](const auto& d) noexcept { return std::abs(d); });
		return vec;
	}

private:
	Complex* input() noexcept { return static_cast<Complex*>(in); }

private:
	const std::size_t N;
	void* in;
	void* out;
	fftw_plan p;
};
