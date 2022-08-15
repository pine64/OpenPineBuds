/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

                .syntax  unified
                .align   4

                .equ   MODE_USR,        0x10
                .equ   MODE_FIQ,        0x11
                .equ   MODE_IRQ,        0x12
                .equ   MODE_SVC,        0x13
                .equ   MODE_ABT,        0x17
                .equ   MODE_UND,        0x1B
                .equ   MODE_SYS,        0x1F
                .equ   MODE_MASK,       0x1F

                .equ   CPSR_BIT_T,      0x20

                .equ   SAVED_REG_NUM,   17

//-------------------------------------------------------
                .section ".text.IRQ_Handler"
                .type    IRQ_Handler, %function
                .global  IRQ_Handler
                .weak    IRQ_Handler
                .fnstart
                .cantunwind
IRQ_Handler:
                SUB     LR, LR, #4                  // Pre-adjust LR
                SRSFD   SP!, #MODE_SVC              // Save LR_irq and SPSR_irq on to the SVC stack
                CPS     #MODE_SVC                   // Change to SVC mode
                PUSH    {R0-R3, R12, LR}            // Save APCS corruptible registers

                MOV     R3, SP                      // Move SP into R3
                AND     R3, R3, #4                  // Get stack adjustment to ensure 8-byte alignment
                SUB     SP, SP, R3                  // Adjust stack
                PUSH    {R3, R4}                    // Store stack adjustment(R3) and user data(R4)

                BLX     c_irq_handler

                POP     {R3, R4}                    // Restore stack adjustment(R3) and user data(R4)
                ADD     SP, SP, R3                  // Unadjust stack

                CLREX                               // Clear exclusive monitor for interrupted code
                POP     {R0-R3, R12, LR}            // Restore stacked APCS registers
                RFEFD   SP!                         // Return from IRQ handler

                .fnend
                .size    IRQ_Handler, .-IRQ_Handler

//-------------------------------------------------------
                .section ".text.Save_Registers"
                .type    Save_Registers, %function
                .fnstart
                .cantunwind
