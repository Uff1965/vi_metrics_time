// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace
{
	constexpr auto operator""_ps(long double v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ps(unsigned long long v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ks(long double v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_ks(unsigned long long v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_Ms(long double v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Ms(unsigned long long v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Gs(long double v) noexcept { return ch::duration<double, std::giga>(v); };
	constexpr auto operator""_Gs(unsigned long long v) noexcept { return ch::duration<double, std::giga>(v); };

	double round(double num, unsigned char prec, unsigned char dec = 1)
	{ // Rounding to 'dec' decimal place and no more than 'prec' significant symbols.
		if (num < 5e-12 || prec == 0 || prec <= dec) {
			assert(num < 5e-12);
			return num;
		}

		const auto exp = static_cast<signed char>(std::floor(std::log10(num)));
		if (const auto n = 1 + dec + (12 + exp) % 3; prec > n)
			prec = static_cast<unsigned char>(n);

		const auto factor = std::max(10e-12, std::pow(10.0, exp - (prec - 1))); // The lower limit of accuracy is 0.01ns.
		return std::round((num * (1 + std::numeric_limits<decltype(num)>::epsilon())) / factor) * factor;
	}
} // namespace

std::string vi_mt::to_string(vi_mt::duration_t sec, unsigned char precision, unsigned char dec)
{
	sec = duration_t{ round(sec.count(), precision, dec) };

	auto prn = [sec, dec](const char* u, double e) {
		std::stringstream ss;
		ss << std::fixed << std::setprecision(dec) << sec.count() * e << u;
		return ss.str();
	};

	std::string result;
	if (10_ps > sec)			result = prn("ps", 1.0);
	else if (999.95_ps > sec)	result = prn("ps", 1e12);
	else if (999.95ns > sec)	result = prn("ns", 1e9);
	else if (999.95us > sec)	result = prn("us", 1e6);
	else if (999.95ms > sec)	result = prn("ms", 1e3);
	else if (999.95s > sec)		result = prn("s ", 1e0);
	else if (999.95_ks > sec)	result = prn("ks", 1e-3);
	else if (999.95_Ms > sec)	result = prn("Ms", 1e-6);
	else						result = prn("Gs", 1e-9);

	return result;
}

#ifndef NDEBUG
const auto unit_test_to_string = []
{
	struct
	{	vi_mt::duration_t sec_;
		std::string_view res_;
		unsigned char precision_{ 2 };
		unsigned char dec_{ 1 };
	}
	constexpr samples[] =
	{	{1234567.89s, "1.2Ms", 9, 1},
		{123456.789s, "123.457ks", 9, 3},
		{0, "0.0ps"},
		{0.1_ps, "0.0ps"},
		{1_ps, "0.0ps"}, // The lower limit of accuracy is 10ps.
		{10_ps, "10.0ps"},
		{100_ps, "100.0ps"},
		{1ns, "1.0ns"},
		{10ns, "10.0ns"},
		{100ns, "100.0ns"},
		{1us, "1.0us"},
		{10us, "10.0us"},
		{100us, "100.0us"},
		{1ms, "1.0ms"},
		{10ms, "10.0ms"},
		{100ms, "100.0ms"},
		{1s, "1.0s "},
		{10s, "10.0s "},
		{100s, "100.0s "},
		{1min, "60.0s "},
		{1h, "3.6ks"},

		{4.999999999999_ps, "0ps", 1, 0},
		{4.999999999999_ps, "0.0ps", 2},
		{4.999999999999_ps, "0.0ps", 3},
		{4.999999999999_ps, "0.0ps", 4},
		{5.000000000000_ps, "10ps", 1, 0},
		{5.000000000000_ps, "10.0ps", 2},
		{5.000000000000_ps, "10.0ps", 3},
		{5.000000000000_ps, "10.0ps", 4},

		{4.499999999999ns, "4ns", 1, 0},
		{4.499999999999ns, "4.5ns", 2},
		{4.499999999999ns, "4.5ns", 3},
		{4.499999999999ns, "4.5ns", 4},
		{4.999999999999ns, "5ns", 1, 0},
		{4.999999999999ns, "5.0ns", 2},
		{4.999999999999ns, "5.0ns", 3},
		{4.999999999999ns, "5.0ns", 4},
		{5.000000000000ns, "5ns", 1, 0},
		{5.000000000000ns, "5.0ns", 2},
		{5.000000000000ns, "5.0ns", 3},
		{5.000000000000ns, "5.0ns", 4},

		{123.4ns, "100ns", 1, 0},
		{123.4ns, "120.0ns", 2},
		{123.4ns, "123.0ns", 3},
		{123.4ns, "123.4ns", 4},

		{4.999999999999_ps, "0ps", 1, 0},
		{4.999999999999_ps, "0.0ps", 2, 1},
		{4.999999999999_ps, "0.00ps", 3, 2},
		{4.999999999999_ps, "0.00ps", 4, 2},
		{5.000000000000_ps, "10.00ps", 3, 2},
		{5.000000000000_ps, "10.00ps", 4, 2},

		{4.499999999999ns, "4.5ns", 2, 1},
		{4.499999999999ns, "4.50ns", 3, 2},
		{4.499999999999ns, "4.50ns", 4, 2},
		{4.999999999999ns, "5ns", 1, 0},
		{4.999999999999ns, "5.0ns", 2, 1},
		{4.999999999999ns, "5.00ns", 3, 2},
		{4.999999999999ns, "5.00ns", 4, 2},
		{5.000000000000ns, "5ns", 1, 0},
		{5.000000000000ns, "5.0ns", 2, 1},
		{5.000000000000ns, "5.00ns", 3, 2},
		{5.000000000000ns, "5.00ns", 4, 2},

		{123.4ns, "100ns", 1, 0},
		{123.4ns, "120.0ns", 2, 1},
		{123.4ns, "123.00ns", 3, 2},
		{123.4ns, "123.40ns", 4, 2},

		//**********************************
		{0.0_ps, "0.0ps"},
		{0.123456789us, "123.5ns", 4},
		{1.23456789s, "1s ", 1, 0},
		{1.23456789s, "1.2s ", 3},
		{1.23456789s, "1.2s "},
		{1.23456789us, "1.2us"},
		{1004.4ns, "1.0us", 2},
		{12.3456789s, "10s ", 1, 0},
		{12.3456789s, "12.3s ", 3},
		{12.3456789us, "12.3us", 3},
		{12.3456s, "12.0s "},
		{12.34999999ms, "10ms", 1, 0},
		{12.34999999ms, "12.3ms", 3},
		{12.34999999ms, "12.3ms", 4},
		{12.4999999ms, "12.0ms"},
		{12.4999999ms, "12.5ms", 3},
		{12.5000000ms, "13.0ms"},
		{123.456789ms, "123.0ms", 3},
		{123.456789us, "120.0us"},
		{123.4999999ms, "123.5ms", 4},
		{1234.56789us, "1.2ms"},
		{1s, "1.0s "},
		{245.0_ps, "250.0ps"},
		{49.999_ps, "50.0ps"},
		{50.0_ps, "50.0ps"},
		{9.49999_ps, "10.0ps"},
		{9.9999_ps, "10.0ps"}, // The lower limit of accuracy is 10ps.
		{9.999ns, "10.0ns"},
		{99.49999_ps, "100.0ps"},
		{99.4999ns, "99.0ns"},
		{99.4ms, "99.0ms"},
		{99.5_ps, "100.0ps"},
		{99.5ms, "100.0ms"},
		{99.5ns, "100.0ns"},
		{99.5us, "100.0us"},
		{99.999_ps, "100.0ps"},
		{999.0_ps, "1.0ns"},
		{999.45ns, "1us", 1, 0},
		{999.45ns, "1.0us", 2},
		{999.45ns, "999.0ns", 3},
		{999.45ns, "999.5ns", 4},
		{999.45ns, "999.45ns", 5, 2},
		{999.55ns, "1.0us", 3},
		{99ms, "99.0ms"},
	};

	for (auto& i : samples)
	{	const auto str = to_string(i.sec_, i.precision_, i.dec_);
		assert(i.res_ == str);
	}

	return 0;
}();
#endif // #ifndef NDEBUG

std::vector<vi_mt::item_t> vi_mt::metric_base_t::action(const std::function<bool(std::string_view)>& filter, const std::function<void(double)>& pb)
{
	std::vector<item_t> result;
	result.reserve(s_measurers_.size());
	for (std::size_t n = 0; n < s_measurers_.size(); ++n)
	{	if (const auto& f = s_measurers_[n]; !filter || filter(f.get().name()))
		{	auto fn = [&pb, n, sz = s_measurers_.size()](double part) {if (pb) pb((static_cast<double>(n) + part) / static_cast<double>(sz)); };
			result.emplace_back(f.get().measurement(fn));
		}
		else if (pb)
		{	pb(static_cast<double>(n + 1) / static_cast<double>(s_measurers_.size()));
		}
	}
	return result;
}
