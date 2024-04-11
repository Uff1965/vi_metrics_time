// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"
#include "misc.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <ostream>
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

namespace
{
#ifdef NDEBUG
	constexpr char BUILD_TYPE[] = "Release";
#else
	constexpr char BUILD_TYPE[] = "Debug";
#endif

	using strs_t = std::vector<std::string>;
	enum class sort_t : unsigned char { name, discreteness, duration, tick, type, nosort, _quantity };

	auto g_sort = sort_t::discreteness;
	auto g_warming = 1'000ms;
	strs_t g_include;
	strs_t g_exclude;
	int g_repeat = 1;

	inline auto now() { return vi_mt::now(); }

	struct space_out : std::numpunct<char> {
		char do_thousands_sep() const override { return '\''; }  // separate with spaces
		std::string do_grouping() const override { return "\3"; } // groups of 1 digit
	};

	void params(int argc, char* argv[])
	{
		auto fn = [](const std::string& i, const char* s) {
			std::ostringstream os;
			os << i << " "s << std::quoted(s);
			return os.str();
			};
		std::cout << std::accumulate(argv, &argv[argc], "Command line:"s, fn) << "\n";
		endl(std::cout);

		for (int n = 1; n < argc;)
		{
			auto ptr = argv[n++];
			if ("-s"sv == ptr || "--sort"sv == ptr)
			{
				if (n >= argc)
					continue;

				ptr = argv[n++];
				if ("name"sv == ptr)
					g_sort = sort_t::name;
				else if ("discreteness"sv == ptr)
					g_sort = sort_t::discreteness;
				else if ("duration"sv == ptr)
					g_sort = sort_t::duration;
				else if ("tick"sv == ptr)
					g_sort = sort_t::tick;
				else if ("type"sv == ptr)
					g_sort = sort_t::type;
				else if ("nosort"sv == ptr)
					g_sort = sort_t::nosort;
				else
				{	std::cerr << "ERROR: Unknown value for parametr --sort: \'" << ptr << "\'\n";
					std::exit(1);
				}
			}
			else if ("-w"sv == ptr || "--warming"sv == ptr)
			{
				if (n >= argc)
					continue;

				ptr = argv[n++];
				std::istringstream in{ ptr };
				std::int64_t d;
				in >> d;
				g_warming = ch::milliseconds{ d };
			}
			else if ("-i"sv == ptr || "--include"sv == ptr)
			{
				if (n >= argc) continue;
				g_include.emplace_back(argv[n++]);
			}
			else if ("-e"sv == ptr || "--exclude"sv == ptr)
			{
				if (n >= argc) continue;
				g_exclude.emplace_back(argv[n++]);
			}
			else if ("-r"sv == ptr || "--repeat"sv == ptr)
			{
				g_repeat = 5;
				if (n >= argc) continue;
				if ('-' == *argv[n]) continue;
				ptr = argv[n++];

				g_repeat = atoi(ptr);
				if(g_repeat <= 0)
				{	std::cerr << "ERROR: Wrong value for parametr --repeat: \'" << ptr << "\'\n";
					std::exit(1);
				}
			}
			else
			{
				auto error = false;
				if ("-h"sv != ptr && "--help"sv != ptr)
				{	std::cerr << "ERROR: Unknown parameter \'" << ptr << "\'\n";
					error = true;
				}

				std::cout << "\nOptions:\n" <<
					"-[-h]elp: this help;\n"
					"-[-w]arming 1|0: by default - ON; Implicit - OFF;\n"
					"-[-s]sort name|discreteness|duration|tick|type|nosort: by default - discreteness;\n"
					"-[-i]nclude <name>: include function name;\n"
					"-[-e]xclude <name>: exclude function name;\n"
					"-[-r]epeat <N>: number of measurements. 5 by default;\n";

				std::exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
			}
		} // for (int n = 1; n < argc;)
	} // params(int argc, char* argv[])

