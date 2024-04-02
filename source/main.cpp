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
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>

#include <stdio.h> // for fileno(...)
#if _WIN32
#	define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#	define NOMINMAX
#	include <Windows.h>
#	include <profileapi.h> // for QueryPerformanceFrequency(...)
#	include <timeapi.h> // for timeGetDevCaps
#	include <winternl.h> // for NtQueryTimerResolution
#else
#	include <sys/time.h> // for clock_getres(...)
#endif

using namespace std::literals;
namespace ch = std::chrono;

namespace
{
	using strs_t = std::vector<std::string>;
	enum class sort_t : unsigned char { name, discreteness, duration, tick, type, _quantity };

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
					"-[-s]sort name|discreteness|duration|tick|type: by default - discreteness;\n"
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
				return vi_mt::duration_t{ ts.tv_sec + 1e-9 * ts.tv_nsec };
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
} // namespace

namespace measure_functions
{
	class one_t
	{
		double sum_{};
		double squares_sum_{};
		unsigned cnt_{};
	public:
		void add(double d);
		[[nodiscard]] double average() const;
		[[nodiscard]] double std_deviation() const;
	};

	void one_t::add(double d)
	{	++cnt_;
		sum_ += d;
		squares_sum_ += std::pow(d, 2.0);
	}

	double one_t::average() const
	{	assert(cnt_);
		return cnt_ ? sum_ / cnt_ : 0.0;
	}

	double one_t::std_deviation() const
	{	assert(cnt_);
		double result = 0.0;
		if (cnt_)
		{	result = std::sqrt(cnt_ * squares_sum_ / std::pow(sum_, 2) - 1) * 100.0;
		}

		return result;
	}

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

