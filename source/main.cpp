// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"
#include "misc.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>

#include <stdio.h> // for fileno(...)
#include <time.h>
#if _WIN32
#	define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#	define NOMINMAX
#	include <Windows.h>
#	include <profileapi.h> // for QueryPerformanceFrequency(...)
#	include <timeapi.h> // for timeGetDevCaps
#	include <winternl.h> // for NtQueryTimerResolution
#else
#	include <sys/time.h> // for clock_getres(...)
#	include <unistd.h> // for _SC_CLK_TCK
#endif

using namespace std::literals;
namespace ch = std::chrono;

namespace misc
{
	ch::milliseconds g_warming = 1'000ms;
}

namespace
{
	std::string version()
	{
#ifdef NDEBUG
		auto result = "Release"s;
#else
		auto result = "Debug"s;
#endif
		result += " \'vi_metrics_time\' " __DATE__ " " __TIME__ "."sv;
		return result;
	}

	using strs_t = std::vector<std::string>;
	enum class sort_t : unsigned char { name, discreteness, duration, tick, type, _quantity };
	enum class stat_t: unsigned char { avg, min, median, _quantity };

	auto g_stat = stat_t::median;
	auto g_sort = sort_t::name;
	auto g_repeat = 1U;
	strs_t g_include;
	strs_t g_exclude;

	template<typename T>
	T from_string(const char* ptr, std::string_view name);
		
	template<>
	sort_t from_string<sort_t>(const char* ptr, std::string_view name)
	{	sort_t result;

		if ("name"sv == ptr)
			result = sort_t::name;
		else if ("discreteness"sv == ptr)
			result = sort_t::discreteness;
		else if ("duration"sv == ptr)
			result = sort_t::duration;
		else if ("tick"sv == ptr)
			result = sort_t::tick;
		else if ("type"sv == ptr)
			result = sort_t::type;
		else
		{	std::cerr << "ERROR: Wrong value for parametr --" << name << ": \'" << ptr << "\'\n";
			std::exit(1);
		}

		return result;
	}

	template<>
	stat_t from_string< stat_t>( const char* ptr, std::string_view name)
	{	stat_t result;

		if ("average"sv == ptr)
			result = stat_t::avg;
		else if ("minimum"sv == ptr)
			result = stat_t::min;
		else if ("median"sv == ptr)
			result = stat_t::median;
		else
		{	std::cerr << "ERROR: Wrong value for parametr --" << name << ": \'" << ptr << "\'\n";
			std::exit(1);
		}

		return result;
	}

	template<>
	unsigned from_string<unsigned>(const char* ptr, std::string_view name) try
	{	return std::stoul(ptr);
	}
	catch (...)
	{	std::cerr << "ERROR: Wrong value for parametr --" << name << ": \'" << ptr << "\'\n";
		std::exit(1);
	}

