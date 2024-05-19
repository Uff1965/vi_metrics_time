; https://metanit.com/assembler/tutorial/12.1.php
; https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid

option casemap:none ; Директива option указывает MASM сделать все символы чувствительными к регистру.

.data
;	text byte "Hello from MASM!", 10, 0 ; определяем выводимые данные

.code
;	externdef printf:proc ; подключаем определение функции printf() из C/C++

;оператор public указывает, что функция будет видна вне исходного/объектного файла MASM.

;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
; Clear functions:
;Intel® 64 and IA-32 Architectures Software Developer’s Manual. Vol. 2B 4-551
;RDTSC—Read Time-Stamp Counter:
;The RDTSC instruction is not a serializing instruction. It does not necessarily wait until all previous instructions have been executed before reading the counter. Similarly, subsequent instructions may begin execution before the read operation is performed. The following items may guide software seeking to order executions of RDTSC:
;	*If software requires RDTSC to be executed only after all previous instructions have executed and all previous loads are globally visible,1 it can execute LFENCE immediately before RDTSC.
;	*If software requires RDTSC to be executed only after all previous instructions have executed and all previous loads and stores are globally visible, it can execute the sequence MFENCE;LFENCE immediately before RDTSC.
;	*If software requires RDTSC to be executed prior to execution of any subsequent instruction (including any memory accesses), it can execute the sequence LFENCE immediately after RDTSC.

;Intel® 64 and IA-32 Architectures Software Developer’s Manual. Vol. 2B 4-553
;RDTSCP—Read Time-Stamp Counter and Processor ID:
;"The RDTSCP instruction is not a serializing instruction, but it does wait until all previous instructions have executed and all previous loads are globally visible. But it does not wait for previous stores to be globally visible, and subse-quent instructions may begin execution before the read operation is performed. The following items may guide software seeking to order executions of RDTSCP:
;	*If software requires RDTSCP to be executed only after all previous stores are globally visible, it can execute MFENCE immediately before RDTSCP.
;	*If software requires RDTSCP to be executed prior to execution of any subsequent instruction (including any memory accesses), it can execute LFENCE immediately after RDTSCP"

public vi_rdtsc_asm, vi_rdtscp_asm

vi_rdtsc_asm PROC
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_rdtsc_asm ENDP

vi_rdtscp_asm PROC
	rdtscp
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_rdtscp_asm ENDP
;^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
; Functions using CPUID:
;Intel® 64 and IA-32 Architectures Software Developer’s Manual. Vol. 3A 9-18:
;"Non-privileged serializing instructions — CPUID, IRET, RSM, and SERIALIZE
;When the processor serializes instruction execution, it ensures that all pending memory transactions are completed 
;(including writes stored in its store buffer) before it executes the next instruction. Nothing can pass a serializing 
;instruction and a serializing instruction cannot pass any other instruction (read, write, instruction fetch, or I/O). 
;For example, CPUID can be executed at any privilege level to serialize instruction execution with no effect on program 
;flow, except that the EAX, EBX, ECX, and EDX registers are modified."

public vi_cpuid_rdtsc_asm, vi_rdtscp_cpuid_asm

vi_cpuid_rdtsc_asm PROC
	push	rbx

	xor	eax, eax
	cpuid

	rdtsc
	shl	rdx, 32
	or	rax, rdx

	pop	rbx
	ret	0
vi_cpuid_rdtsc_asm ENDP

vi_rdtscp_cpuid_asm PROC
	push	rbx

	rdtscp
	shl	rdx, 32
	or	rax, rdx
	mov	r8, rax

	xor	eax, eax
	cpuid

	mov	rax, r8

	pop	rbx
	ret	0
vi_rdtscp_cpuid_asm ENDP
;^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
; Functions using FENCE:
;Intel® 64 and IA-32 Architectures Software Developer’s Manual. Vol. 2A 3-603
;LFENCE—Load Fence:
;"Performs a serializing operation on all load-from-memory instructions that were issued prior the LFENCE instruction.
;Specifically, LFENCE does not execute until all prior instructions have completed locally, and no later instruction
;begins execution until LFENCE completes. In particular, an instruction that loads from memory and that
;precedes an LFENCE receives data from memory prior to completion of the LFENCE. (An LFENCE that follows an
;instruction that stores to memory might complete before the data being stored have become globally visible.)
;Instructions following an LFENCE may be fetched from memory before the LFENCE, but they will not execute (even
;speculatively) until the LFENCE completes.
;Weakly ordered memory types can be used to achieve higher processor performance through such techniques as
;out-of-order issue and speculative reads. The degree to which a consumer of data recognizes or knows that the
;data is weakly ordered varies among applications and may be unknown to the producer of this data. The LFENCE
;instruction provides a performance-efficient way of ensuring load ordering between routines that produce weaklyordered
;results and routines that consume that data.
;Processors are free to fetch and cache data speculatively from regions of system memory that use the WB, WC,
;and WT memory types. This speculative fetching can occur at any time and is not tied to instruction execution.
;Thus, it is not ordered with respect to executions of the LFENCE instruction; data can be brought into the caches
;speculatively just before, during, or after the execution of an LFENCE instruction."

public vi_lfence_rdtsc_asm, vi_mfence_lfence_rdtsc_asm, vi_rdtscp_lfence_asm

vi_lfence_rdtsc_asm PROC
	lfence
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_lfence_rdtsc_asm ENDP

vi_mfence_lfence_rdtsc_asm PROC
	mfence
	lfence
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_mfence_lfence_rdtsc_asm ENDP

vi_rdtscp_lfence_asm PROC
	rdtscp
	shl	rdx, 32
	or	rax, rdx
	lfence
	ret	0
vi_rdtscp_lfence_asm ENDP
;^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^



;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
; The RDPMC instruction can only be executed at privilege level 0.
;public vi_asm_rdpmc_instructions, vi_asm_rdpmc_actual_cycles, vi_asm_rdpmc_reference_cycles
;
;vi_asm_rdpmc_instructions PROC
;	mov ecx, 40000000H
;	rdpmc
;	shl	rdx, 32
;	or	rax, rdx
;	ret	0
;vi_asm_rdpmc_instructions ENDP
;
;vi_asm_rdpmc_actual_cycles PROC
;   unsigned long a, d, c;
;
;   c = (1UL<<30)+1;
;   __asm__ __volatile__("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
;
;   return (a | (d << 32));
;	ret	0
;vi_asm_rdpmc_actual_cycles ENDP
;
;vi_asm_rdpmc_reference_cycles PROC
;   unsigned long a, d, c;
;
;   c = (1UL<<30)+2;
;   __asm__ __volatile__("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
;
;   return (a | (d << 32));
;	ret	0
;vi_asm_rdpmc_reference_cycles ENDP
;^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

END
