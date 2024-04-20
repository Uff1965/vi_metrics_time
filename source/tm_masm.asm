; https://metanit.com/assembler/tutorial/12.1.php
; https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid

option casemap:none ; Директива option указывает MASM сделать все символы чувствительными к регистру.

.data
;	text byte "Hello from MASM!", 10, 0 ; определяем выводимые данные

.code
;	externdef printf:proc ; подключаем определение функции printf() из C/C++

public vi_start, vi_finish, vi_test, vi_test2, vi_start2, vi_finish2; оператор public указывает, что функция будет видна вне исходного/объектного файла MASM.

vi_start PROC
	xor	eax, eax
	xor	ecx, ecx
	cpuid
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_start ENDP

vi_finish PROC
	rdtscp
	shl	rdx, 32
	or	rax, rdx
	mov	r8, rax
	xor	eax, eax
	xor	ecx, ecx
	cpuid
	mov	rax, r8
	ret	0
vi_finish ENDP

vi_test PROC
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_test ENDP

vi_test2 PROC
	lfence
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_test2 ENDP

vi_start2 PROC
	mfence
	lfence
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_start2 ENDP

vi_finish2 PROC
	rdtscp
	lfence
	shl	rdx, 32
	or	rax, rdx
	ret	0
vi_finish2 ENDP

END