	void parsing_of_parameters(int argc, char* argv[])
	{
		auto fn = [](const std::string& i, const char* s)
			{	std::ostringstream os;
				os << i << " "s << std::quoted(s);
				return os.str();
			};
		std::cout << std::accumulate(argv, &argv[argc], "Command line:"s, fn) << "\n";
		endl(std::cout);

		for (int n = 1; n < argc;)
		{	auto ptr = argv[n++];
			if ("-s"sv == ptr || "--sort"sv == ptr)
			{	g_sort = (n < argc && argv[n][0] != '-')? from_string<sort_t>(argv[n++], "sort"sv) : sort_t::discreteness;
			}
			else if ("--stat"sv == ptr)
			{	g_stat = (n < argc && argv[n][0] != '-')? from_string<stat_t>(argv[n++], "stat"sv) : stat_t::min;
			}
			else if ("-w"sv == ptr || "--warming"sv == ptr)
			{	misc::g_warming = ch::milliseconds{ (n < argc && argv[n][0] != '-') ? from_string<unsigned>(argv[n++], "warming"sv) : 0 };
			}
			else if ("-r"sv == ptr || "--repeat"sv == ptr)
			{	g_repeat = (n < argc && argv[n][0] != '-') ? from_string<unsigned>(argv[n++], "repeat"sv) : 5;
			}
			else if ("-i"sv == ptr || "--include"sv == ptr)
			{	if (n >= argc || argv[n][0] == '-')
				{	std::cerr << "ERROR: Empty value for parametr --include\n";
					std::exit(1);
				}

				g_include.emplace_back(argv[n++]);
			}
			else if ("-e"sv == ptr || "--exclude"sv == ptr)
			{	if (n >= argc || argv[n][0] == '-')
				{	std::cerr << "ERROR: Empty value for parametr --exclude\n";
					std::exit(1);
				}

				g_exclude.emplace_back(argv[n++]);
			}
			else if ("--version"sv == ptr)
			{	std::cout << version() << std::endl;
				std::exit(1);
			}
			else
			{	auto error = false;
				if ("-h"sv != ptr && "--help"sv != ptr)
				{	std::cerr << "ERROR: Unknown parameter \'" << ptr << "\'\n";
					error = true;
				}

				std::cout << "Options:\n" <<
					"-[-h]elp: this help;\n"
					"-[-w]arming 1|0: by default - 1s; implicit - OFF;\n"
					"-[-s]sort name|discreteness|duration|tick|type: by default - name; implicit - discreteness;\n"
					"-[-i]nclude <name>: include function name;\n"
					"-[-e]xclude <name>: exclude function name;\n"
					"-[-r]epeat <N>: number of measurements. by default - 1; implicit - 5;\n"
					"--stat average|minimum|median: by default - median; implicit - minimum;\n"
					"--version: type and time compyle program;\n";

				std::exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
			}
		} // for (int n = 1; n < argc;)
	} // parsing_of_parameters(int argc, char* argv[])

	void prefix()
	{
#ifdef _WIN32
		constexpr char subkey[] = "Hardware\\Description\\System\\CentralProcessor\\0";
		constexpr char value[] = "ProcessorNameString";
		std::string buff("Unknown");
		auto len = static_cast<DWORD>(buff.size());
		do
		{	buff.resize(len); //-V106
		} while (ERROR_MORE_DATA == ::RegGetValueA(HKEY_LOCAL_MACHINE, subkey, value, RRF_RT_REG_SZ, NULL, buff.data(), &len)); //-V2571
		std::cout << "Processor: " << buff;

		constexpr char COMPUTERNAME[] = "COMPUTERNAME";
#elif defined(__linux__)
		(void)std::system("lscpu | grep \'Model name:\'");
		(void)std::system("lscpu | grep \'CPU max\'");
		(void)std::system("lscpu | grep \'CPU min\'");

		constexpr char COMPUTERNAME[] = "HOSTNAME";
#endif
		endl(std::cout);

#pragma warning(suppress: 4996)
		if (const auto computername = std::getenv(COMPUTERNAME))
		{	std::cout << "Computer: \'" << computername << "\'\n";
		}
	}

