// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

// "RDTSCP versus RDTSC + CPUID" https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid
// "Difference between rdtscp, rdtsc : memory and cpuid / rdtsc?" https://stackoverflow.com/questions/12631856/difference-between-rdtscp-rdtsc-memory-and-cpuid-rdtsc

#define METRIC_ASM(title, ...) TM_METRIC(("<ASM>::" title), __VA_ARGS__)

#define METRIC(title) \
vi_mt::count_t chSTR4(func_, __LINE__)(); \
METRIC_ASM(title, chSTR4(func_, __LINE__)); \
vi_mt::count_t chSTR4(func_, __LINE__)()

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
#	include "tm_masm_x64.h"
#
#	if defined(_M_X64) || defined(_M_AMD64) // MS compiler for x64
#		 include <intrin.h>
#		 ifndef __clang__
#			  pragma intrinsic(__rdtsc, __rdtscp, _mm_lfence, _mm_sfence, _mm_mfence)
#		 endif
#	elif defined(__x86_64__) || defined(__amd64__) // GNU on Intel
#		 include <x86intrin.h>
#		 include <cpuid.h>
#	endif

namespace vi_mt
{
	METRIC_ASM("RDTSC ASM", vi_rdtsc_asm);
	METRIC_ASM("RDTSCP ASM", vi_rdtscp_asm);
	METRIC_ASM("CPUID+RDTSC ASM", vi_cpuid_rdtsc_asm);
	METRIC_ASM("RDTSCP+CPUID ASM", vi_rdtscp_cpuid_asm);
	METRIC_ASM("RDTSCP+LFENCE ASM", vi_rdtscp_lfence_asm);
	METRIC_ASM("LFENCE+RDTSC ASM", vi_lfence_rdtsc_asm);
	METRIC_ASM("MFENCE+LFENCE+RDTSC ASM", vi_mfence_lfence_rdtsc_asm);

	inline count_t tm_rdtsc()
	{	return __rdtsc();
	}
	METRIC_ASM("RDTSC INTRINSIC", tm_rdtsc);

	inline count_t tm_rdtscp()
	{	unsigned int _;
		return __rdtscp(&_);
	}
	METRIC_ASM("RDTSCP INTRINSIC", tm_rdtscp);

	inline count_t vi_lfence_rdtsc()
	{	_mm_lfence();
		return __rdtsc();
	}
	METRIC_ASM("LFENCE+RDTSC INTRINSIC", vi_lfence_rdtsc);

	inline count_t vi_mfence_lfence_rdtsc()
	{	_mm_mfence();
		_mm_lfence();
		return __rdtsc();
	}
	METRIC_ASM("MFENCE+LFENCE+RDTSC INTRINSIC", vi_mfence_lfence_rdtsc);

	inline count_t vi_rdtscp_lfence()
	{	uint32_t _;
		const auto result = __rdtscp(&_);
		_mm_lfence();
		return result;
	}
	METRIC_ASM("RDTSCP+LFENCE INTRINSIC", vi_rdtscp_lfence);

#	if defined(_M_X64) || defined(_M_AMD64) // MSC for x64 or ARM64EC
	inline count_t tm_cpuid_rdtsc()
	{	int _[4];
		__cpuid(_, 0);
		return __rdtsc();
	}
	METRIC_ASM("CPUID+RDTSC INTRINSIC", tm_cpuid_rdtsc);

	inline count_t tm_rdtscp_cpuid()
	{	unsigned int _;
		const auto result = __rdtscp(&_);
		int regs[4];
		__cpuid(regs, 0);
		return result;
	}
	METRIC_ASM("RDTSCP+CPUID INTRINSIC", tm_rdtscp_cpuid);

#		ifdef _KERNEL_MODE
	// The intrinsic is available in kernel mode only, and the routine is only available as an intrinsic.
	inline count_t tm_readpmc_intrinsic()
	{	const auto result = __readpmc(0UL);
		return result;
	}
	METRIC_ASM("READPMC INTRINSIC", tm_readpmc_intrinsic);
#		endif
#	elif defined(__x86_64__) || defined(__amd64__) // GCC/CLANG for x64 or ARM64EC

