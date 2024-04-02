#ifndef TESTS_METRICS_HEADER_H_
#	define TESTS_METRICS_HEADER_H_ 1.0
#	pragma once

#include "misc.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#ifdef _MSC_VER
#	define VI_OPTIMIZE_OFF _Pragma("optimize(\"\", off)")
#	define VI_OPTIMIZE_ON  _Pragma("optimize(\"\", on)")
#elif defined __GNUC__
#	define VI_OPTIMIZE_OFF _Pragma("GCC push_options") _Pragma("GCC optimize(\"O0\")")
#	define VI_OPTIMIZE_ON  _Pragma("GCC pop_options")
#else
#	define VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_ON
#endif

namespace vi_mt
{
	namespace ch = std::chrono;
	using count_t = std::uint64_t;

	[[nodiscard]] inline auto now() { return ch::high_resolution_clock::now(); }
	using now_t = decltype(now());

	struct item_t
	{
		std::string name_;
		misc::duration_t call_duration_; // Duration of the Function call.
		double discreteness_{}; // Average tick growth.
		misc::duration_t unit_of_sleeping_process_; // Duration of one tick in a sleeping process.
		misc::duration_t unit_of_currrentthread_work_; // Duration of one tick when loading current thread.
		misc::duration_t unit_of_allthreads_work_; // Duration of one tick when loading multiple threads.
	};

	struct metric_base_t
	{
		virtual std::string_view name() const = 0;
		virtual item_t measurement(const std::function<void(double)>& progress) const = 0;
		metric_base_t() { s_measurers_.emplace_back(std::ref(*this)); }; // Self-registration

		static inline std::vector< std::reference_wrapper<const metric_base_t>> s_measurers_;
		static std::vector<item_t> action(const std::function<bool(std::string_view)>& filter = {}, const std::function<void(double)>& pb = {});
	};

	template<const char* Name, auto Func, auto... Args>
	class metric_t: protected metric_base_t
	{
		static misc::duration_t measurement_unit_aux(void(*burden)(now_t));

		[[nodiscard]] static inline auto vi_tmGetTicks()
		{	return Func(Args...);
		}

		static misc::duration_t measurement_call_duration();
		static double measurement_discreteness();
		static misc::duration_t measurement_unit_process_sleep();
		static misc::duration_t measurement_unit_one_thread_work();
		static misc::duration_t measurement_unit_all_threads_work();

		std::string_view name() const override { return Name; }
		item_t measurement(const std::function<void(double)> &pb) const override;

		static const metric_t s_inst_;
	};

	template<const char* Name, auto Func, auto... Args>
	inline const metric_t<Name, Func, Args...> metric_t<Name, Func, Args...>::s_inst_;

	template<const char* Name, auto Func, auto... Args>
	double metric_t<Name, Func, Args...>::measurement_discreteness()
	{	constexpr auto CNT = 5U;
		std::this_thread::yield(); // Reduce the likelihood of interrupting measurements by switching threads.
		const auto first = vi_tmGetTicks();
		auto last = first;
		for (auto cnt = CNT; cnt; )
		{	if (const auto c = vi_tmGetTicks(); c != last)
			{	last = c;
				--cnt;
			}
		}
		return static_cast<double>(last - first) / static_cast<double>(CNT);
	}

