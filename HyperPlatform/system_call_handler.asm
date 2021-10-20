extern SystemCallHandler:proc
extern KiSystemCall64ShadowCommon:proc 
;extern OriKiSystemServiceCopyEnd:proc
;extern OriKiSystemServiceCopyStart:proc
;extern OriOtherKiSystemServiceCopyEnd:proc
extern OriKiSystemServiceStart:proc

.code

SAVE macro
	push rax
	push rcx
	push rdx
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push rdi
	push rsi 
	push rbx
	push rbp
endm

RESTOR macro
	pop rbp
	pop rbx
	pop rsi
	pop rdi
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdx
	pop rcx
	pop rax
endm	

DetourKiSystemCall64Shadow proc
	swapgs
	mov gs:[7010h],rsp
	mov rsp,gs:[7000h]
	bt dword ptr gs:[7018],1
	jb kpti
	mov cr3,rsp ;�л�cr3��������ǵ��ں˴���ӳ��
kpti:
	mov rsp,gs:[7008h]

	;int 3

	SAVE
	sub rsp,28h
	mov rax,rcx
	call SystemCallHandler
	add rsp,28h
	RESTOR

	;int 3

	;ִ����֮�������Ӧ�÷��ص�ԭKiSystemCall64Shadow+0x2D�ĵط�
	jmp qword ptr [KiSystemCall64ShadowCommon]


DetourKiSystemCall64Shadow endp

DetourKiSystemServiceCopyEnd proc
	
	;int 3
	;jmp qword ptr[OriKiSystemServiceCopyEnd]

DetourKiSystemServiceCopyEnd endp

;DetourKiSystemServiceCopyStart proc
	
;	int 3
;	jmp qword ptr[OriKiSystemServiceCopyStart]

;DetourKiSystemServiceCopyStart endp


DetourOtherKiSystemServiceCopyEnd proc

	int 3
	;jmp qword ptr[OriOtherKiSystemServiceCopyEnd]
	
DetourOtherKiSystemServiceCopyEnd endp

DetourKiSystemServiceStart proc
	

		SAVE
	sub rsp,28h
	mov rcx,rsp
	mov edx,eax
	call SystemCallHandler
	add rsp,28h
	RESTOR

	jmp qword ptr[OriKiSystemServiceStart]

DetourKiSystemServiceStart endp


end