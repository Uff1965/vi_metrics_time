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

#       ifdef _KERNEL_MODE
    // The intrinsic is available in kernel mode only, and the routine is only available as an intrinsic.
    inline count_t tm_readpmc_intrinsic()
    {   const auto result = __readpmc(0UL);
        return result;
    }
    METRIC("READPMC_INTRINSIC", tm_readpmc_intrinsic);
#       endif

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

// vvv With Data Synchronization Barrier
    inline count_t tm_dsb_mrs()
    {   count_t result;
        asm volatile
        (   "DSB SY\n\t"
            "mrs %0, cntvct_el0"
            : "=r"(result)
        );
        return result;
    }
    METRIC("DSB MRS", tm_dsb_mrs);

    inline count_t tm_mrs_dsb()
    {   count_t result;
        asm volatile
        (   "mrs %0, cntvct_el0\n\t"
            "DSB SY"
            : "=r"(result)
        );
        return result;
    }
    METRIC("MRS DSB", tm_mrs_dsb);

    inline count_t tm_dsb_mrs_dsb()
    {   count_t result;
        asm volatile
        (   "DSB SY\n\t"
            "mrs %0, cntvct_el0\n\t"
            "DSB SY"
            : "=r"(result)
        );
        return result;
    }
    METRIC("DSB MRS DSB", tm_dsb_mrs_dsb);
// ^^^ With Data Synchronization Barrier

// vvv With Instruction Synchronization Barrier
    inline count_t tm_isb_mrs()
    {   count_t result;
        asm volatile
        (   "ISB SY\n\t"
            "mrs %0, cntvct_el0"
            : "=r"(result)
        );
        return result;
    }
    METRIC("ISB MRS", tm_isb_mrs);

    inline count_t tm_mrs_isb()
    {   count_t result;
        asm volatile
        (   "mrs %0, cntvct_el0\n\t"
            "ISB SY"
            : "=r"(result)
        );
        return result;
    }
    METRIC("MRS ISB", tm_mrs_isb);

    inline count_t tm_isb_mrs_isb()
    {   count_t result;
        asm volatile
        (   "ISB SY\n\t"
            "mrs %0, cntvct_el0\n\t"
            "ISB SY"
            : "=r"(result)
        );
        return result;
    }
    METRIC("ISB MRS ISB", tm_isb_mrs_isb);

    inline count_t tm_dsb_mrs_isb()
    {   count_t result;
        asm volatile
        (   "DSB SY\n\t"
            "mrs %0, cntvct_el0\n\t"
            "ISB SY"
            : "=r"(result)
        );
        return result;
    }
    METRIC("DSB MRS ISB", tm_isb_mrs_isb);
// ^^^ With Instruction Synchronization Barrier

// vvv From vi_timing
	inline count_t vi_tmGetTicks(void) noexcept
	{	count_t result;
		asm volatile
		(	// too slow: "dmb ish\n\t" // Ensure all previous memory accesses are complete before reading the timer
			"isb\n\t" // Ensure the instruction stream is synchronized
			"mrs %0, cntvct_el0\n\t" // Read the current value of the system timer
			"isb\n\t" // Ensure the instruction stream is synchronized again
			: "=r"(result) // Output operand: result will hold the current timer value
			: // No input operands
			: "memory" // Clobber memory to ensure the compiler does not reorder instructions
		);
		return result;
	}
    METRIC("ISB MRS ISB MEM", tm_isb_mrs_isb);
// ^^^ From vi_timing
}
#else
//#   ERROR: You need to define function(s) for your OS and CPU
#endif