	void prefix()
	{
#ifdef _WIN32
		const char subkey[] = "Hardware\\Description\\System\\CentralProcessor\\0";
		const char value[] = "ProcessorNameString";
		std::string buff("Unknown");
		auto len = static_cast<DWORD>(buff.size());
		do {
			buff.resize(len);
		} while (ERROR_MORE_DATA == ::RegGetValueA(HKEY_LOCAL_MACHINE, subkey, value, RRF_RT_REG_SZ, NULL, buff.data(), &len)); //-V2571
		std::cout << "Processor: " << buff;
#elif defined(__linux__)
		[[maybe_unused]] int _;
		_ = std::system("lscpu | grep \'Model name:\'");
		_ = std::system("lscpu | grep \'CPU max\'");
		_ = std::system("lscpu | grep \'CPU min\'");
#endif
		endl(std::cout);
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
					<< "\tPeriodic time adjustments \'GetSystemTimeAdjustment()\':"
					<< " TimeAdjustment = " << static_cast<double>(TimeAdjustment) * F << "ms;"
					<< " TimeIncrement = " << static_cast<double>(TimeIncrement) * F << "ms;"
					<< " TimeAdjustmentDisabled = " << std::boolalpha << (0 != TimeAdjustmentDisabled) << ";"
					<< "\n";
			}
			else
				std::cout << "GetSystemTimeAdjustment() -> " << GetLastError() << " Error!\n";
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
				<< clock_getres(CLOCK_REALTIME) << ";";
			std::cout
				<< "\tResolution (precision) \'clock_getres(CLOCK_MONOTONIC)\': "
				<< clock_getres(CLOCK_MONOTONIC) << ";";
			std::cout
				<< "\tResolution (precision) \'clock_getres(CLOCK_PROCESS_CPUTIME_ID)\': "
				<< clock_getres(CLOCK_PROCESS_CPUTIME_ID) << ";";
			std::cout
				<< "\tResolution (precision) \'clock_getres(CLOCK_THREAD_CPUTIME_ID)\': "
				<< clock_getres(CLOCK_THREAD_CPUTIME_ID) << ";";

			endl(std::cout);
		}
#endif
	} // suffix()

	std::ostream& time_now(std::ostream& os)
	{	char mbstr[20];
		std::time_t t = std::time(nullptr);
#pragma warning(suppress: 4996)
		std::strftime(mbstr, sizeof(mbstr), "%Y.%m.%d %H:%M:%S", std::localtime(&t));
		return os << mbstr;
	}
} // namespace