	inline count_t tm_cpuid_rdtsc()
	{	unsigned int _;
		__cpuid(0, _, _, _, _);
		return __rdtsc();
	}
	METRIC_ASM("CPUID+RDTSC INTRINSIC", tm_cpuid_rdtsc);

	inline count_t tm_rdtscp_cpuid()
	{	unsigned int _;
		const auto result = __rdtscp(&_);
		__cpuid(0, _, _, _, _);
		return result;
	}
	METRIC_ASM("RDTSCP+CPUID INTRINSIC", tm_rdtscp_cpuid);

#	endif // GNU on Intel
} // namespace vi_mt

#elif defined(__aarch64__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 8) // ARMv8 (RaspberryPi4)
namespace vi_mt
{
	inline count_t tm_mrs()
	{	count_t result;
		asm volatile
			(	"mrs %0, cntvct_el0"
			:	"=r"(result)
			);
		return result;
	}
	METRIC_ASM("MRS", tm_mrs);

	inline count_t tm_mrs_mem()
	{	count_t result;
		asm volatile
			(	"mrs %0, cntvct_el0"
			:	"=r"(result)
			:
			: "memory"
			);
		return result;
	}
	METRIC_ASM("MRS+MEM", tm_mrs);

	METRIC("MRS_")
	{	count_t result;
		asm volatile
			(	"mrs %0, cntvct_el0"
			:	"=r"(result)
			);
		return result;
	}

	METRIC("MRS+MEM_")
	{	count_t result;
		asm volatile
			(	"mrs %0, cntvct_el0"
			:	"=r"(result)
			:
			: "memory"
			);
		return result;
	}

	// vvv With Data Synchronization Barrier
	inline count_t tm_dsb_mrs()
	{	count_t result;
		asm volatile
			(	"dsb sy\n\t"
				"mrs %0, cntvct_el0"
			:	"=r"(result)
			:
			:	"memory"
			);
		return result;
	}
	METRIC_ASM("DSB+MRS+MEM", tm_dsb_mrs);

	inline count_t tm_mrs_dsb()
	{	count_t result;
		asm volatile
			("	mrs %0, cntvct_el0\n\t"
				"dsb sy"
			:	"=r"(result)
			:
			:	"memory"
			);
		return result;
	}
	METRIC_ASM("MRS+DSB+MEM", tm_mrs_dsb);

	inline count_t tm_dsb_mrs_dsb()
	{	count_t result;
		asm volatile
			(	"dsb sy\n\t"
				"mrs %0, cntvct_el0\n\t"
				"dsb sy"
			:	"=r"(result)
			:
			:	"memory"
			);
		return result;
	}
	METRIC_ASM("DSB+MRS+DSB+MEM", tm_dsb_mrs_dsb);
	// ^^^ With Data Synchronization Barrier

	// vvv With Instruction Synchronization Barrier
	inline count_t tm_isb_mrs()
	{	count_t result;
		asm volatile
			(	"isb\n\t"
				"mrs %0, cntvct_el0"
			:	"=r"(result)
			:
			:	"memory"
			);
		return result;
	}
	METRIC_ASM("ISB+MRS+MEM", tm_isb_mrs);

	inline count_t tm_mrs_isb()
	{	count_t result;
		asm volatile
			(	"mrs %0, cntvct_el0\n\t"
				"isb"
			:	"=r"(result)
			:
			:	"memory"
			);
		return result;
	}
	METRIC_ASM("MRS+ISB+MEM", tm_mrs_isb);

	inline count_t tm_isb_mrs_isb()
	{	count_t result;
		asm volatile
			(	"isb\n\t"
				"mrs %0, cntvct_el0\n\t"
				"isb"
			:	"=r"(result)
			:
			:	"memory"
			);
		return result;
	}
	METRIC_ASM("ISB+MRS+ISB+MEM", tm_isb_mrs_isb);

	inline count_t tm_dsb_mrs_isb()
	{	count_t result;
		asm volatile
			(	"dsb sy\n\t"
				"mrs %0, cntvct_el0\n\t"
				"isb"
			:	"=r"(result)
			:
			:	"memory"
			);
		return result;
	}
	METRIC_ASM("DSB+MRS+ISB+MEM", tm_dsb_mrs_isb);
	// ^^^ With Instruction Synchronization Barrier
} // namespace vi_mt
#else
//	You need to define function(s) for your OS and CPU
#endif