	void suffix()
	{
		std::cout << "Notes:\n";

		std::cout
			<< "\tThe std::chrono::system_clock::is_steady:          "
			<< std::boolalpha << ch::system_clock::is_steady << ";\n"
			<< "\tThe std::chrono::steady_clock::is_steady:          "
			<< std::boolalpha << ch::steady_clock::is_steady << ";\n"
			<< "\tThe std::chrono::high_resolution_clock::is_steady: "
			<< std::boolalpha << ch::high_resolution_clock::is_steady << ";\n";
		std::cout
			<< "\tThe std::chrono::system_clock::duration:          "
			<< std::setw(7) << std::right << misc::duration_t{ ch::system_clock::duration{ 1 } } << ";\n"
			<< "\tThe std::chrono::steady_clock::duration:          "
			<< std::setw(7) << std::right << misc::duration_t{ ch::steady_clock::duration{ 1 } } << ";\n"
			<< "\tThe std::chrono::high_resolution_clock::duration: "
			<< std::setw(7) << std::right << misc::duration_t{ ch::high_resolution_clock::duration{ 1 } } << ";\n";

		std::cout
			<< "\tThe number of clock vi_tmGetTicks per second \'CLOCKS_PER_SEC\': " << CLOCKS_PER_SEC
			<< " (what is equivalent to " << misc::to_string(misc::duration_t{ 1.0 / static_cast<double>(CLOCKS_PER_SEC) }, 3) << ")"
			<< "\n";

#ifdef _WIN32
		if (LARGE_INTEGER frequency; QueryPerformanceFrequency(&frequency))
		{
			std::cout
				<< "\tFrequency of the performance counter \'QueryPerformanceFrequency()\': " << frequency.QuadPart
				<< " (what is equivalent to " << misc::to_string(misc::duration_t{ 1.0 / static_cast<double>(frequency.QuadPart) }, 3) << ");" // counts per second
				<< "\n";
		}

		if (ULONG minimum, maximum, current; SUCCEEDED(NtQueryTimerResolution(&maximum, &minimum, &current)))
		{
			static constexpr auto F = 100e-9 * 1e3; // 100ns interval in 'ms'.
			std::cout
				<< "\tThe timer resolution \'NtQueryTimerResolution()\': " // 100-nanoseconds intervals
				<< std::setprecision(3) << F * static_cast<double>(current) << "ms. (from " << F * static_cast<double>(minimum) << " to " << F * static_cast<double>(maximum) << "ms);"
				<< "\n";
		}

		if (TIMECAPS tc; MMSYSERR_NOERROR == timeGetDevCaps(&tc, sizeof(tc)))
		{
			std::cout
				<< "\tThe timer device to determine its resolution \'timeGetDevCaps()\': from "
				<< tc.wPeriodMin << " to " // The minimum supported resolution, in milliseconds.
				<< tc.wPeriodMax << "ms.;" // The maximum supported resolution, in milliseconds.
				<< "\n";
		}

		{
			DWORD TimeAdjustment;
			DWORD TimeIncrement;
			BOOL TimeAdjustmentDisabled;
			if (::GetSystemTimeAdjustment(&TimeAdjustment, &TimeIncrement, &TimeAdjustmentDisabled))
			{
				static constexpr auto F = 100e-9 * 1e3; // 100ns interval in 'ms'.
				std::cout
					<< "\tPeriodic time adjustments \'GetSystemTimeAdjustment()\':\n"
					<< "\t\tTimeAdjustment = " << static_cast<double>(TimeAdjustment) * F << "ms;\n"
					<< "\t\tTimeIncrement = " << static_cast<double>(TimeIncrement) * F << "ms;\n"
					<< "\t\tTimeAdjustmentDisabled = " << std::boolalpha << (0 != TimeAdjustmentDisabled) << ";"
					<< "\n";
			}
			else
			{	std::cout << "GetSystemTimeAdjustment() -> " << GetLastError() << " Error!\n";
			}
		}
#elif defined __linux__
		std::cout
			<< "\tThe number of clock vi_tmGetTicks per second \'sysconf(_SC_CLK_TCK)\': " << sysconf(_SC_CLK_TCK)
			<< " (" << 1e3 / sysconf(_SC_CLK_TCK) << "ms);"
			<< "\n";

		{
			auto clock_getres = [](clockid_t clk_id)
			{
				timespec ts;
				::clock_getres(clk_id, &ts);
				return misc::duration_t{ ts.tv_sec + 1e-9 * ts.tv_nsec };
			};

			std::cout
				<< "\tResolution (precision) \'clock_getres(CLOCK_REALTIME)\': " 
				<< clock_getres(CLOCK_REALTIME) << ";\n";
			std::cout
				<< "\tResolution (precision) \'clock_getres(CLOCK_MONOTONIC)\': "
				<< clock_getres(CLOCK_MONOTONIC) << ";\n";
			std::cout
				<< "\tResolution (precision) \'clock_getres(CLOCK_PROCESS_CPUTIME_ID)\': "
				<< clock_getres(CLOCK_PROCESS_CPUTIME_ID) << ";\n";
			std::cout
				<< "\tResolution (precision) \'clock_getres(CLOCK_THREAD_CPUTIME_ID)\': "
				<< clock_getres(CLOCK_THREAD_CPUTIME_ID) << ";";

			endl(std::cout);
		}
#endif
	} // suffix()

