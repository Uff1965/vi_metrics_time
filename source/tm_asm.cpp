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

#ifdef VI_MSC_INTRIN
    extern "C" unsigned long long vi_asm_rdtsc(void);
    extern "C" unsigned long long vi_asm_rdtscp(void);
    extern "C" unsigned long long vi_asm_cpuid_rdtsc(void);
    extern "C" unsigned long long vi_asm_rdtscp_cpuid(void);
    extern "C" unsigned long long vi_asm_rdtscp_lfence(void);
    extern "C" unsigned long long vi_asm_mfence_lfence_rdtsc(void);
#endif

namespace vi_mt
{
#ifdef VI_MSC_INTRIN
#   pragma intrinsic(__rdtsc, __rdtscp, _mm_lfence, _mm_sfence, _mm_mfence)

    inline count_t tm_rdtsc() {
        return __rdtsc();
    }
    METRIC("RDTSC_INTRINSIC", tm_rdtsc);

    inline count_t tm_rdtscp() {
        unsigned int aux;
        return __rdtscp(&aux);
    }
    METRIC("RDTSCP_INTRINSIC", tm_rdtsc);

    inline count_t tm_rdtsc_cpuid() {
        int cpuInfo[4];
        __cpuid(cpuInfo, 0);
        return __rdtsc();
    }
    METRIC("CPUID+RDTSC_INTRINSIC", tm_rdtsc_cpuid);

    inline count_t tm_rdtscp_cpuid() {
        unsigned int aux;
        const auto result = __rdtscp(&aux);
        int cpuInfo[4];
        __cpuid(cpuInfo, 0);
        return result;
    }
    METRIC("RDTSCP+CPUID_INTRINSIC", tm_rdtscp_cpuid);

    inline count_t tm_rdtsc_seq_cst() {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const auto result = __rdtsc();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return result;
    }
    METRIC("seq_cst+RDTSC+seq_cst", tm_rdtsc_seq_cst);

    inline count_t tm_rdtsc_acq_rel() {
        std::atomic_thread_fence(std::memory_order_acq_rel);
        const auto result = __rdtsc();
        std::atomic_thread_fence(std::memory_order_acq_rel);
        return result;
    }
    METRIC("acq_rel+RDTSC+acq_rel", tm_rdtsc_acq_rel);

    inline count_t tm_rdtscp_seq_cst() {
        unsigned int aux;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const auto result = __rdtscp(&aux);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return result;
    }
    METRIC("seq_cst+RDTSCP+seq_cst", tm_rdtscp_seq_cst);

    inline count_t tm_rdtscp_acq_rel() {
        unsigned int aux;
        std::atomic_thread_fence(std::memory_order_acq_rel);
        const auto result = __rdtscp(&aux);
        std::atomic_thread_fence(std::memory_order_acq_rel);
        return result;
    }
    METRIC("acq_rel+RDTSCP+acq_rel", tm_rdtscp_acq_rel);

#elif defined(__x86_64__) || defined(__amd64__) // GNU on Intel

	inline count_t vi_asm_rdtsc()
	{   uint64_t low, high;
		__asm__ volatile("rdtsc\n" : "=a" (low), "=d" (high));
		return (high << 32) | low;
	}
	METRIC("RDTSC_ASM", vi_asm_rdtsc);

	inline count_t vi_asm_rdtscp()
	{   uint32_t aux;
        uint64_t low, high;
		__asm__ volatile("rdtscp\n" : "=a" (low), "=d" (high), "=c" (aux));
		return (high << 32) | low;
	}
	METRIC("RDTSCP_ASM", vi_asm_rdtscp);

    inline count_t vi_asm_cpuid_rdtsc()
    {
        uint64_t low, high;
        __asm__ volatile("push rbx");
        __asm__ volatile("xor eax, eax");
        __asm__ volatile("xor ecx, ecx");
        __asm__ volatile("cpuid");
        __asm__ volatile("rdtsc\n" : "=a" (low), "=d" (high));
        __asm__ volatile("pop rbx");
        return (high << 32) | low;
    }
    METRIC("CPUID+RDTSC_ASM", vi_asm_cpuid_rdtsc);

    inline count_t vi_asm_rdtscp_cpuid()
    {
        uint32_t aux;
        uint64_t low, high;
        __asm__ volatile("push rbx");
        __asm__ volatile("rdtscp\n" : "=a" (low), "=d" (high), "=c" (aux));
        __asm__ volatile("xor eax, eax");
        __asm__ volatile("xor ecx, ecx");
        __asm__ volatile("cpuid");
        __asm__ volatile("pop rbx");
        return (high << 32) | low;
    }
    METRIC("RDTSCP+CPUID_ASM", vi_asm_rdtscp_cpuid);

    inline count_t vi_asm_mfence_lfence_rdtsc()
    {
        uint64_t low, high;
        __asm__ volatile("mfence");
        __asm__ volatile("lfence");
        __asm__ volatile("rdtsc\n" : "=a" (low), "=d" (high));
        return (high << 32) | low;
    }
    METRIC("MFENCE+LFENCE+RDTSC_ASM", vi_asm_mfence_lfence_rdtsc);

	inline count_t vi_asm_rdtscp_lfence()
	{   uint32_t aux;
        uint64_t low, high;
		__asm__ volatile("rdtscp\n" : "=a" (low), "=d" (high), "=c" (aux));
        __asm__ volatile("lfence");
		return (high << 32) | low;
	}
	METRIC("RDTSCP+LFENCE_ASM", vi_asm_rdtscp_lfence);

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

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
    METRIC("RDTSC_ASM", vi_asm_rdtsc);
    METRIC("RDTSCP_ASM", vi_asm_rdtscp);
    METRIC("CPUID+RDTSC_ASM", vi_asm_cpuid_rdtsc);
    METRIC("RDTSCP+CPUID_ASM", vi_asm_rdtscp_cpuid);
    METRIC("RDTSCP+LFENCE_ASM", vi_asm_rdtscp_lfence);
    METRIC("MFENCE+LFENCE+RDTSC_ASM", vi_asm_mfence_lfence_rdtsc);
#endif

} // namespace vi_mt
