// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#ifndef __linux__
#	error "This code is intended for Linux on Raspberry Pi (BCM283x/BCM2711)."
#endif

#if defined(__arm__) && __ARM_ARCH >= 6 // ARMv6 (RaspberryPi 1/2/3/4) 32bit.
#	include <cassert>      // assert
#	include <cstdint>      // uint32_t, uint64_t
#	include <cstring>      // memcpy

#	include <endian.h>     // be32toh
#	include <fcntl.h>
#	include <sys/mman.h>
#	include <sys/stat.h>   // struct stat, fstat
#	include <unistd.h>

#define METRIC(title, ...) TM_METRIC(("<LNX>::" title), __VA_ARGS__)

namespace
{
	using be32_t = std::uint32_t;
	static_assert(sizeof(be32_t) == 4);
	static_assert(sizeof(be32_t) == sizeof(std::uint32_t));
	constexpr auto fail = std::uint32_t(-1);

	std::uint32_t read_cell(const char *path)
	{	const int fd = open(path, O_RDONLY);
		if (fd < 0)
		{	assert(false);
			return fail;
		}
		be32_t be_val;
		constexpr ssize_t need = sizeof(be_val);
		const ssize_t rd = read(fd, &be_val, need);
		close(fd);
		return rd == need? be32toh(be_val): fail;
	}

	std::size_t read_prop_words(const char *path, std::uint32_t *range_entry, std::size_t size)
	{	assert(path && range_entry && size && size % sizeof(be32_t) == 0);
		int fd = open(path, O_RDONLY);
		if (fd < 0)
		{	assert(false);
			return 0;
		}
		// Expect at least one ranges entry; use the first one (Raspberry Pi)
		if ( struct stat st{}; fstat(fd, &st) != 0 || st.st_size < size)
		{	assert(false);
			close(fd);
			return 0;
		}

		const auto rd = read(fd, range_entry, size);
		close(fd);
		if (rd != size)
		{	assert(false);
			return 0;
		}
		return size;
	}

	uint64_t get_peripheral_base()
	{	// address/size-cells for /soc
		const auto soc_addr_cells = read_cell("/proc/device-tree/soc/#address-cells");
		const auto soc_size_cells = read_cell("/proc/device-tree/soc/#size-cells");
		if (soc_addr_cells == fail || soc_size_cells == fail)
		{	assert(false);
			return 0;
		}
		// address-cells of parent (/)
		auto parent_addr_cells = read_cell("/proc/device-tree/#address-cells");
		if (parent_addr_cells == fail)
		{	parent_addr_cells = soc_addr_cells; // fallback для Pi
		}

		std::uint32_t words[8] = {}; // RPi 4 uses 2+2+2=6 cells
		const auto size_cell = (soc_addr_cells + parent_addr_cells + soc_size_cells) * sizeof(be32_t);
		if(size_cell > sizeof(words))
		{	assert(false);
			return 0;
		}

		if (read_prop_words("/proc/device-tree/soc/ranges", words, size_cell) != size_cell)
		{	assert(false);
			return 0;
		}

		for (std::size_t i = 0; i < soc_addr_cells; ++i)
		{	if (be32toh(words[i]) != 0)
			{	assert(false);
				return 0;
			}
		}

		if (parent_addr_cells == 1)
		{	return be32toh(words[soc_addr_cells]);
		}
		else if (parent_addr_cells == 2)
		{	const auto lo = be32toh(words[soc_addr_cells + 1]);
			const auto hi = be32toh(words[soc_addr_cells]);
			return (uint64_t(hi) << 32) | uint64_t(lo);
		}
		assert(false);
		return 0; // Not Raspberry Pi ???
	}

	const volatile uint32_t *map_system_timer()
	{	const auto periph_base = get_peripheral_base();
		if (!periph_base)
		{	assert(false);
			return nullptr;
		}
		const auto page_size = sysconf(_SC_PAGE_SIZE);
		if (page_size <= 0)
		{	assert(false);
			return nullptr;
		}
		assert((page_size & (page_size - 1)) == 0);
		const auto fd = open("/dev/mem", O_RDONLY | O_SYNC);
		if (fd < 0)
		{	assert(false);
			return nullptr;
		}

		constexpr off_t TIMER_OFFSET = 0x3000; // System Timer offset from peripheral base
		const auto timer_phys = static_cast<off_t>(periph_base + TIMER_OFFSET);
		const off_t page_base = timer_phys & ~(static_cast<off_t>(page_size) - 1);

		auto const raw = mmap(nullptr, page_size, PROT_READ, MAP_SHARED, fd, page_base);
		close(fd);

		if (MAP_FAILED != raw)
		{	const std::uint8_t *mapped = static_cast<const std::uint8_t *>(raw);
			return reinterpret_cast<const volatile be32_t *>(mapped + (timer_phys - page_base));
		}
		return nullptr;
	}

	vi_mt::count_t tm_SystemTimer_by_DevMem()
	{	if (static auto const timer_base = map_system_timer())
		{	const auto chi_check = timer_base[2];
			auto clo = timer_base[1]; // Timer low 32 bits
			const auto chi = timer_base[2]; // Timer high 32 bits
			if (chi_check != chi)
			{	clo = timer_base[1];
			}
			return (static_cast<std::uint64_t>(chi) << 32) | static_cast<std::uint64_t>(clo);
		}
		return 0;
	}
	METRIC("SystemTimer_by_DevMem", tm_SystemTimer_by_DevMem);

} // namespace
#endif // #if __ARM_ARCH >= 6
