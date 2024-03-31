// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

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
#	include <io.h> // for _isatty(...)
#	include <profileapi.h> // for QueryPerformanceFrequency(...)
#	include <timeapi.h> // for timeGetDevCaps
#	include <winternl.h> // for NtQueryTimerResolution
#else
#	include <unistd.h> // for isatty(...)
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
	strs_t g_tests;

	bool check_test(std::string_view s)
	{
		return g_tests.empty() || std::any_of(g_tests.begin(), g_tests.end(), [s](const auto& i) {return i == s; });
	}

	inline auto now() { return vi_mt::now(); }

	class progress_t
	{
		const ch::steady_clock::time_point s_{ ch::steady_clock::now() };
		std::string title_;
		void print(double f) const;
		progress_t(const progress_t&) = delete;
		progress_t& operator=(const progress_t&) = delete;
	public:
		explicit progress_t(std::string title) :title_{ std::move(title) } { print(.0); }
		~progress_t() { print(std::numeric_limits<double>::quiet_NaN()); }
		void operator()(double f) const { assert(.0 <= f); print(f); }
	};

	void progress_t::print(double f) const
	{
#if _WIN32
		auto is_atty = [](FILE* pf) { return 0 != _isatty(_fileno(pf)); }; // test whether a file descriptor refers to a terminal
#else
		auto is_atty = [](FILE* pf) { return 0 != isatty(fileno(pf)); };
#endif

		if (is_atty(stdout))
		{
			if (!std::isnan(f))
			{
				std::cout << "\r" << title_ << "... " << std::fixed << std::setprecision(1) << std::setw(3) << 100.0 * f << '%';
			}
			else
			{	// std::isnan(f) - Call from the destructor.
				const auto str = vi_mt::to_string(ch::steady_clock::now() - s_);
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
				const auto str = vi_mt::to_string(ch::steady_clock::now() - s_);
				std::cout << " Done (" << str << ")\n";
			}
		}
	}

	struct space_out : std::numpunct<char> {
		char do_thousands_sep() const override { return '\''; }  // separate with spaces
		std::string do_grouping() const override { return "\3"; } // groups of 1 digit
	};

	void warming(bool all = false, ch::milliseconds d = g_warming)
	{
		if (0 == d.count())
			return;

		progress_t progress{ "Warming" };

		std::atomic_bool done = false;
		const auto cnt = (all && std::thread::hardware_concurrency() > 1) ?
			(std::thread::hardware_concurrency() - 1) :
			0;
		std::vector<std::thread> threads(cnt);
		for (auto& t : threads)
			t = std::thread{ [&done] { while (!done) {}} };

		const auto start = now();
		const auto stop = start + d;
		constexpr auto step = 250ms;
		for (auto temp = start + step; temp < stop; temp += step)
		{
			while (now() < temp) {}
			progress(ch::duration_cast<ch::duration<double>>(temp - start) / d);
		}

		while (now() < stop) {}

		done = true;
		for (auto& t : threads)
			t.join();
	}

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
				{
					std::cerr << "ERROR: Unknown value for parametr --sort: \'" << ptr << "\'\n";
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
			else if ("-t"sv == ptr || "--test"sv == ptr)
			{
				if (n >= argc) continue;
				g_tests.emplace_back(argv[n++]);
			}
			else
			{
				auto error = false;
				if ("-h"sv == ptr && "--help"sv == ptr)
				{
					std::cerr << "ERROR: Unknown parameter \'" << ptr << "\'\n";
					error = true;
				}

				std::cout << "\nOptions:\n" <<
					"-[-h]elp: this help;\n"
					"-[-w]arming 1|0: by default - ON; Implicit - OFF;\n"
					"-[-s]sort name|discreteness|duration|tick|type: by default - discreteness;\n"
					"-[-i]nclude <name>: include function name;\n"
					"-[-e]xclude <name>: exclude function name;\n"
					"-[-t]est <name>: execute test name;\n";

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
			<< std::setw(7) << std::right << vi_mt::duration_t{ ch::system_clock::duration{ 1 } } << ";\n"
			<< "\tThe std::chrono::steady_clock::duration:          "
			<< std::setw(7) << std::right << vi_mt::duration_t{ ch::steady_clock::duration{ 1 } } << ";\n"
			<< "\tThe std::chrono::high_resolution_clock::duration: "
			<< std::setw(7) << std::right << vi_mt::duration_t{ ch::high_resolution_clock::duration{ 1 } } << ";\n";

		std::cout
			<< "\tThe number of clock vi_tmGetTicks per second \'CLOCKS_PER_SEC\': " << CLOCKS_PER_SEC
			<< " (what is equivalent to " << to_string(vi_mt::duration_t{ 1.0 / static_cast<double>(CLOCKS_PER_SEC) }, 3) << ")"
			<< "\n";

#ifdef _WIN32
		if (LARGE_INTEGER frequency; QueryPerformanceFrequency(&frequency))
		{
			std::cout
				<< "\tFrequency of the performance counter \'QueryPerformanceFrequency()\': " << frequency.QuadPart
				<< " (what is equivalent to " << to_string(vi_mt::duration_t{ 1.0 / static_cast<double>(frequency.QuadPart) }, 3) << ");" // counts per second
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
	std::ostream& print_itm(std::ostream& out, std::string_view name, std::string_view disc, std::string_view durn, std::string_view val, std::string_view val_sleep)
	{
		return out << std::left << std::setfill('.')
			<< std::setw(48) << name << ": "
			<< std::right << std::setfill(' ')
			<< std::setw(13) << disc
			<< std::setw(10) << durn
			<< std::setw(10) << val
			<< std::setw(8) << val_sleep;
	}

	std::string_view type(const vi_mt::item_t& itm)
	{
		if (itm.unit_of_currrentthread_work_ / itm.unit_of_allthreads_work_ > 1.2)
			return "Process"sv; // The process-clock is affected by the load on all cores.
		if (itm.unit_of_sleeping_process_ / itm.unit_of_currrentthread_work_ > 1.2)
			return "Thread"sv; // The thread-clock readings are affected only by the thread's load.
		return "Wall"sv; // Wall-clock readings are independent of processor load.
	}

	vi_mt::duration_t tick(const vi_mt::item_t& itm)
	{
		return itm.unit_of_currrentthread_work_;
	}

	vi_mt::duration_t discreteness(const vi_mt::item_t& itm)
	{
		static_assert(std::is_same_v<double, decltype(itm.unit_of_currrentthread_work_)::rep>);
		return vi_mt::duration_t{ itm.unit_of_currrentthread_work_ * itm.discreteness_ };
	}

	template<sort_t E> auto make_tuple(const vi_mt::item_t& v);
	template<sort_t E> bool less(const vi_mt::item_t& l, const vi_mt::item_t& r) {
		return make_tuple<E>(r) < make_tuple<E>(l);
	}

	template<> auto make_tuple<sort_t::name>(const vi_mt::item_t& v) {
		return std::tuple{ v.name_, discreteness(v), v.call_duration_, tick(v), type(v) };
	}
	template<> auto make_tuple<sort_t::discreteness>(const vi_mt::item_t& v) {
		return std::tuple{ discreteness(v), v.call_duration_, tick(v), type(v), v.name_ };
	}
	template<> auto make_tuple<sort_t::duration>(const vi_mt::item_t& v) {
		return std::tuple{ v.call_duration_, discreteness(v), tick(v), type(v), v.name_ };
	}
	template<> auto make_tuple<sort_t::tick>(const vi_mt::item_t& v) {
		return std::tuple{ tick(v), discreteness(v), v.call_duration_, type(v), v.name_ };
	}
	template<> auto make_tuple<sort_t::type>(const vi_mt::item_t& v) {
		return std::tuple{ type(v), discreteness(v), v.call_duration_, tick(v), v.name_ };
	}

	std::ostream& operator<<(std::ostream& out, const std::vector<vi_mt::item_t>& data)
	{
		print_itm(out, "Name", "Discreteness:", "Duration:", "One tick:", "Type:") << "\n";
		for (const auto& m : data)
		{
			print_itm
			(out,
				m.name_,
				to_string(discreteness(m), 2),
				to_string(m.call_duration_),
				to_string(m.unit_of_currrentthread_work_),
				type(m)
			) << "\n";
		}
		return out;
	}

	std::vector<vi_mt::item_t> prepare(std::vector<vi_mt::item_t>&& data)
	{
		auto pred = less<sort_t::discreteness>;
		{
			static constexpr bool(*sort_predicates[])(const vi_mt::item_t & l, const vi_mt::item_t & r) = {
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

	std::vector<vi_mt::item_t> measurement(const strs_t& inc, const strs_t& exc) {
		auto filter = [&inc, &exc](std::string_view s)
			{
				auto pred = [s](const auto& e) { return s.find(e) != std::string::npos; };
				if (std::any_of(exc.begin(), exc.end(), pred))
					return false;
				return inc.empty() || std::any_of(inc.begin(), inc.end(), pred);
			};

		progress_t progress{ "Collecting function properties" };
		return vi_mt::metric_base_t::action(filter, [&progress](double f) { progress(f); });
	}

	void measure_functions()
	{
		warming();
		const auto data = prepare(measurement(g_include, g_exclude));
		std::cout << "\nMeasured properties of time functions:\n" << data << std::endl;
	}
} // namespace measure_functions

int main(int argc, char* argv[])
{
	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	params(argc, argv);
	prefix();

	volatile auto _ = now(); // Preload

	if (check_test("functions"sv))
	{
		endl(std::cout);
		measure_functions::measure_functions();
	}

	endl(std::cout);
	suffix();
}
