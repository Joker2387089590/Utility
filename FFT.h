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
		in(fftw_malloc(sizeof(double) * N)),
		out(fftw_malloc(sizeof(Complex) * (N / 2 + 1))),
		p(fftw_plan_dft_r2c_1d(int(N),
							   static_cast<double*>(in),
							   static_cast<fftw_complex*>(out),
							   FFTW_ESTIMATE))
	{}

	~FFT()
	{
		fftw_destroy_plan(p);
		fftw_free(out);
		fftw_free(in);
	}

public:
	const double* input() const noexcept { return static_cast<double*>(in); }
	const Complex* output() const noexcept { return static_cast<Complex*>(out); }
    std::size_t size() const noexcept { return N; }

	const FFT& calc(const double* input) noexcept
	{
		std::copy(std::execution::par_unseq, input, input + N, this->input());
		fftw_execute(p);
		return *this;
	}

	template<template<typename...> typename Vec>
	Vec<double> abs() const noexcept
	{
        const int outSize = int(N / 2 + 1);
		Vec<double> vec(outSize);
		auto o = this->output();
		std::transform(std::execution::par_unseq, o, o + outSize,
					   vec.data(), [](const auto& d) noexcept { return std::abs(d); });
		return vec;
	}

private:
	double* input() noexcept { return static_cast<double*>(in); }

private:
	const std::size_t N;
	void* in;
	void* out;
	fftw_plan p;
};
