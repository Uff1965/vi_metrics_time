#ifndef VI_METRICS_TIME_SOURCE_MISC_H_
#	define VI_METRICS_TIME_SOURCE_MISC_H_
#	pragma once

#	include <cassert>
#	include <chrono>
#	include <iosfwd>
#	include <limits>
#	include <string>
#	include <utility>

namespace misc
{
	namespace ch = std::chrono;

	[[nodiscard]] double round(double num, unsigned char prec = 2, unsigned char dec = 1);
	void warming(bool all = false, ch::milliseconds d = ch::seconds{ 1 }, bool silent = false);

	struct duration_t : ch::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T&& v) : ch::duration<double>{ std::forward<T>(v) } {}
		using ch::duration<double>::duration;
	};

	bool operator<(duration_t l, duration_t r);
	std::string to_string(duration_t sec, unsigned char precision = 2, unsigned char dec = 1);
	inline std::ostream& operator<<(std::ostream& os, const duration_t& d) { return os << misc::to_string(d); }

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
} // namespace misc

#	ifdef _MSC_VER
#		define VI_OPTIMIZE_OFF _Pragma("optimize(\"\", off)")
#		define VI_OPTIMIZE_ON  _Pragma("optimize(\"\", on)")
#	elif defined __GNUC__
#		define VI_OPTIMIZE_OFF _Pragma("GCC push_options") _Pragma("GCC optimize(\"O0\")")
#		define VI_OPTIMIZE_ON  _Pragma("GCC pop_options")
#	else
#		define VI_OPTIMIZE_OFF
#		define VI_OPTIMIZE_ON
#	endif
#endif // #ifndef VI_METRICS_TIME_SOURCE_MISC_H_
