#ifndef VI_METRICS_TIME_SOURCE_MISC_H_
#	define VI_METRICS_TIME_SOURCE_MISC_H_
#	pragma once

#	include <algorithm>
#	include <array>
#	include <cassert>
#	include <chrono>
#	include <iosfwd>
#	include <limits>
#	include <string>
#	include <utility>

namespace misc
{
	namespace ch = std::chrono;

	extern ch::milliseconds g_warming;

	[[nodiscard]] double round(double num, unsigned char prec = 2, unsigned char dec = 1);
	void warming(bool all = false, ch::milliseconds d = g_warming, bool silent = false);

	struct duration_t : ch::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T&& v) : ch::duration<double>{ std::forward<T>(v) } {}
		using ch::duration<double>::duration;
	};

	bool operator<(duration_t l, duration_t r);
	std::string to_string(duration_t sec, unsigned char precision = 3, unsigned char dec = 1);
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

	template<typename Key, typename T>
	class emap_t
	{
	public:
		using key_type = Key;
		using mapped_type = T;
		using value_type = T;
		using size_type = std::size_t;
		using reference = T&;
		using const_reference = const T&;

#	ifdef __cpp_lib_to_underlying // C++23?
		constexpr static auto to_underlying(key_type e) noexcept { return std::to_underlying<key_type>(e); }
#	else
		constexpr static auto to_underlying(key_type e) noexcept { return static_cast<std::underlying_type_t<key_type>>(e); }
#	endif

		constexpr emap_t(std::initializer_list<std::pair<key_type, mapped_type>> values)
		{	assert(values.size() == N);
			for (auto &&p : values)
			{	assert(to_underlying(p.first) >= 0 && to_underlying(p.first) < N);
				data_[to_underlying(p.first)] = p.second;
			}
		}
		constexpr emap_t(std::initializer_list<mapped_type> values)
		{	assert(values.size() == N);
			std::copy(std::begin(values), std::end(values), std::begin(data_));
		}
		[[nodiscard]] constexpr static size_type size() noexcept { return N; }
		[[nodiscard]] constexpr reference operator[](key_type key) { return data_[to_underlying(key)]; }
		[[nodiscard]] constexpr const_reference operator[](key_type key) const { return data_[to_underlying(key)]; }
		[[nodiscard]] constexpr reference at(key_type key)
		{	if(const auto n = to_underlying(key); index_check(n))
				return data_[n];
			throw std::out_of_range("invalid emap_t<K, T> key");
		}
		[[nodiscard]] constexpr const_reference at(key_type key) const
		{	if(const auto n = to_underlying(key); index_check(n))
				return data_[n];
			throw std::out_of_range("invalid emap_t<K, T> key");
		}

	private:
		constexpr static size_type N = to_underlying(key_type::_quantity);
		constexpr static bool index_check(size_type index) noexcept { return index >= 0 && index < N; }
		mapped_type data_[N];

	}; // class emap_t

	template<typename Key, typename... Args>
	[[nodiscard]] constexpr auto make_emap( Args&&... args)
	{	using T = std::common_type_t<Args...>;
		return emap_t<Key, T>{ std::initializer_list<T>{std::forward<Args>(args)...} };
	}

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