Save_Registers:
                PUSH    {R12}
                SUB     SP, SP, #SAVED_REG_NUM*4

                // Save r0-r7
                STMFD   SP, {R0-R7}

                // Switch to the mode in SPSR
                LDR     R12,[SP, #(SAVED_REG_NUM+2)*4]
                MRS     R1, CPSR
                AND     R2, R1, #MODE_MASK
                BIC     R3, R1, #MODE_MASK
                AND     R0, R12, #MODE_MASK
                CMP     R0, #MODE_USR
                MOVEQ   R0, #MODE_SYS
                ORR     R3, R3, R0
                MSR     CPSR, R3

                // Save r8-r14
                ADD     R0, SP, #8*4
                STMFD   R0, {R8-R14}

                // Switch back to the original CPSR
                MSR     CPSR, R1

                // LR already saved by SRSFD
                LDR     R2, [SP, #(SAVED_REG_NUM+1)*4]

                // Save pc, spsr
                STR     R2, [SP, #15*4]
                STR     R12,[SP, #16*4]

                BX      LR

                .fnend
                .size    Save_Registers, .-Save_Registers

//-------------------------------------------------------
                .section ".text.Restore_Registers"
                .type    Restore_Registers, %function
                .fnstart
                .cantunwind
Restore_Registers:
                // Restore APCS corruptible registers
                LDMFD   SP, {R0-R3}
                ADD     SP, SP, #SAVED_REG_NUM*4
                POP     {R12}
                BX      LR

                .fnend
                .size    Restore_Registers, .-Restore_Registers

//-------------------------------------------------------
                .section ".text.SVC_Handler"
                .type    SVC_Handler, %function
                .global  SVC_Handler
                .weak    SVC_Handler
                .fnstart
                .cantunwind
SVC_Handler:
                SRSFD   SP!, #MODE_SVC              // Store SPSR_svc and LR_svc onto SVC stack

                BLX     Save_Registers

                MRS     R12, SPSR                   // Load SPSR
                TST     R12, #CPSR_BIT_T            // Thumb bit set?
                LDRHNE  R12, [LR,#-2]               // Thumb: load halfword
                BICNE   R12, R12, #0xFF00           //        extract SVC number
                LDREQ   R12, [LR,#-4]               // ARM:   load word
                BICEQ   R12, R12, #0xFF000000       //        extract SVC number
                MOV     R0,  R12                    // Save SVC number

                MOV     R1, SP

                MOV     R3, SP                      // Move SP into R3
                AND     R3, R3, #4                  // Get stack adjustment to ensure 8-byte alignment
                SUB     SP, SP, R3                  // Adjust stack
                PUSH    {R3, R4}                    // Store stack adjustment(R3) and user data(R4)

                BLX     c_svc_handler             // Call SVC Function

                POP     {R3, R4}                    // Restore stack adjustment(R3) and user data(R4)
                ADD     SP, SP, R3                  // Unadjust stack

                BLX     Restore_Registers
                CLREX                               // Clear exclusive monitor
                RFEFD   SP!                         // Return from exception

                .fnend
                .size    SVC_Handler, .-SVC_Handler

//-------------------------------------------------------
                .section ".text.Undef_Handler"
                .type    Undef_Handler, %function
                .global  Undef_Handler
                .weak    Undef_Handler
                .fnstart
                .cantunwind
Undef_Handler:
                SRSFD   SP!, #MODE_SVC              // Save LR_irq and SPSR_irq on to the SVC stack
                CPS     #MODE_SVC                   // Change to SVC mode

                BLX     Save_Registers

                TST     R12,#CPSR_BIT_T             // Check mode
                MOVEQ   R1, #4                      // R1 = 4 ARM mode
                MOVNE   R1, #2                      // R1 = 2 Thumb mode

                // Get offending instruction address
                LDR     LR, [SP, #15*4]
                SUB     LR, LR, R1
                STR     LR, [SP, #15*4]

                SUB     R0, LR, R1
                CMP     R1, 4
                LDREQ   R0, [R0]                    // ARM mode - R0 points to offending instruction
                BEQ     Undef_Cont

                // Thumb instruction
                // Determine if it is a 32-bit Thumb instruction
                LDRH    R0, [R0]
                MOV     R2, #0x1C
                CMP     R2, R0, LSR #11
                BHS     Undef_Cont                  // 16-bit Thumb instruction

                // 32-bit Thumb instruction. Unaligned - reconstruct the offending instruction
                LDRH    R2, [LR]
                ORR     R0, R2, R0, LSL #16
Undef_Cont:
                MOV     R2, SP                      // Set SP to third argument

                AND     R12, SP, #4                 // Ensure stack is 8-byte aligned
                SUB     SP, SP, R12                 // Adjust stack
                PUSH    {R12, LR}                   // Store stack adjustment and dummy LR

                // R0 =Offending instruction, R1 =2(Thumb) or =4(ARM)
                BLX     c_undef_handler

                POP     {R12, LR}                   // Get stack adjustment & discard dummy LR
                ADD     SP, SP, R12                 // Unadjust stack

                LDR     LR, [SP, #15*4]             // Restore stacked LR and possibly adjust for retry
                PUSH    {LR}
                BLX     Restore_Registers
                POP     {LR}
                STR     LR, [SP]
                CLREX                               // Clear exclusive monitor
                RFEFD   SP!                         // Return from exception

                .fnend
                .size    Undef_Handler, .-Undef_Handler

//-------------------------------------------------------
                .section ".text.PAbt_Handler"
                .type    PAbt_Handler, %function
                .global  PAbt_Handler
                .weak    PAbt_Handler
                .fnstart
                .cantunwind
PAbt_Handler:
                SUB     LR, LR, #4                  // Pre-adjust LR
                SRSFD   SP!, #MODE_SVC              // Save LR_irq and SPSR_irq on to the SVC stack
                CPS     #MODE_SVC                   // Change to SVC mode

                BLX     Save_Registers

                MRC     p15, 0, R0, c5, c0, 1       // IFSR
                MRC     p15, 0, R1, c6, c0, 2       // IFAR

                MOV     R2, SP                      // Set SP to third argument

                AND     R12, SP, #4                 // Ensure stack is 8-byte aligned
                SUB     SP, SP, R12                 // Adjust stack
                PUSH    {R12, LR}                   // Store stack adjustment and dummy LR

                BLX     c_pabt_handler

                POP     {R12, LR}                   // Get stack adjustment & discard dummy LR
                ADD     SP, SP, R12                 // Unadjust stack

                CLREX                               // Clear exclusive monitor
                BLX     Restore_Registers
                RFEFD   SP!                         // Return from exception

                .fnend
                .size    PAbt_Handler, .-PAbt_Handler

//-------------------------------------------------------
                .section ".text.DAbt_Handler"
                .type    DAbt_Handler, %function
                .global  DAbt_Handler
                .weak    DAbt_Handler
                .fnstart
                .cantunwind
DAbt_Handler:
                SUB     LR, LR, #8                  // Pre-adjust LR
                SRSFD   SP!, #MODE_SVC              // Save LR_irq and SPSR_irq on to the SVC stack
                CPS     #MODE_SVC                   // Change to SVC mode

                BLX     Save_Registers

                MRC     p15, 0, R0, c5, c0, 0       // DFSR
                MRC     p15, 0, R1, c6, c0, 0       // DFAR

                MOV     R2, SP                      // Set SP to third argument

                AND     R12, SP, #4                 // Ensure stack is 8-byte aligned
                SUB     SP, SP, R12                 // Adjust stack
                PUSH    {R12, LR}                   // Store stack adjustment and dummy LR

                BLX     c_dabt_handler

                POP     {R12, LR}                   // Get stack adjustment & discard dummy LR
                ADD     SP, SP, R12                 // Unadjust stack

                CLREX                               // Clear exclusive monitor
                BLX     Restore_Registers
                RFEFD   SP!                         // Return from exception

                .fnend
                .size    DAbt_Handler, .-DAbt_Handler

                .end
