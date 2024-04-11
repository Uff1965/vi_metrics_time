// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

// https://chromium.googlesource.com/external/gperftools/+/master/src/base/cycleclock.h

#if defined(_MSC_VER)
#	include <intrin.h>
#endif

#define METRIC(title, ...) TM_METRIC(("<ASM>::" title), __VA_ARGS__)

namespace vi_mt
{
#if defined(_M_X64) || defined(_M_AMD64) // MS compiler on Intel
#   pragma intrinsic(__rdtsc, __rdtscp)

    inline count_t tm_rdtsc() {
        return __rdtsc();
    }
    METRIC("rdtsc", tm_rdtsc);

    inline count_t tm_rdtscp() {
        unsigned int _;
        return __rdtscp(&_);
    }
    METRIC("rdtscp", tm_rdtscp);

#elif defined(__x86_64__) || defined(__amd64__) // GNU on Intel

    inline count_t tm_rdtsc() {
        uint64_t low, high;
        __asm__ volatile("rdtsc\n" : "=a"(low), "=d"(high));
        return (high << 32) | low;
    }
    METRIC("rdtsc", tm_rdtsc);

    inline count_t tm_rdtscp() {
        uint32_t aux;
        uint64_t low, high;
        __asm__ volatile("rdtscp\n" : "=a" (low), "=d" (high), "=c" (aux));
        return (high << 32) | low;
    }
    METRIC("rdtscp", tm_rdtscp);

#elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)

    inline count_t tm_mrs() {
        count_t result;
        asm volatile("mrs %0, cntvct_el0" : "=r"(result));
        return result;
    }
    METRIC("mrs", tm_mrs);

#else
//#   ERROR: You need to define function(s) for your OS and CPU
#endif
} // namespace vi_mt

using ptr_rdtsc_t = vi_mt::count_t(*)();
extern volatile ptr_rdtsc_t ptr_rdtsc = vi_mt::tm_rdtsc;
extern volatile ptr_rdtsc_t ptr_rdtscp = vi_mt::tm_rdtscp;

namespace vi_mt
{
    count_t tm_rdtsc_ptr() {
        return ptr_rdtsc();
    }
    METRIC("rdtsc_ptr", tm_rdtsc_ptr);

    count_t tm_rdtscp_ptr() {
        return ptr_rdtscp();
    }
    METRIC("rdtscp_ptr", tm_rdtscp_ptr);
}