	class locale_with_grouping : public std::locale
	{
		struct space_out_t : std::numpunct<char>
		{	char do_thousands_sep() const override { return '\''; }  // separate with apostrophe
			std::string do_grouping() const override { return "\3"; } // groups of 3 digit
		};
	public:
		locale_with_grouping(const std::locale& loc) : std::locale(loc, new space_out_t) {}
	};

	struct str_out
	{
		std::string name_;
		std::string disc_;
		std::string durn_;
		std::string val_;
		std::string val_sleep_;
		std::string disc_prec_{""};
		std::string durn_prec_{""};
		std::string val_prec_{""};
	};

	std::ostream& print_itm(std::ostream& out, const str_out& str)
	{
		static auto marker = [](const std::string& s) {return !s.empty() && std::all_of(s.begin(), s.end(), [](auto c) {return c == ' '; }); };
		auto text = [](const std::string& s) {return marker(s) ? "vvvvvv" : s.c_str(); };
		out << std::left << std::setw(49);
		if (std::any_of(str.name_.begin(), str.name_.end(), [](auto c) {return c != ' '; }))
		{	out << std::setfill('.') << (str.name_ + ":") << std::setfill(' ');
		}
		else
		{	out << text(str.name_);
		}
		return out << std::right
			<< std::setw(14) << text(str.disc_)
			<< std::setw(7) << text(str.disc_prec_)
			<< std::setw(10) << text(str.durn_)
			<< std::setw(7) << text(str.durn_prec_)
			<< std::setw(10) << text(str.val_)
			<< std::setw(7) << text(str.val_prec_)
			<< std::setw(8) << text(str.val_sleep_);
	}

	struct data_t
	{
		std::string name_;
		double call_duration_;
		double discreteness_;
		double unit_;
		std::string type_;

		double duration_prec_;
		double discreteness_prec_;
		double unit_prec_;
	};

	misc::duration_t tick(const data_t& itm)
	{	return misc::duration_t{ itm.unit_ };
	}

	misc::duration_t discreteness(const data_t& itm)
	{	return misc::duration_t{ itm.discreteness_ * itm.unit_ };
	}

	template<sort_t E> auto make_tuple(const data_t& v);
	template<> auto make_tuple<sort_t::name>(const data_t& v)
	{	return std::tuple{ v.name_, discreteness(v), v.call_duration_, tick(v), v.type_ };
	}
	template<> auto make_tuple<sort_t::discreteness>(const data_t& v)
	{	return std::tuple{ discreteness(v), v.call_duration_, tick(v), v.type_, v.name_ };
	}
	template<> auto make_tuple<sort_t::duration>(const data_t& v)
	{	return std::tuple{ v.call_duration_, discreteness(v), tick(v), v.type_, v.name_ };
	}
	template<> auto make_tuple<sort_t::tick>(const data_t& v)
	{	return std::tuple{ tick(v), discreteness(v), v.call_duration_, v.type_, v.name_ };
	}
	template<> auto make_tuple<sort_t::type>(const data_t& v)
	{	return std::tuple{ v.type_, discreteness(v), v.call_duration_, tick(v), v.name_ };
	}
	template<sort_t E> bool less(const data_t& l, const data_t& r)
	{	return make_tuple<E>(r) < make_tuple<E>(l);
	}