	std::ostream& print_itm(std::ostream& out, str_out str)
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
		one_t call_duration_;
		one_t discreteness_;
		one_t unit_of_sleeping_process_;
		one_t unit_of_currrentthread_work_;
		one_t unit_of_allthreads_work_;
	};

	std::string type(const data_t& itm)
	{
		if (itm.unit_of_currrentthread_work_.average() / itm.unit_of_allthreads_work_.average() > 1.2)
			return "Process"s; // The process-clock is affected by the load on all cores.
		if (itm.unit_of_sleeping_process_.average() / itm.unit_of_currrentthread_work_.average() > 1.2)
			return "Thread"s; // The thread-clock readings are affected only by the thread's load.
		return "Wall"s; // Wall-clock readings are independent of processor load.
	}

	misc::duration_t tick(const data_t& itm)
	{
		return misc::duration_t{ itm.unit_of_currrentthread_work_.average() };
	}

	misc::duration_t discreteness(const data_t& itm)
	{
		return misc::duration_t{ itm.unit_of_currrentthread_work_.average() * itm.discreteness_.average() };
	}

	template<sort_t E> auto make_tuple(const data_t& v);
	template<sort_t E> bool less(const data_t& l, const data_t& r)
	{	return make_tuple<E>(r) < make_tuple<E>(l);
	}

	template<> auto make_tuple<sort_t::name>(const data_t& v)
	{	return std::tuple{ v.name_, discreteness(v), v.call_duration_.average(), tick(v), type(v) };
	}
	template<> auto make_tuple<sort_t::discreteness>(const data_t& v)
	{	return std::tuple{ discreteness(v), v.call_duration_.average(), tick(v), type(v), v.name_ };
	}
	template<> auto make_tuple<sort_t::duration>(const data_t& v)
	{	return std::tuple{ v.call_duration_.average(), discreteness(v), tick(v), type(v), v.name_ };
	}
	template<> auto make_tuple<sort_t::tick>(const data_t& v)
	{	return std::tuple{ tick(v), discreteness(v), v.call_duration_.average(), type(v), v.name_ };
	}
	template<> auto make_tuple<sort_t::type>(const data_t& v)
	{	return std::tuple{ type(v), discreteness(v), v.call_duration_.average(), tick(v), v.name_ };
	}

	std::ostream& operator<<(std::ostream& out, const std::vector<data_t>& data)
	{
		print_itm(out, { "Name", "Discreteness:", "Duration:", "One tick:", "Type:", "%", "%", "%"}) << "\n";

		for (const auto& m : data)
		{
			auto prn_sd = [](double f)
			{
				std::string result = "<err>";
				if (!isnan(f))
				{
					std::ostringstream os;
					os << std::setprecision(1) << std::fixed << misc::round(f, 2);
					result = os.str() + "%";
				}
				return result;
			};

			str_out s =
			{	m.name_,
				to_string(discreteness(m), 3),
				to_string(misc::duration_t{ m.call_duration_.average() }),
				to_string(misc::duration_t{ m.unit_of_currrentthread_work_.average() }),
				type(m),
				prn_sd(m.discreteness_.std_deviation()),
				prn_sd(m.call_duration_.std_deviation()),
				prn_sd(m.unit_of_currrentthread_work_.std_deviation()),
			};
			print_itm(out, s) << "\n";
		}
		return out;
	}

	std::vector<vi_mt::item_t> measurement(const strs_t& inc, const strs_t& exc, const std::function<void(double)>& progress) {
		auto filter = [&inc, &exc](std::string_view s)
			{
				auto pred = [s](const auto& e) { return s.find(e) != std::string::npos; };
				if (std::any_of(exc.begin(), exc.end(), pred))
					return false;
				return inc.empty() || std::any_of(inc.begin(), inc.end(), pred);
			};

		return vi_mt::metric_base_t::action(filter, progress);
	}

	std::vector<data_t> prepare(std::vector<data_t>&& data)
	{
		auto pred = less<sort_t::discreteness>;
		{
			static constexpr bool(*sort_predicates[])(const data_t & l, const data_t & r) = {
				// The order of the elements must match the order of the sort_t enum.
				less<sort_t::name>,
				less<sort_t::discreteness>,
				less<sort_t::duration>,
				less<sort_t::tick>,
				less<sort_t::type>,
			};
			static_assert(std::size(sort_predicates) == static_cast<std::underlying_type_t<sort_t>>(sort_t::_quantity));

			if (const auto pos = static_cast<std::underlying_type_t<sort_t>>(g_sort); pos < std::size(sort_predicates))
				pred = sort_predicates[pos];
		}

		auto result{ std::move(data) };
		std::stable_sort(result.begin(), result.end(), pred);

		return result;
	}

	std::vector<data_t> collect()
	{
		misc::progress_t progress{ "Collecting function properties" };

		std::vector<data_t> result;
		for (int n = 0; n < g_repeat; ++n)
		{
			auto meas = measurement(g_include, g_exclude, [n, &progress](double f) { progress((n + f) / g_repeat); });
			if (result.empty())
				result.resize(meas.size());

			auto action = [](const vi_mt::item_t& s, data_t& d)
			{	assert(d.name_.empty() || d.name_ == s.name_);
				d.name_ = s.name_;
				d.discreteness_.add(s.discreteness_);
				d.call_duration_.add(s.call_duration_.count());
				d.unit_of_allthreads_work_.add(s.unit_of_allthreads_work_.count());
				d.unit_of_currrentthread_work_.add(s.unit_of_currrentthread_work_.count());
				d.unit_of_sleeping_process_.add(s.unit_of_sleeping_process_.count());
				return d;
			};

			std::transform(meas.begin(), meas.end(), result.begin(), result.begin(), action);
		}

		return result;
	}

	void work()
	{
		auto data = prepare(collect());
		std::cout << "\nMeasured properties of time functions:\n" << data << std::endl;
	}
} // namespace measure_functions

std::vector<vi_mt::item_t> vi_mt::metric_base_t::action(const std::function<bool(std::string_view)>& filter, const std::function<void(double)>& pb)
{
	std::vector<item_t> result;
	result.reserve(s_measurers_.size());
	for (std::size_t n = 0; n < s_measurers_.size(); ++n)
	{
		if (const auto& f = s_measurers_[n]; !filter || filter(f.get().name()))
		{
			auto fn = [&pb, n, sz = s_measurers_.size()](double part) {if (pb) pb((static_cast<double>(n) + part) / static_cast<double>(sz)); };
			result.emplace_back(f.get().measurement(fn));
		}
		else if (pb)
		{
			pb(static_cast<double>(n + 1) / static_cast<double>(s_measurers_.size()));
		}
	}
	return result;
}

int main(int argc, char* argv[])
{
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
}
