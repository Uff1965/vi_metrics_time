#ifndef VI_METRICS_TIME_SOURCE_TM_MASM_H_
#	define VI_METRICS_TIME_SOURCE_TM_MASM_H_
#	pragma once

// "Встроенный ассемблер GCC" https://av-assembler.ru/asm/high-level-languages/assembler-gcc.php

#	include <cstdint>

#   if defined(_M_X64) || defined(_M_AMD64) // MS compiler for x64

// MSVC for x64 does not support inline assembler, so the functions are defined in a separate 'tm_masm_x64.asm'.
	extern "C" uint64_t vi_rdtsc_asm(void);
	extern "C" uint64_t vi_rdtscp_asm(void);
	extern "C" uint64_t vi_cpuid_rdtsc_asm(void);
	extern "C" uint64_t vi_rdtscp_cpuid_asm(void);
	extern "C" uint64_t vi_rdtscp_lfence_asm(void);
	extern "C" uint64_t vi_lfence_rdtsc_asm(void);
	extern "C" uint64_t vi_mfence_lfence_rdtsc_asm(void);

#   elif defined(__x86_64__) || defined(__amd64__) // GNU on x64
// In GCC, we use the built-in assembler.
	inline uint64_t vi_rdtsc_asm()
	{   uint64_t low, high;
		__asm__ __volatile__("rdtsc" 
							: "=a"(low), "=d"(high)
							);
		return (high << 32) | low;
	}

	inline uint64_t vi_rdtscp_asm()
	{   uint64_t low, high;
		__asm__ __volatile__("rdtscp" 
							: "=a"(low), "=d"(high) 
							:
							: "%rcx"
							);
		return (high << 32) | low;
	}

	inline uint64_t vi_cpuid_rdtsc_asm()
	{   uint64_t low, high;
		__asm__ __volatile__("cpuid; rdtsc" 
							: "=a"(low), "=d"(high) 
							: "a"(0) 
							: "%rbx", "%rcx", "memory"
							);
		return (high << 32) | low;
	}

	inline uint64_t vi_rdtscp_cpuid_asm()
	{   uint64_t result;
		__asm__ __volatile__("rdtscp              \n\t"
							"movq %%rax, %0       \n\t"
							"salq $32, %%rdx      \n\t"
							"orq %%rdx, %0        \n\t"
							"xorl %%eax, %%eax    \n\t"
							"cpuid                \n\t"
							: "=r"(result)
							:
							: "%rax", "%rbx", "%rcx", "%rdx", "memory"
							);
		return result;
	}

	inline uint64_t vi_rdtscp_lfence_asm()
	{   uint64_t low, high;
		__asm__ __volatile__("rdtscp; lfence"
							: "=a"(low), "=d"(high) 
							:
							: "%rcx", "memory"
							);
		return (high << 32) | low;
	}

	inline uint64_t vi_lfence_rdtsc_asm()
	{   uint64_t low, high;
		__asm__ __volatile__("lfence; rdtsc" 
							: "=a"(low), "=d"(high) 
							);
		return (high << 32) | low;
	}

	inline uint64_t vi_mfence_lfence_rdtsc_asm()
	{   uint64_t low, high;
		__asm__ __volatile__("mfence; lfence; rdtsc" 
							: "=a"(low), "=d"(high) 
							:
							: "memory"
							);
		return (high << 32) | low;
	}

#   endif
#endif // #ifndef VI_METRICS_TIME_SOURCE_TM_MASM_H_