	std::ostream& operator<<(std::ostream& out, const std::vector<data_t>& data)
	{
		print_itm(out, { "Name", "Discreteness:", "Duration:", "One tick:", "Type:", "+/-", "+/-", "+/-" }) << "\n";
		{
			str_out sort_line = { "", "", "", "", "" };
			static constexpr char marker[] = " ";
			switch (g_sort)
			{
				case sort_t::discreteness:
					sort_line.disc_ = marker;
					break;
				case sort_t::duration:
					sort_line.durn_ = marker;
					break;
				case sort_t::tick:
					sort_line.val_ = marker;
					break;
				case sort_t::type:
					sort_line.val_sleep_ = marker;
					break;
				case sort_t::name:
				default:
					sort_line.name_ = marker;
					break;
			}
			print_itm(out, sort_line) << "\n";
		}

		for (const auto& m : data)
		{
			auto prn_sd = [](double f)
				{	std::string result = "<err>";
					if (!std::isnan(f))
					{	std::ostringstream os;
						os << std::setprecision(1) << std::fixed << misc::round(f, 2);
						result = os.str() + "%";
					}
					return result;
				};

			str_out s =
			{	m.name_,
				to_string(discreteness(m), 3),
				to_string(misc::duration_t{ m.call_duration_ }, 2),
				to_string(tick(m), 2),
				m.type_,
				prn_sd(m.discreteness_prec_),
				prn_sd(m.duration_prec_),
				prn_sd(m.unit_prec_),
			};
			print_itm(out, s) << "\n";
		}
		return out;
	}

	vi_mt::raw_t measurement(const strs_t& inc, const strs_t& exc, const std::function<void(double)>& progress)
	{	auto filter = [&inc, &exc](std::string_view s)
			{	auto pred = [s](const auto& e) { return s.find(e) != std::string::npos; };
				return !(std::any_of(exc.begin(), exc.end(), pred) || (!inc.empty() && std::none_of(inc.begin(), inc.end(), pred)));
			};

		return vi_mt::metric_base_t::action(filter, progress);
	}

	struct data2_t
	{
		std::vector<double> call_duration_;
		std::vector<double> discreteness_;
		std::vector<double> unit_of_sleeping_process_;
		std::vector<double> unit_of_currrentthread_work_;
		std::vector<double> unit_of_allthreads_work_;
	};

	using cont2_t = std::map<std::string, data2_t, std::less<>>;

	cont2_t collect()
	{	std::map<std::string, data2_t, std::less<>> result;
		misc::progress_t progress{ "Collecting function properties" };

		for (unsigned n = 0U; n < g_repeat; ++n)
		{	const auto meas = measurement(g_include, g_exclude, [n, &progress](double f) { progress((n + f) / g_repeat); });

			for (const auto& [name, m] : meas)
			{	auto& r = result[name];
				r.call_duration_.emplace_back(m.call_duration_.count());
				r.discreteness_.emplace_back(m.discreteness_);
				r.unit_of_allthreads_work_.emplace_back(m.unit_of_allthreads_work_.count());
				r.unit_of_currrentthread_work_.emplace_back(m.unit_of_currrentthread_work_.count());
				r.unit_of_sleeping_process_.emplace_back(m.unit_of_sleeping_process_.count());
			}
		}

		return result;
	}

	template<typename I>
	constexpr double average(I begin, I end)
	{	return std::accumulate(begin, end, 0.0) / static_cast<double>(std::distance(begin, end));
	}

	template<typename I>
	double median(I begin, I end)
	{	std::vector<double> result(std::distance(begin, end));
		std::partial_sort_copy(begin, end, result.begin(), result.end()); //-V530
		const auto half = result.size() / 2;
		return result.size() % 2 ? result[half] : 0.5 * (result[half - 1] + result[half]);
	}