namespace measure_functions
{
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
		return out << std::left << std::setfill('.')
			<< std::setw(48) << str.name_ << ": "
			<< std::right << std::setfill(' ')
			<< std::setw(13) << str.disc_
			<< std::setw(7) << str.disc_prec_
			<< std::setw(10) << str.durn_
			<< std::setw(7) << str.durn_prec_
			<< std::setw(10) << str.val_
			<< std::setw(7) << str.val_prec_
			<< std::setw(8) << str.val_sleep_;
	}

	struct data_t
	{
		std::string name_;
		double call_duration_;
		double discreteness_;
		double unit_;
		std::string type_;
		//double unit_of_sleeping_process_;
		//double unit_of_currrentthread_work_;
		//double unit_of_allthreads_work_;

		double duration_prec_;
		double discreteness_prec_;
		double unit_prec_;
	};

	misc::duration_t tick(const data_t& itm)
	{
		return misc::duration_t{ itm.unit_ };
	}

	misc::duration_t discreteness(const data_t& itm)
	{
		return misc::duration_t{ itm.unit_ * itm.discreteness_ };
	}

	template<sort_t E> auto make_tuple(const data_t& v);
	template<sort_t E> bool less(const data_t& l, const data_t& r)
	{	return make_tuple<E>(r) < make_tuple<E>(l);
	}

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

	std::ostream& operator<<(std::ostream& out, const std::vector<data_t>& data)
	{
		print_itm(out, { "Name", "Discreteness:", "Duration:", "One tick:", "Type:", "%", "%", "%"}) << "\n";

		for (const auto& m : data)
		{
			auto prn_sd = [](double f)
			{
				std::string result = "<err>";
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
				to_string(misc::duration_t{ m.call_duration_ }),
				to_string(misc::duration_t{ m.unit_ }),
				m.type_,
				prn_sd(m.discreteness_prec_),
				prn_sd(m.duration_prec_),
				prn_sd(m.unit_prec_),
			};
			print_itm(out, s) << "\n";
		}
		return out;
	}

	vi_mt::cont_t measurement(const strs_t& inc, const strs_t& exc, const std::function<void(double)>& progress)
	{	auto filter = [&inc, &exc](std::string_view s)
		{	auto pred = [s](const auto& e) { return s.find(e) != std::string::npos; };
			return !(std::any_of(exc.begin(), exc.end(), pred) || !inc.empty() && std::none_of(inc.begin(), inc.end(), pred));
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
	{
		misc::progress_t progress{ "Collecting function properties" };

		std::map<std::string, data2_t, std::less<>> result;
		for (int n = 0; n < g_repeat; ++n)
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

#define CALC_SLICE
#ifdef CALC_SLICE
	std::pair<double, double> calc(std::vector<double> data)
	{	assert(!data.empty());

		const auto begin = data.begin();
		auto end = data.end();
		auto size = static_cast<double>(data.size());

		// Average.
		auto aveg = std::accumulate(begin, end, 0.0) / size;

		if (const auto it = std::remove_if(begin, end, [max = 1.2 * aveg](auto v) {return v > max; }); it != end)
		{	end = it;
			size = static_cast<double>(std::distance(begin, end));
			aveg = std::accumulate(begin, end, 0.0) / size;
		}

		// Standard Deviation in percentage.
		auto sd = std::accumulate(begin, end, 0.0, [](auto i, auto v) {return i + std::pow(v, 2.0); });
		sd = sd * size / std::pow(std::accumulate(begin, end, 0.0), 2) + std::numeric_limits<decltype(sd)>::epsilon();
		assert(sd >= 1.0);
		sd = (sd >= 1.0) ? (std::sqrt(sd - 1.0) * 100.0) : 0.0;

		return std::make_pair(aveg, sd);
	}
#	ifndef NDEBUG
	const auto test_calc = []
	{	static const std::vector<double> samples = {5, 2, 4, 7, 4, 4, 5, 5, 9}; // ->{5, 2, 4, 4, 4, 5, 5} see function calc !!!
		constexpr auto average = 4.142857143;
		constexpr auto sd = 0.9897433186 * 100.0 / average; // percentages

		const auto [a, d] = calc(samples);
		assert(std::abs(a / average - 1.0) < 1e-6);
		assert(std::abs(d / sd - 1.0) < 1e-6);
		return 0;
	}();
#	endif
#elif defined CALC_MIN
	std::pair<double, double> calc(std::vector<double> data)
	{
		assert(!data.empty());

		const auto min = *std::min_element(data.begin(), data.end());
		auto sd = std::accumulate(data.begin(), data.end(), 0.0, [](auto i, auto v) {return i + std::pow(v, 2.0); });
		sd = sd * static_cast<double>(data.size()) / std::pow(std::accumulate(data.begin(), data.end(), 0.0), 2);
		assert(sd >= 1.0);
		sd = (sd >= 1.0) ? (std::sqrt(sd - 1.0) * 100.0) : 0.0;
		return std::make_pair(min, sd);
	}
#else
#	error "'calc()' function not defined."
#endif

	auto action(const cont2_t::value_type& pair)
	{
		const auto &itm = pair.second;
		data_t result{ pair.first };

		std::tie(result.discreteness_, result.discreteness_prec_) = calc(itm.discreteness_);
		std::tie(result.call_duration_, result.duration_prec_) = calc(itm.call_duration_);
		std::tie(result.unit_, result.unit_prec_) = calc(itm.unit_of_currrentthread_work_);
		const auto uallw = calc(itm.unit_of_allthreads_work_).first;
		const auto usleep = calc(itm.unit_of_sleeping_process_).first;

		if (result.unit_ / uallw > 1.2)
			result.type_ = "Process"s; // The process-clock is affected by the load on all cores.
		else if (usleep / result.unit_ > 1.2)
			result.type_ = "Thread"s; // The thread-clock readings are affected only by the thread's load.
		else
			result.type_ = "Wall"s; // Wall-clock readings are independent of processor load.

		return result;
	}

	std::vector<data_t> prepare(cont2_t&& data)
	{
		std::vector<data_t> result;

		std::transform(data.begin(), data.end(), std::back_inserter(result), action);

		if (g_sort != sort_t::nosort)
		{
			static constexpr bool(*sort_predicates[])(const data_t & l, const data_t & r) = {
				// The order of the elements must match the order of the sort_t enum.
				less<sort_t::name>,
				less<sort_t::discreteness>,
				less<sort_t::duration>,
				less<sort_t::tick>,
				less<sort_t::type>,
			};
			static_assert(std::size(sort_predicates) == static_cast<std::underlying_type_t<sort_t>>(sort_t::_quantity) - 1);

			auto pred = less<sort_t::discreteness>;
			if (const auto pos = static_cast<std::underlying_type_t<sort_t>>(g_sort); pos < std::size(sort_predicates))
				pred = sort_predicates[pos];

			std::stable_sort(result.begin(), result.end(), pred);
		}

		return result;
	}

	void work()
	{
		auto data = prepare(collect());
		std::cout << "\nMeasured properties of time functions:\n" << data << std::endl;
	}
} // namespace measure_functions

vi_mt::cont_t vi_mt::metric_base_t::action(const std::function<bool(std::string_view)>& filter, const std::function<void(double)>& pb)
{	
	cont_t result;
	for (std::size_t n = 0; n < s_measurers_.size(); ++n)
	{	if (const auto& f = s_measurers_[n].get(); !filter || filter(f.name()))
		{	auto fn = [&pb, n, sz = s_measurers_.size()](double part) {if (pb) pb((static_cast<double>(n) + part) / static_cast<double>(sz)); };
			result.emplace(f.name(), f.measurement(fn));
		}
		else if (pb)
		{	pb(static_cast<double>(n + 1) / static_cast<double>(s_measurers_.size()));
		}
	}
	return result;
}

int main(int argc, char* argv[])
{
	std::cout << "Start: " << time_now << '\n';
	std::cout << "Build type: " << BUILD_TYPE << '\n';
	endl(std::cout);

	struct space_out : std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with spaces
		std::string do_grouping() const override { return "\3"; } // groups of 1 digit
	};
	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	params(argc, argv);
	prefix();

	endl(std::cout);
	measure_functions::work();

	endl(std::cout);
	suffix();

	std::cout << "Finish: " << time_now << std::endl;
}
