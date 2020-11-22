; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc

	LDR R0, RunningPtr	;Load R0 with address of the CurrentlyRunningThread
	LDR R1, [R0]		;Load R1 = value of RunningPtr
	LDR SP, [R1]		; Sp = RunningPtr.sp
	pop {R4-R11}		;Restore Registers
	pop {R0-R3}
	pop {R12}
	ADD SP, SP, #4 		;discard LR from initial stack
	pop {LR}			;start location
	ADD SP, SP, #4 		;discard PSR

	CPSIE I				;Enable Interrupts
	BX LR				;Start First Thread

	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:
	
	.asmfunc

	CPSID I					;Prevent interrupt during context switch
	push {R4-R11}			;Push R4-R11 onto the stack. This updates stack pointer location
	LDR R0, RunningPtr		;Load address of the Current TCB stack pointer into R0
	LDR R1, [R0]			;R1 = RunningPtr
	STR SP, [R1]			;Store the new stackpointer location to the CTR sp memory
							;So it knows where R4-R11 were pushed to
	push {R0, LR}
	BL G8RTOS_Scheduler 	;Call Scheduler to update CTR to next thread
	pop {R0, LR}

	LDR R1, [R0]			;Load the new TCB's stack pointer address into R0
	LDR SP, [R1]			;Set the sp address to the same sp address as the new TCB
	pop {R4-R11}

	CPSIE I					;enable interrupts
	BX LR					;Restore R0-R3,R12,LR,PC,PSR register

	.endasmfunc
	
	; end of the asm file
	.align
	.end