	template<typename I>
	double percentage_standard_deviation(I begin, I end)
	{
		auto result = std::accumulate(begin, end, 0.0, [](auto i, auto v) {return i + std::pow(v, 2.0); });
		result *= static_cast<double>(std::distance(begin, end)) / std::pow(std::accumulate(begin, end, 0.0), 2.0);
		result += std::numeric_limits<decltype(result)>::epsilon();
		assert(result >= 1.0);
		result = 100.0 * ((result > 1.0) ? std::sqrt(result - 1.0) : 0.0);
		return result;
	}

#ifndef NDEBUG
	const auto test_stat = []
		{	static constexpr double samples1[] = {5, 2, 4, 4, 4, 5, 5};
			constexpr auto median1 = 4.0;
			constexpr auto mean1 = 4.1428571428571;
			constexpr auto sd1 = 0.98974331861079 / mean1;

			static constexpr double samples2[] = {5, 2, 4, 7, 4, 4, 5, 5, 9, 3};
			constexpr auto median2 = 4.5;
			constexpr auto mean2 = 4.8;
			constexpr auto sd2 = 1.8867962264113 / mean2;

			// Math online calculators: https://www.calculator.net/math-calculator.html

			assert(std::abs(median(std::begin(samples1), std::end(samples1)) - median1) < 1e-12);
			assert(std::abs(average(std::begin(samples1), std::end(samples1)) - mean1) < 1e-12);
			assert(std::abs(percentage_standard_deviation(std::begin(samples1), std::end(samples1)) / sd1 - 100.0) < 1e-10);
			assert(std::abs(median(std::begin(samples2), std::end(samples2)) - median2) < 1e-12);
			assert(std::abs(average(std::begin(samples2), std::end(samples2)) - mean2) < 1e-12);
			assert(std::abs(percentage_standard_deviation(std::begin(samples2), std::end(samples2)) / sd2 - 100.0) < 1e-10);
			return 0;
		}();
#endif

	std::pair<double, double> calc_stat_median(std::vector<double> data)
	{	assert(!data.empty());
		const auto medn = median(data.begin(), data.end());
		const auto sd = percentage_standard_deviation(data.begin(), data.end());
		return std::make_pair(medn, sd);
	}

	std::pair<double, double> calc_stat_avg(std::vector<double> data) //-V813
	{	assert(!data.empty());

		const auto begin = data.begin();
		auto end = data.end();
		auto size = static_cast<double>(data.size()); //-V203

		// Average.
		auto aveg = average(begin, end);

		// We filter out large deviations.
		if (const auto it = std::remove_if(begin, end, [lim = aveg * 1.2](auto v) {return v >= lim; }); it != end)
		{	end = it;
			size = static_cast<double>(std::distance(begin, end));
			aveg = average(begin, end);
		}

		// Standard Deviation in percentage.
		const auto sd = percentage_standard_deviation(begin, end);

		return std::make_pair(aveg, sd);
	}

	std::pair<double, double> calc_stat_min(std::vector<double> data)
	{	assert(!data.empty());
		const auto min = *std::min_element(data.begin(), data.end());
		const auto sd = percentage_standard_deviation(data.begin(), data.end());
		return std::make_pair(min, sd);
	}

	std::pair<double, double> calc_stat(std::vector<double> data)
	{	auto calc_stat = calc_stat_median;
		switch (g_stat)
		{	default:
				assert(false);
				[[fallthrough]];
			case stat_t::median:
				calc_stat = calc_stat_median;
				break;
			case stat_t::avg:
				calc_stat = calc_stat_avg;
				break;
			case stat_t::min:
				calc_stat = calc_stat_min;
				break;
		}

		return calc_stat(std::move(data));
	}

