// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

// Something was peeped in: https://chromium.googlesource.com/external/gperftools/+/master/src/base/cycleclock.h

#if defined(_M_X64) || defined(_M_AMD64) // MS compiler for x64 or ARM64EC
#	include <intrin.h>
#   pragma intrinsic(__rdtsc, __rdtscp, _mm_lfence, _mm_sfence, _mm_mfence)
#elif defined(__x86_64__) || defined(__amd64__) // GNU on Intel
#   include <x86intrin.h>
#   include <cpuid.h>
#endif

#define METRIC(title, ...) TM_METRIC(("<ASM>::" title), __VA_ARGS__)

namespace vi_mt
{
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
    inline count_t tm_rdtsc()
    {   return __rdtsc();
    }
    METRIC("RDTSC_INTRINSIC", tm_rdtsc);

    inline count_t tm_rdtscp()
    {   unsigned int aux;
        return __rdtscp(&aux);
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
    {   uint32_t aux;
        const auto result = __rdtscp(&aux);
        _mm_lfence();
        return result;
    }
    METRIC("RDTSCP+LFENCE_INTRINSIC", vi_rdtscp_lfence);

#   if defined(_M_X64) || defined(_M_AMD64) // MS compiler for x64 or ARM64EC

        inline count_t tm_cpuid_rdtsc()
        {   int cpuInfo[4];
            __cpuid(cpuInfo, 0);
            return __rdtsc();
        }
        METRIC("CPUID+RDTSC_INTRINSIC", tm_cpuid_rdtsc);

        inline count_t tm_rdtscp_cpuid()
        {   unsigned int aux;
            const auto result = __rdtscp(&aux);
            int cpuInfo[4];
            __cpuid(cpuInfo, 0);
            return result;
        }
        METRIC("RDTSCP+CPUID_INTRINSIC", tm_rdtscp_cpuid);

        // Functions from the file 'tm_masm.asm'.
        extern "C" unsigned long long vi_asm_rdtsc(void);
        extern "C" unsigned long long vi_asm_rdtscp(void);
        extern "C" unsigned long long vi_asm_cpuid_rdtsc(void);
        extern "C" unsigned long long vi_asm_rdtscp_cpuid(void);
        extern "C" unsigned long long vi_asm_rdtscp_lfence(void);
        extern "C" unsigned long long vi_asm_lfence_rdtsc(void);
        extern "C" unsigned long long vi_asm_mfence_lfence_rdtsc(void);

#   elif defined(__x86_64__) || defined(__amd64__)

    inline count_t tm_cpuid_rdtsc()
    {   unsigned int _ = 0;
        __cpuid(_, _, _, _, _);
        return __rdtsc();
    }
    METRIC("CPUID+RDTSC_INTRINSIC", tm_cpuid_rdtsc);

    inline count_t tm_rdtscp_cpuid()
    {   unsigned int _;
        const auto result = __rdtscp(&_);
        __cpuid(_, _, _, _, _);
        return result;
    }
    METRIC("RDTSCP+CPUID_INTRINSIC", tm_rdtscp_cpuid);

    inline count_t vi_asm_rdtsc()
        {   uint64_t result;
            __asm__ __volatile__( "rdtsc            \n\t"
                                  "movq %%rax, %0   \n\t"
                                  "salq $32, %%rdx  \n\t"
                                  "orq %%rdx, %0    \n\t"
                                : "=r"(result)
                                :
                                : "%rax", "%rdx"
            );
            return result;
        }

        inline count_t vi_asm_rdtscp()
        {   uint64_t result;
            __asm__ __volatile__( "rdtscp           \n\t"
                                  "movq %%rax, %0   \n\t"
                                  "salq $32, %%rdx  \n\t"
                                  "orq %%rdx, %0    \n\t"
                                : "=r"(result)
                                :
                                : "%rax", "%rcx", "%rdx"
            );
            return result;
        }

        inline count_t vi_asm_cpuid_rdtsc()
        {   uint64_t result;
            __asm__ __volatile__( "xor %%eax, %%eax     \n\t"
                                  "cpuid                \n\t"
                                  "rdtsc                \n\t"
                                  "movq %%rax, %0       \n\t"
                                  "salq $32, %%rdx      \n\t"
                                  "orq %%rdx, %0        \n\t"
                                : "=r"(result)
                                :
                                : "%rax", "%rbx", "%rcx", "%rdx"
                                );
            return result;
        }

        inline count_t vi_asm_rdtscp_cpuid()
        {   uint64_t result;
            __asm__ __volatile__( "rdtscp            \n\t"
                                  "movq %%rax, %0    \n\t"
                                  "salq $32, %%rdx   \n\t"
                                  "orq %%rdx, %0     \n\t"
                                  "xorl %%eax, %%eax \n\t"
                                  "cpuid             \n\t"
                                : "=r"(result)
                                :
                                : "%rax", "%rbx", "%rcx", "%rdx"
                                );
            return result;
        }

        inline count_t vi_asm_rdtscp_lfence()
        {   uint64_t result;
            __asm__ __volatile__( "rdtscp               \n\t"
                                  "lfence               \n\t"
                                  "movq %%rax, %0       \n\t"
                                  "salq $32, %%rdx      \n\t"
                                  "orq %%rdx, %0        \n\t"
                                : "=r"(result)
                                :
                                : "%rax", "rcx", "%rdx"
                                );
            return result;
        }

        inline count_t vi_asm_lfence_rdtsc()
        {   uint64_t result;
            __asm__ __volatile__( "lfence               \n\t"
                                  "rdtsc                \n\t"
                                  "movq %%rax, %0       \n\t"
                                  "salq $32, %%rdx      \n\t"
                                  "orq %%rdx, %0        \n\t"
                                : "=r"(result)
                                :
                                : "%rax", "%rdx"
                                );
            return result;
        }

        inline count_t vi_asm_mfence_lfence_rdtsc()
        {   uint64_t result;
            __asm__ __volatile__( "mfence               \n\t"
                                  "lfence               \n\t"
                                  "rdtsc                \n\t"
                                  "movq %%rax, %0       \n\t"
                                  "salq $32, %%rdx      \n\t"
                                  "orq %%rdx, %0        \n\t"
                                : "=r"(result)
                                :
                                : "%rax", "%rdx"
                                );
            return result;
        }

/* The RDPMC instruction can only be executed at privilege level 0.
        inline count_t vi_rdpmc_instructions()
        {   return __rdpmc(1UL << 30);
        }
        METRIC("RDPMC_INSTRUCTIONS_INTRINSIC", vi_rdpmc_instructions);

        inline count_t vi_rdpmc_actual_cycles()
        {   return __rdpmc((1UL << 30) + 1U);
        }
        METRIC("RDPMC_ACTUAL_CYCLES_INTRINSIC", vi_rdpmc_actual_cycles);

        inline count_t vi_rdpmc_reference_cycles()
        {   return __rdpmc((1UL << 30) + 2U);
        }
        METRIC("RDPMC_REFERENCE_CYCLES_INTRINSIC", vi_rdpmc_reference_cycles);

        inline count_t vi_asm_rdpmc_instructions()
        {   uint64_t low, high;
            uint32_t ecx = 1U << 30;
            __asm __volatile("rdpmc" : "=a" (low), "=d" (high) : "c"(ecx));
            return (high << 32) | low;
        }
        METRIC("RDPMC_INSTRUCTIONS_ASM", vi_asm_rdpmc_instructions);

        inline count_t vi_asm_rdpmc_actual_cycles()
        {   uint64_t low, high;
            uint32_t ecx = (1U << 30) + 1;
            __asm __volatile("rdpmc" : "=a" (low), "=d" (high) : "c"(ecx));
            return (high << 32) | low;
        }
        METRIC("RDPMC_ACTUAL_CYCLES_ASM", vi_asm_rdpmc_actual_cycles);

        inline count_t vi_asm_rdpmc_reference_cycles()
        {   uint64_t low, high;
            uint32_t ecx = (1U << 30) + 1;
            __asm __volatile("rdpmc" : "=a" (low), "=d" (high) : "c"(ecx));
            return (high << 32) | low;
        }
        METRIC("RDPMC_REFERENCE_CYCLES_ASM", vi_asm_rdpmc_reference_cycles);
    */
#   endif // GNU on Intel

    METRIC("RDTSC_ASM", vi_asm_rdtsc);
    METRIC("RDTSCP_ASM", vi_asm_rdtscp);
    METRIC("CPUID+RDTSC_ASM", vi_asm_cpuid_rdtsc);
    METRIC("RDTSCP+CPUID_ASM", vi_asm_rdtscp_cpuid);
    METRIC("RDTSCP+LFENCE_ASM", vi_asm_rdtscp_lfence);
    METRIC("LFENCE+RDTSC_ASM", vi_asm_lfence_rdtsc);
    METRIC("MFENCE+LFENCE+RDTSC_ASM", vi_asm_mfence_lfence_rdtsc);

#elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)

    inline count_t tm_mrs()
    {   count_t result;
        asm __volatile__("mrs %0, cntvct_el0" : "=r"(result));
        return result;
    }
    METRIC("MRS", tm_mrs);

#else
//#   ERROR: You need to define function(s) for your OS and CPU
#endif

} // namespace vi_mt
