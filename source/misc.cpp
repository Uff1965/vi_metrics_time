// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "misc.h"

#if _WIN32
#	define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#	define NOMINMAX
#	include <Windows.h>
#	include <io.h> // for _isatty(...)
#else
#	include <unistd.h> // for isatty(...)
#endif

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <thread>
#include <iostream>
#include <vector>
#include <memory>

namespace ch = std::chrono;
using namespace std::literals;

namespace
{	constexpr auto operator""_ps(long double v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ps(unsigned long long v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ks(long double v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_ks(unsigned long long v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_Ms(long double v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Ms(unsigned long long v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Gs(long double v) noexcept { return ch::duration<double, std::giga>(v); };
	constexpr auto operator""_Gs(unsigned long long v) noexcept { return ch::duration<double, std::giga>(v); };
}

bool misc::operator < (misc::duration_t l, misc::duration_t r)
{	return l.count() < r.count() && to_string(l, 2, 1) != to_string(r, 2, 1);
}

double misc::round(double num, unsigned char prec, unsigned char dec)
{ // Rounding to 'dec' decimal place and no more than 'prec' significant symbols.
	assert(num >= 0 && prec > dec && prec <= 3 + dec);

	double result = num;
	if (num >= 0 && prec > dec && prec <= 3 + dec)
	{	auto power = static_cast<signed char>(std::floor(std::log10(num)));
		auto t = 1U + (3 + power % 3) % 3;
		if (prec > t)
		{	t += std::min(dec, static_cast<unsigned char>(prec - t));
		}
		else
		{	t = prec;
		}
		power -= t - 1;

		const auto factor = std::pow(10, -power);
		result = std::round(factor * (num * (1 + std::numeric_limits<decltype(num)>::epsilon()))) / factor;
	}

	return result;
}

#ifndef NDEBUG
const auto unit_test_round = []
{
	struct
	{
		int line_;
		double org_;
		double rnd_;
		unsigned char precision_ = 2;
		unsigned char dec_ = 1;
	}
	constexpr static samples[] =
	{
		{__LINE__, 0.0, 0.0, 1, 0},
		{__LINE__, 0.0, 0.0, 2, 1},
		{__LINE__, 0.0, 0.0, 4, 1},
		{__LINE__, 0.0, 0.0, 5, 2},

		{__LINE__, 1.23456789, 1.0, 1, 0},
		{__LINE__, 1.23456789, 1.2, 2, 1},
		{__LINE__, 1.23456789, 1.2, 4, 1},
		{__LINE__, 1.23456789, 1.23, 5, 2},

		{__LINE__, 123.456789, 100.0, 1, 0},
		{__LINE__, 123.456789, 120.0, 2, 1},
		{__LINE__, 123.456789, 123.5, 4, 1},
		{__LINE__, 123.456789, 123.46, 5, 2},

		{__LINE__, 12.3456789e3, 10.0e3, 1, 0},
		{__LINE__, 12.3456789e3, 12.0e3, 2, 1},
		{__LINE__, 12.3456789e3, 12.3e3, 4, 1},
		{__LINE__, 12.3456789e3, 12.35e3, 5, 2},

		{__LINE__, 0.123456789, 0.10, 1, 0},
		{__LINE__, 0.123456789, 0.12, 2, 1},
		{__LINE__, 0.123456789, 0.1235, 4, 1},
		{__LINE__, 0.123456789, 0.12346, 5, 2},

		{__LINE__, 0.00123456789, 0.001, 1, 0},
		{__LINE__, 0.00123456789, 0.0012, 2, 1},
		{__LINE__, 0.00123456789, 0.0012, 4, 1},
		{__LINE__, 0.00123456789, 0.00123, 5, 2},

		{__LINE__, 0.0123456789e-3, 0.010e-3, 1, 0},
		{__LINE__, 0.0123456789e-3, 0.012e-3, 2, 1},
		{__LINE__, 0.0123456789e-3, 0.0123e-3, 4, 1},
		{__LINE__, 0.0123456789e-3, 0.01235e-3, 5, 2},
	};

	for (auto& i : samples)
	{
		const auto rnd = misc::round(i.org_, i.precision_, i.dec_);
		assert(std::max(rnd, i.rnd_) * DBL_EPSILON >= std::abs(rnd - i.rnd_));
	}

	return 0;
}();
#endif // #ifndef NDEBUG

std::string misc::to_string(duration_t sec, unsigned char precision, unsigned char dec)
{	sec = misc::duration_t{ round(sec.count(), precision, dec) };

	struct { std::string_view suffix_; double factor_{}; } k;
	if (10_ps > sec) { k = { "ps"sv, 1.0 }; }
	else if (1ns > sec) { k = { "ps"sv, 1e12 }; }
	else if (1us > sec) { k = { "ns"sv, 1e9 }; }
	else if (1ms > sec) { k = { "us"sv, 1e6 }; }
	else if (1s > sec) { k = { "ms"sv, 1e3 }; }
	else if (1_ks > sec) { k = { "s "sv, 1e0 }; }
	else if (1_Ms > sec) { k = { "ks"sv, 1e-3 }; }
	else if (1'000_Ms > sec) { k = { "Ms"sv, 1e-6 }; }
	else { k = { "Gs"sv, 1e-9 }; }

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(dec) << sec.count() * k.factor_ << k.suffix_;
	return ss.str();
}

#ifndef NDEBUG
const auto unit_test_to_string = []
{
	struct
	{
		int line_;
		misc::duration_t sec_;
		std::string_view res_;
		unsigned char precision_{ 2 };
		unsigned char dec_{ 1 };
	}
	static constexpr samples[] =
	{
		{__LINE__, 0s, "0.0ps"},
		{__LINE__, 0.01234567891s, "12.346ms", 6, 3},
		{__LINE__, 0.01234567891s, "12.35ms", 5, 2},
		{__LINE__, 0.1_ps, "0.0ps"},
		{__LINE__, 1_ps, "0.0ps"}, // The lower limit of accuracy is 10ps.
		{__LINE__, 10.01ms, "10.0ms"},
		{__LINE__, 10.1ms, "10.0ms"},
		{__LINE__, 10_ps, "10.0ps"},
		{__LINE__, 100_ps, "100.0ps"},
		{__LINE__, 100ms, "100.0ms"},
		{__LINE__, 100ns, "100.0ns"},
		{__LINE__, 100s, "100.0s "},
		{__LINE__, 100us, "100.0us"},
		{__LINE__, 10ms, "10.0ms"},
		{__LINE__, 10ns, "10.0ns"},
		{__LINE__, 10s, "10.0s "},
		{__LINE__, 10us, "10.0us"},
		{__LINE__, 12.34567891s, "12.346s ", 6, 3},
		{__LINE__, 12.34567891s, "12.35s ", 5, 2},
		{__LINE__, 123.456789_ks, "123.457ks", 6, 3},
		{__LINE__, 123.4ns, "100ns", 1, 0},
		{__LINE__, 123.4ns, "120.0ns", 2, 1},
		{__LINE__, 123.4ns, "120.0ns", 2},
		{__LINE__, 123.4ns, "123.00ns", 3, 2},
		{__LINE__, 123.4ns, "123.0ns", 3},
		{__LINE__, 123.4ns, "123.40ns", 4, 2},
		{__LINE__, 123.4ns, "123.4ns", 4},
		{__LINE__, 1234.56789_ks, "1.2Ms", 3, 1},
		{__LINE__, 1h, "3.6ks"},
		{__LINE__, 1min, "60.0s "},
		{__LINE__, 1ms, "1.0ms"},
		{__LINE__, 1ns, "1.0ns"},
		{__LINE__, 1s, "1.0s "},
		{__LINE__, 1us, "1.0us"},
		{__LINE__, 4.499999999999ns, "4.50ns", 3, 2},
		{__LINE__, 4.499999999999ns, "4.50ns", 4, 2},
		{__LINE__, 4.499999999999ns, "4.5ns", 2, 1},
		{__LINE__, 4.499999999999ns, "4.5ns", 2},
		{__LINE__, 4.499999999999ns, "4.5ns", 3},
		{__LINE__, 4.499999999999ns, "4.5ns", 4},
		{__LINE__, 4.499999999999ns, "4ns", 1, 0},
		{__LINE__, 4.999999999999_ps, "0.00ps", 3, 2},
		{__LINE__, 4.999999999999_ps, "0.00ps", 4, 2},
		{__LINE__, 4.999999999999_ps, "0.0ps", 2, 1},
		{__LINE__, 4.999999999999_ps, "0.0ps", 2},
		{__LINE__, 4.999999999999_ps, "0.0ps", 3},
		{__LINE__, 4.999999999999_ps, "0.0ps", 4},
		{__LINE__, 4.999999999999_ps, "0ps", 1, 0},
		{__LINE__, 4.999999999999ns, "5.00ns", 3, 2},
		{__LINE__, 4.999999999999ns, "5.00ns", 4, 2},
		{__LINE__, 4.999999999999ns, "5.0ns", 2, 1},
		{__LINE__, 4.999999999999ns, "5.0ns", 2},
		{__LINE__, 4.999999999999ns, "5.0ns", 3},
		{__LINE__, 4.999999999999ns, "5.0ns", 4},
		{__LINE__, 4.999999999999ns, "5ns", 1, 0},
		{__LINE__, 5.000000000000_ps, "0.00ps", 3, 2},
		{__LINE__, 5.000000000000_ps, "0.00ps", 4, 2},
		{__LINE__, 5.000000000000_ps, "0.0ps", 2},
		{__LINE__, 5.000000000000_ps, "0.0ps", 3},
		{__LINE__, 5.000000000000_ps, "0.0ps", 4},
		{__LINE__, 5.000000000000_ps, "0ps", 1, 0},
		{__LINE__, 5.000000000000ns, "5.00ns", 3, 2},
		{__LINE__, 5.000000000000ns, "5.00ns", 4, 2},
		{__LINE__, 5.000000000000ns, "5.0ns", 2, 1},
		{__LINE__, 5.000000000000ns, "5.0ns", 2},
		{__LINE__, 5.000000000000ns, "5.0ns", 3},
		{__LINE__, 5.000000000000ns, "5.0ns", 4},
		{__LINE__, 5.000000000000ns, "5ns", 1, 0},
		//**********************************
		{__LINE__, 0.0_ps, "0.0ps"},
		{__LINE__, 0.123456789us, "123.5ns", 4},
		{__LINE__, 1.23456789s, "1s ", 1, 0},
		{__LINE__, 1.23456789s, "1.2s ", 3},
		{__LINE__, 1.23456789s, "1.2s "},
		{__LINE__, 1.23456789us, "1.2us"},
		{__LINE__, 1004.4ns, "1.0us", 2},
		{__LINE__, 12.3456789s, "10s ", 1, 0},
		{__LINE__, 12.3456789s, "12.3s ", 3},
		{__LINE__, 12.3456789us, "12.3us", 3},
		{__LINE__, 12.3456s, "12.0s "},
		{__LINE__, 12.34999999ms, "10ms", 1, 0},
		{__LINE__, 12.34999999ms, "12.3ms", 3},
		{__LINE__, 12.34999999ms, "12.3ms", 4},
		{__LINE__, 12.4999999ms, "12.0ms"},
		{__LINE__, 12.4999999ms, "12.5ms", 3},
		{__LINE__, 12.5000000ms, "13.0ms"},
		{__LINE__, 123.456789ms, "123.0ms", 3},
		{__LINE__, 123.456789us, "120.0us"},
		{__LINE__, 123.4999999ms, "123.5ms", 4},
		{__LINE__, 1234.56789us, "1.2ms"},
		{__LINE__, 245.0_ps, "250.0ps"},
		{__LINE__, 49.999_ps, "50.0ps"},
		{__LINE__, 50.0_ps, "50.0ps"},
		{__LINE__, 9.49999_ps, "0.0ps"},
		{__LINE__, 9.9999_ps, "10.0ps"}, // The lower limit of accuracy is 10ps.
		{__LINE__, 9.999ns, "10.0ns"},
		{__LINE__, 99.49999_ps, "99.0ps"},
		{__LINE__, 99.4999ns, "99.0ns"},
		{__LINE__, 99.4ms, "99.0ms"},
		{__LINE__, 99.5_ps, "100.0ps"},
		{__LINE__, 99.5ms, "100.0ms"},
		{__LINE__, 99.5ns, "100.0ns"},
		{__LINE__, 99.5us, "100.0us"},
		{__LINE__, 99.999_ps, "100.0ps"},
		{__LINE__, 999.0_ps, "1.0ns"},
		{__LINE__, 999.45ns, "1us", 1, 0},
		{__LINE__, 999.45ns, "1.0us", 2},
		{__LINE__, 999.45ns, "999.0ns", 3},
		{__LINE__, 999.45ns, "999.5ns", 4},
		{__LINE__, 999.45ns, "999.45ns", 5, 2},
		{__LINE__, 999.55ns, "1.0us", 3},
		{__LINE__, 99ms, "99.0ms"},
	};

	for (auto& i : samples)
	{
		const auto str = misc::to_string(i.sec_, i.precision_, i.dec_);
		assert(i.res_ == str);
	}

	return 0;
}();
#endif // #ifndef NDEBUG

void misc::progress_t::print(double f) const
{
#if _WIN32
	auto is_atty = [](FILE* pf) { return 0 != _isatty(_fileno(pf)); }; // test whether a file descriptor refers to a terminal
#else
	auto is_atty = [](FILE* pf) { return 0 != isatty(fileno(pf)); };
#endif

	if (is_atty(stdout))
	{
		if (!std::isnan(f))
		{	std::cout << "\r" << title_ << "... " << std::fixed << std::setprecision(1) << std::setw(3) << 100.0 * f << '%';
		}
		else
		{	// std::isnan(f) - Call from the destructor.
			const auto str = to_string(ch::steady_clock::now() - s_);
			std::cout << "\r" << std::setw(title_.size() + 8) << "" << "\r" << title_ << "... Done (" << str << ")\n";
		}
		flush(std::cout);
	}
	else
	{
		if (0.0 == f) //-V550 //-V2550
		{	// Call from the constructor.
			std::cout << title_ << "...";
		}
		else if (std::isnan(f))
		{	// Call from the destructor.
			const auto str = to_string(ch::steady_clock::now() - s_);
			std::cout << " Done (" << str << ")\n";
		}
	}
}

void misc::warming(bool all, ch::milliseconds d, bool silent)
{
	if (0 != d.count())
	{	std::unique_ptr<progress_t> progress{ silent ? nullptr : new progress_t{ "Warming" } };

		const auto cnt = (all && std::thread::hardware_concurrency() > 1) ?
			(std::thread::hardware_concurrency() - 1) :
			0;

		std::atomic_bool done = false;
		std::vector<std::thread> threads(cnt);
		std::generate(threads.begin(), threads.end(), [&done] {return std::thread{ [&done] { while (!done) {/**/ }} }; });

		const auto start = ch::steady_clock::now();
		const auto stop = start + d;
		constexpr auto step = 250ms;
		for (auto temp = start + step; temp < stop; temp += step)
		{	while (ch::steady_clock::now() < temp)
			{/**/
			}

			if(progress)
				progress->operator()(ch::duration_cast<ch::duration<double>>(temp - start) / d);
		}
		while (ch::steady_clock::now() < stop)
		{/**/
		}
		done = true;

		std::for_each(threads.begin(), threads.end(), [](auto& t) {t.join(); });
	}
}
