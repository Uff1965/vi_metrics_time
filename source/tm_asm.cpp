// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <atomic>

// Something was peeped in: https://chromium.googlesource.com/external/gperftools/+/master/src/base/cycleclock.h

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64)) // MS compiler for x64 or ARM64EC
#	include <intrin.h>
#	define VI_MSC_INTRIN
#endif

#define METRIC(title, ...) TM_METRIC(("<ASM>::" title), __VA_ARGS__)

namespace vi_mt
{
#ifdef VI_MSC_INTRIN
#   pragma intrinsic(__rdtsc, __rdtscp, _mm_lfence, _mm_sfence, _mm_mfence)

    inline count_t tm_rdtsc_cpuid() {
        int _[4];
        __cpuid(_, 0);
        return __rdtsc();
    }
    METRIC("RDTSC & CPUID", tm_rdtsc_cpuid);

    inline count_t tm_rdtsc() {
        return __rdtsc();
    }
    METRIC("RDTSC", tm_rdtsc);

    inline count_t tm_rdtsc_seq_cst() {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto result = __rdtsc();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return result;
    }
    METRIC("RDTSC & seq_cst", tm_rdtsc_seq_cst);

    inline count_t tm_rdtsc_acq_rel() {
        std::atomic_thread_fence(std::memory_order_acq_rel);
        auto result = __rdtsc();
        std::atomic_thread_fence(std::memory_order_acq_rel);
        return result;
    }
    METRIC("RDTSC & acq_rel", tm_rdtsc_acq_rel);

    inline count_t tm_rdtscp() {
        unsigned int _;
        return __rdtscp(&_);
    }
    METRIC("RDTSCP", tm_rdtscp);

    inline count_t tm_rdtscp_seq_cst() {
        unsigned int _;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return __rdtscp(&_);
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    METRIC("RDTSCP & seq_cst", tm_rdtscp_seq_cst);

    inline count_t tm_rdtscp_acq_rel() {
        unsigned int _;
        std::atomic_thread_fence(std::memory_order_acq_rel);
        return __rdtscp(&_);
        std::atomic_thread_fence(std::memory_order_acq_rel);
    }
    METRIC("RDTSCP & acq_rel", tm_rdtscp_acq_rel);

#elif defined(__x86_64__) || defined(__amd64__) // GNU on Intel

    //inline count_t tm_rdtsc() {
    //    uint64_t low, high;
    //    __asm__ volatile("rdtsc\n" : "=a"(low), "=d"(high));
    //    return (high << 32) | low;
    //}
    //METRIC("rdtsc", tm_rdtsc);

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
