// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

// "RDTSCP versus RDTSC + CPUID" https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid
// "Difference between rdtscp, rdtsc : memory and cpuid / rdtsc?" https://stackoverflow.com/questions/12631856/difference-between-rdtscp-rdtsc-memory-and-cpuid-rdtsc

#define METRIC(title, ...) TM_METRIC(("<ASM>::" title), __VA_ARGS__)

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
#   include "tm_masm_x64.h"

#   if defined(_M_X64) || defined(_M_AMD64) // MS compiler for x64 or ARM64EC
#       include <intrin.h>
#       ifndef __clang__
#           pragma intrinsic(__rdtsc, __rdtscp, _mm_lfence, _mm_sfence, _mm_mfence)
#       endif
#   elif defined(__x86_64__) || defined(__amd64__) // GNU on Intel
#       include <x86intrin.h>
#       include <cpuid.h>
#   endif

namespace vi_mt
{
    METRIC("RDTSC_ASM", vi_rdtsc_asm);
    METRIC("RDTSCP_ASM", vi_rdtscp_asm);
    METRIC("CPUID+RDTSC_ASM", vi_cpuid_rdtsc_asm);
    METRIC("RDTSCP+CPUID_ASM", vi_rdtscp_cpuid_asm);
    METRIC("RDTSCP+LFENCE_ASM", vi_rdtscp_lfence_asm);
    METRIC("LFENCE+RDTSC_ASM", vi_lfence_rdtsc_asm);
    METRIC("MFENCE+LFENCE+RDTSC_ASM", vi_mfence_lfence_rdtsc_asm);

    inline count_t tm_rdtsc()
    {   return __rdtsc();
    }
    METRIC("RDTSC_INTRINSIC", tm_rdtsc);

    inline count_t tm_rdtscp()
    {   unsigned int _;
        return __rdtscp(&_);
    }
    METRIC("RDTSCP_INTRINSIC", tm_rdtscp);

    inline count_t vi_lfence_rdtsc()
    {   _mm_lfence();
        return __rdtsc();
    }
    METRIC("LFENCE+RDTSC_INTRINSIC", vi_lfence_rdtsc);

    inline count_t vi_mfence_lfence_rdtsc()
    {   _mm_mfence();
        _mm_lfence();
        return __rdtsc();
    }
    METRIC("MFENCE+LFENCE+RDTSC_INTRINSIC", vi_mfence_lfence_rdtsc);

    inline count_t vi_rdtscp_lfence()
    {   uint32_t _;
        const auto result = __rdtscp(&_);
        _mm_lfence();
        return result;
    }
    METRIC("RDTSCP+LFENCE_INTRINSIC", vi_rdtscp_lfence);

#   if defined(_M_X64) || defined(_M_AMD64) // MS compiler for x64 or ARM64EC

    inline count_t tm_cpuid_rdtsc()
    {   int _[4];
        __cpuid(_, 0);
        return __rdtsc();
    }
    METRIC("CPUID+RDTSC_INTRINSIC", tm_cpuid_rdtsc);

    inline count_t tm_rdtscp_cpuid()
    {   unsigned int _;
        const auto result = __rdtscp(&_);
        int __[4];
        __cpuid(__, 0);
        return result;
    }
    METRIC("RDTSCP+CPUID_INTRINSIC", tm_rdtscp_cpuid);

#   elif defined(__x86_64__) || defined(__amd64__)

    inline count_t tm_cpuid_rdtsc()
    {   unsigned int _;
        __cpuid(0, _, _, _, _);
        return __rdtsc();
    }
    METRIC("CPUID+RDTSC_INTRINSIC", tm_cpuid_rdtsc);

    inline count_t tm_rdtscp_cpuid()
    {   unsigned int _;
        const auto result = __rdtscp(&_);
        __cpuid(0, _, _, _, _);
        return result;
    }
    METRIC("RDTSCP+CPUID_INTRINSIC", tm_rdtscp_cpuid);

#   endif // GNU on Intel
} // namespace vi_mt

#elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
namespace vi_mt
{
    inline count_t tm_mrs()
    {   count_t result;
        __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(result));
        return result;
    }
    METRIC("MRS", tm_mrs);

    inline count_t tm_dsb_mrs_dsb()
    {   count_t result;
        asm volatile("DSB" ::: "memory");
        asm volatile("mrs %0, cntvct_el0" : "=r"(result));
        asm volatile("DSB" ::: "memory");
        return result;
    }
    METRIC("DSB MRS DSB", tm_dsb_mrs_dsb);

    inline count_t tm_mrs_dsb()
    {   count_t result;
        asm volatile("DSB" ::: "memory");
        asm volatile("mrs %0, cntvct_el0" : "=r"(result));
        return result;
    }
    METRIC("DSB MRS", tm_mrs_dsb);
}
#else
//#   ERROR: You need to define function(s) for your OS and CPU
#endif
