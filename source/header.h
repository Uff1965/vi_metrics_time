#ifndef TESTS_METRICS_HEADER_H_
#	define TESTS_METRICS_HEADER_H_ 1.0
#	pragma once

#include "misc.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#define chSTR3( a, b ) a##b
#define chSTR4( a, b ) chSTR3( a, b )

namespace vi_mt
{
	namespace ch = std::chrono;
	using namespace std::chrono_literals;
	using count_t = std::uint64_t;

	[[nodiscard]] inline auto now() { return ch::high_resolution_clock::now(); }
	using now_t = std::invoke_result_t<decltype(now)>;

	struct item_t
	{
		misc::duration_t call_duration_; // Duration of the Function call.
		double discreteness_{}; // Average tick growth.
		misc::duration_t unit_of_sleeping_process_; // Duration of one tick in a sleeping process.
		misc::duration_t unit_of_currrentthread_work_; // Duration of one tick when loading current thread.
		misc::duration_t unit_of_allthreads_work_; // Duration of one tick when loading multiple threads.
	};

	using raw_t = std::map<std::string, vi_mt::item_t, std::less<>>;

	struct metric_base_t
	{
		virtual ~metric_base_t() = default;
		virtual std::string_view name() const = 0;
		virtual item_t measurement(const std::function<void(double)>& progress) const = 0;
		metric_base_t() { s_measurers_.emplace_back(std::ref(*this)); }; // Self-registration

		static inline std::vector<std::reference_wrapper<const metric_base_t>> s_measurers_;
		static raw_t action(const std::function<bool(std::string_view)>& filter = {}, const std::function<void(double)>& pb = {});
	};

	template<const char* Name, auto Func, auto... Args>
	class metric_t: protected metric_base_t
	{
		static misc::duration_t measurement_unit_aux(void(*burden)(now_t));
		using tick_t = std::invoke_result_t<decltype(Func), decltype(Args)...>;
		static_assert(std::is_convertible_v<count_t, tick_t>);

		[[nodiscard]] static inline tick_t vi_tmGetTicks()
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
	constexpr auto cache_warmup = 5U;

	template<const char* Name, auto Func, auto... Args>
	double metric_t<Name, Func, Args...>::measurement_discreteness()
	{	auto CNT = 4U;
		tick_t last, first;
		for (;; CNT *= 8)
		{	std::this_thread::yield(); // Reduce the likelihood of interrupting measurements by switching threads.
			const auto limit = now() + 128us;
			for (auto n = 0U; n < cache_warmup; ++n)
			{	last = first = vi_tmGetTicks();
			}

			for (auto cnt = CNT; cnt; )
			{	if (const auto current = vi_tmGetTicks(); current != last)
				{	last = current;
					--cnt;
				}
			}

			if (now() > limit)
			{	break;
			}
		}

		return static_cast<double>(last - first) / static_cast<double>(CNT);
	}

	template<const char* Name, auto Func, auto... Args>
	misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_aux(void(*burden)(now_t))
	{
		auto get_pair = []
		{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			for (auto n = 0U; n < cache_warmup; ++n)
			{	(void)vi_tmGetTicks();
				(void)now();
			}
			auto next = vi_tmGetTicks(); // Preloading a function into cache
			for (const auto prev = vi_tmGetTicks(); prev >= next; )
			{	next = vi_tmGetTicks();
			}

			return std::tuple{ next, now()};
		};

		const auto [tick_b, time_b] = get_pair();
		tick_t tick_e;
		auto time_e = time_b;
		do
		{	burden(time_e + 512ms);
			std::tie(tick_e, time_e) = get_pair();
		} while (tick_e - tick_b < 5 && time_e - time_b < 2s); // For functions with resolution worse than 512ms. For example, for the function time()..

		return (tick_e > tick_b) ? misc::duration_t{ time_e - time_b } / (tick_e - tick_b) : misc::duration_t{};
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_process_sleep()
	{	return measurement_unit_aux([](auto limit) {std::this_thread::sleep_until(limit); });
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_one_thread_work()
	{	return measurement_unit_aux([](auto limit) { while (now() < limit) {/**/} });
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_unit_all_threads_work()
	{
		auto allthreads_load = [](auto limit)
		{	static const auto cnt = (std::thread::hardware_concurrency() > 1) ? (std::thread::hardware_concurrency() - 1) : 0;
			std::atomic_bool done = false;
			std::vector<std::thread> threads(cnt);
			std::generate(threads.begin(), threads.end(), [&done] {return std::thread{ [&done] { while (!done) {/**/ }}};}); //-V776 Potentially infinite loop

			while (now() < limit)
			{/**/}

			done = true;
			std::for_each(threads.begin(), threads.end(), [](auto& t) {t.join(); });
		};

		return  measurement_unit_aux(allthreads_load);
	}

	template<const char* Name, auto Func, auto... Args>
	inline misc::duration_t metric_t<Name, Func, Args...>::measurement_call_duration()
	{
		static constexpr auto CNT = 1'000U;
		volatile tick_t _;

		auto start = [&_]
		{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			for (auto n = 0U; n < cache_warmup; ++n)
			{	(void)vi_tmGetTicks();
				(void)now();
			}
			_ = vi_tmGetTicks(); // Preload

			// Are waiting for the start of a new time interval.
			auto next = now(); // Preload
			for (const auto prev = now(); prev >= next; next = now())
			{/**/}
			return next;
		};

		auto s = start();
		for (unsigned int cnt = 0; cnt < CNT; ++cnt)
		{	_ = vi_tmGetTicks();
		}
		auto e = now();
		const auto base = e - s;

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
		const auto extent = e - s;

		const auto diff = extent - base;
		return misc::duration_t{ std::max(decltype(diff){0}, diff) / static_cast<double>(CNT * CNT_EXT) };
	}

	template<const char* Name, auto Func, auto... Args>
	inline item_t metric_t<Name, Func, Args...>::measurement(const std::function<void(double)>& progress) const
	{	misc::warming(false, misc::g_warming, true);
		item_t result;

		const bool ok = []
			{	const auto tick_s = vi_tmGetTicks();
				auto tick = tick_s;
				for (const auto t = ch::steady_clock::now() + 2s; ch::steady_clock::now() < t && tick <= tick_s; tick = vi_tmGetTicks())
				{
				}
				return tick > tick_s;
			}();
		progress(1.0 / 6.);

		if (ok)
		{	result.unit_of_currrentthread_work_ = measurement_unit_one_thread_work(); // The first, because it can warming the processor.
			progress(2.0 / 6.);
			result.discreteness_ = measurement_discreteness();
			progress(3.0 / 6.);
			result.call_duration_ = measurement_call_duration();
			progress(4.0 / 6.);
			result.unit_of_allthreads_work_ = measurement_unit_all_threads_work();
			progress(5.0 / 6.);
			result.unit_of_sleeping_process_ = measurement_unit_process_sleep(); // The latter, because it can reduce the processor frequency.
		}
		progress(6.0 / 6.);

		return result;
	}
} // namespace vi_mt

#define TM_METRIC(title, ...) \
static const char chSTR4(title_, __LINE__)[] = title; \
template class vi_mt::metric_t<chSTR4(title_, __LINE__), __VA_ARGS__>;

//-V:assert:2570,2528

#endif // #ifndef TESTS_METRICS_HEADER_H_
