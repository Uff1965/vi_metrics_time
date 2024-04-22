; https://metanit.com/assembler/tutorial/12.1.php
; https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid

option casemap:none ; Директива option указывает MASM сделать все символы чувствительными к регистру.

.data
;	text byte "Hello from MASM!", 10, 0 ; определяем выводимые данные

.code
;	externdef printf:proc ; подключаем определение функции printf() из C/C++

;оператор public указывает, что функция будет видна вне исходного/объектного файла MASM.

;***************************************************************
; Clear functions:
public vi_asm_rdtsc, vi_asm_rdtscp

vi_asm_rdtsc PROC
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_asm_rdtsc ENDP

vi_asm_rdtscp PROC
	rdtscp
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_asm_rdtscp ENDP

;***************************************************************
; Functions using CPUID:
public vi_asm_cpuid_rdtsc, vi_asm_rdtscp_cpuid

vi_asm_cpuid_rdtsc PROC
	push	rbx

	xor	eax, eax
	xor	ecx, ecx
	cpuid

	rdtsc
	shl	rdx, 32
	or	rax, rdx

	pop	rbx
	ret	0
vi_asm_cpuid_rdtsc ENDP

vi_asm_rdtscp_cpuid PROC
	push	rbx

	rdtscp
	shl	rdx, 32
	or	rax, rdx
	mov	r8, rax

	xor	eax, eax
	xor	ecx, ecx
	cpuid

	mov	rax, r8
	pop	rbx
	ret	0
vi_asm_rdtscp_cpuid ENDP


;***************************************************************
; Functions using FENCE:
public vi_asm_mfence_lfence_rdtsc, vi_asm_rdtscp_lfence

vi_asm_mfence_lfence_rdtsc PROC
	mfence
	lfence
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_asm_mfence_lfence_rdtsc ENDP

vi_asm_rdtscp_lfence PROC
	rdtscp
	lfence
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_asm_rdtscp_lfence ENDP

END