	template<const char* Name, auto Func, auto... Args>
	misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_aux(void(*burden)(now_t))
	{
		using tick_t = std::invoke_result_t<decltype(vi_tmGetTicks)>;
		auto get_pair = []
		{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			(void)vi_tmGetTicks(); // Preloading a function into cache
			(void)now(); // Preloading a function into cache

			auto next = vi_tmGetTicks();
			for (const auto prev = vi_tmGetTicks(); prev >= next; )
			{	next = vi_tmGetTicks();
			}

			return std::tuple{ next, now()};
		};

		const auto [tick_b, time_b] = get_pair();
		tick_t tick_e;
		auto time_e = time_b;
		do
		{	burden(time_e + ch::milliseconds{ 512 });
			std::tie(tick_e, time_e) = get_pair();
		} while (tick_e - tick_b < 5); // For functions with resolution worse than 512ms. For example, for the function time()..

		return misc::duration_t{ time_e - time_b } / (tick_e - tick_b);
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_process_sleep()
	{	const auto result = measurement_unit_aux([](auto limit) {std::this_thread::sleep_until(limit); });

		// Naive are trying to wake up the processor core after Sleep.
		for (const auto tp = now() + ch::milliseconds{ 500 }; now() < tp; )
		{/**/
		}

		return result;
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_one_thread_work()
	{	return measurement_unit_aux([](auto limit) { while (now() < limit) {/**/} });
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_all_threads_work()
	{
		auto allthreads_load = [](auto limit) {
			static const auto cnt = (std::thread::hardware_concurrency() > 1) ? (std::thread::hardware_concurrency() - 1) : 0;
			std::vector<std::thread> threads(cnt);
			std::atomic_bool done = false;

			for (auto& t : threads)
				t = std::thread([&done] { while (!done) {/**/} });
			while (now() < limit) {/**/}

			done = true;
			for (auto& t : threads)
				t.join();
		};

		return  measurement_unit_aux(allthreads_load);
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_call_duration()
	{
		static constexpr auto CNT = 100U;
		volatile std::invoke_result_t<decltype(vi_tmGetTicks)> _;

		auto start = [&_]
		{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			_ = vi_tmGetTicks(); // Preload
			// Are waiting for the start of a new time interval.
			auto next = now();
			for (const auto prev = next; next <= prev; )
			{	next = now();
			}
			return next;
		};

		auto s = start();
		for (unsigned int cnt = 0; cnt < CNT; ++cnt)
		{	_ = vi_tmGetTicks();
		}
		auto e = now();
		const auto diff1 = e - s;

		static constexpr auto CNT_EXT = 20U;
		s = start();
		for (unsigned int cnt = 0; cnt < CNT; ++cnt)
		{	_ = vi_tmGetTicks(); //-V761

			// And CNT_EXT more calls.
			_ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks();
			_ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks();
			_ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks();
			_ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks(); _ = vi_tmGetTicks();
		}
		e = now();
		const auto diff2 = e - s;

		assert(diff2 > diff1);
		const auto diff = diff2 - diff1;
		return misc::duration_t{ std::max(decltype(diff){0}, diff) / static_cast<double>(CNT * CNT_EXT) };
	}

	template<const char* Name, auto Func, auto... Args>
	inline item_t metric_t<Name, Func, Args...>::measurement(const std::function<void(double)>& progress) const
	{
		misc::warming(false, ch::seconds{ 1 }, true);

		item_t result{ Name };
		[[maybe_unused]] volatile auto _ = vi_tmGetTicks(); // A naive attempt to load the function code into the processor cache before starting measurements.
		result.call_duration_ = measurement_call_duration();
		progress(1.0 / 5.);
		result.discreteness_ = measurement_discreteness();
		progress(2.0 / 5.);
		result.unit_of_sleeping_process_ = measurement_unit_process_sleep();
		progress(3.0 / 5.);
		result.unit_of_currrentthread_work_ = measurement_unit_one_thread_work();
		progress(4.0 / 5.);
		result.unit_of_allthreads_work_ = measurement_unit_all_threads_work();
		progress(5.0 / 5.);
		return result;
	}
} // namespace vi_mt

#define chSTR3( a, b ) a##b
#define chSTR4( a, b ) chSTR3( a, b )

#define TM_METRIC(title, ...) \
static const char chSTR4(title_, __LINE__)[] = title; \
template class vi_mt::metric_t<chSTR4(title_, __LINE__), __VA_ARGS__>;

//-V:assert:2570,2528

#endif // #ifndef TESTS_METRICS_HEADER_H_
