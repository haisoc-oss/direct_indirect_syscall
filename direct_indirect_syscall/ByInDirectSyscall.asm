.code
InDirectSyscall PROC
	push	rbx					;store value of rbx 
	mov		rbx, rsp			;save current stack pointer before alloacte more space for new stack
	sub		rsp, 40				;allocate space for new stack
	
	xor		rax, rax			;clear out value store in rax			
	movzx	eax , cx			;mov SSN store in rcx to rax

	cmp		rdx, 4				;Check if api function have 4 or more agrument
	jle		Only_4				

	
	sub		rdx, 4				;sub to find many agrument need to push in stack
	imul	rdx, 8				;mul to caculate size of stack frame for agrument
	sub		rsp, rdx			;allocate space on stack for agrument
	mov		[rbx + 16] , rdx	;save rdx value

	Many_agru:					;this loop will begin from the last argrument to the fifth argrument
	mov		r11 ,[rbx+rdx+72]	;get pointer of value that store in stack
	mov		r11 , [r11]			;move the value to r11
	mov		[rsp+rdx+24], r11	;move the value to stack
	sub		rdx,8				;sub rbx untill it become zero then the loop is end
	cmp		rdx,0
	jne		Many_agru

	Only_4:
	mov		r11, r9				;Get return back to shell code after syscall
	push	r11					;push return adress to stack
	mov		r11, r8				;mov adress to syscall to r11

	mov		rcx, [rbx + 48]		;get pointer of first value that store in stack
	mov		rcx, [rcx]			;move the value to rcx
	mov		rdx, [rbx + 56]		;get pointer of second value that store in stack
	mov		rdx, [rdx]			;move the value to rdx
	mov		r8,  [rbx + 64]		;get pointer of third value that store in stack
	mov		r8,  [r8]			;move the value to r8
	mov		r9,  [rbx + 72]		;get pointer of fourth value that store in stack
	mov		r9,  [r9]			;move the value to r9

	mov		r10,rcx
	jmp		r11

	mov		rbx, [rbx +16]
	add		rsp, rbx
	add		rsp, 40
	pop		rbx
	ret


InDirectSyscall ENDP
END	