	auto action(const cont2_t::value_type& pair)
	{	data_t result{ pair.first };
		const auto& itm = pair.second;

		std::tie(result.discreteness_, result.discreteness_prec_) = calc_stat(itm.discreteness_);
		std::tie(result.call_duration_, result.duration_prec_) = calc_stat(itm.call_duration_);
		std::tie(result.unit_, result.unit_prec_) = calc_stat_median(itm.unit_of_currrentthread_work_);
		const auto uallw = calc_stat_median(itm.unit_of_allthreads_work_).first;
		const auto usleep = calc_stat_median(itm.unit_of_sleeping_process_).first;

		if (result.unit_ / uallw > 1.2)
			result.type_ = "Process"s; // The process-clock is affected by the load on all cores.
		else if (usleep / result.unit_ > 1.2)
			result.type_ = "Thread"s; // The thread-clock readings are affected only by the thread's load.
		else
			result.type_ = "Wall"s; // Wall-clock readings are independent of processor load.

		return result;
	}

	std::vector<data_t> prepare(const cont2_t& data)
	{	std::vector<data_t> result;

		std::transform(data.begin(), data.end(), std::back_inserter(result), action);

		static constexpr bool(*sort_predicates[])(const data_t & l, const data_t & r) = {
			// The order of the elements must match the order of the sort_t enum.
			less<sort_t::name>,
			less<sort_t::discreteness>,
			less<sort_t::duration>,
			less<sort_t::tick>,
			less<sort_t::type>,
		};
		static_assert(std::size(sort_predicates) == static_cast<std::underlying_type_t<sort_t>>(sort_t::_quantity));

		auto pred = less<sort_t::name>;
		if (const auto pos = static_cast<std::underlying_type_t<sort_t>>(g_sort); pos < std::size(sort_predicates))
			pred = sort_predicates[pos];

		std::stable_sort(result.begin(), result.end(), pred);

		return result;
	}

	void work()
	{	const auto raw_data = collect();

		// RAW data can be saved here.

		const auto data = prepare(raw_data);

		std::cout << "\nMeasured properties of time functions";
		if (const auto size = raw_data.empty() ? 0 : raw_data.begin()->second.call_duration_.size(); size > 1)
		{
			switch (g_stat)
			{
				case stat_t::avg:
					std::cout << " (average of " << size << " measurements, excluding extreme values)";
					break;
				case stat_t::min:
					std::cout << " (minimum of " << size << " measurements)";
					break;
				case stat_t::median:
					std::cout << " (median of " << size << " measurements)";
					break;
				default:
					std::cout << " (ERROR - unknown prepare method!!!)";
					assert(false);
			}
		}
		std::cout << ":\n" << data << std::endl;
	}
} // namespace

vi_mt::raw_t vi_mt::metric_base_t::action(const std::function<bool(std::string_view)>& filter, const std::function<void(double)>& pb)
{	
	decltype(s_measurers_) v;
	if (filter)
	{	std::for_each(s_measurers_.begin(), s_measurers_.end(), [&v, &filter](const auto& f) { if (filter(f.get().name())) v.emplace_back(f); });
	}
	else
	{	v = s_measurers_;
	}
	std::shuffle(v.begin(), v.end(), std::mt19937{ std::random_device{}() });

	raw_t result;
	for (std::size_t n = 0; n < v.size(); ++n)
	{	auto fn = [&pb, n, sz = v.size()](double part) {if (pb) pb((static_cast<double>(n) + part) / static_cast<double>(sz)); }; //-V203
		const auto& f = v[n].get();
		result.emplace(f.name(), f.measurement(fn));
	}
	return result;
}

int main(int argc, char* argv[])
{
	parsing_of_parameters(argc, argv);
	std::cout.imbue(locale_with_grouping{ std::cout.getloc() });
	std::cout << version() << "\n";

	const auto start = std::time(nullptr);
#pragma warning(suppress: 4996)
	std::cout << "Start: "sv << std::put_time(std::localtime(&start), "%Y.%m.%d %H:%M:%S");
	endl(std::cout);

	prefix();
	endl(std::cout);

	work();
	endl(std::cout);

	suffix();
	endl(std::cout);

	const auto expend = std::time(nullptr) - start;
#pragma warning(suppress: 4996)
	std::cout << "Time expend: "sv << std::put_time(std::gmtime(&expend), "%H:%M:%S");
	endl(std::cout);
}
