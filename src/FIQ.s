//
// Pi1MHz FIQ handler
//
// (c) Dominic Plunkett
//
// FIQ code sits at FIQ vector
// This code is directly included at the FIQ entry
// So don't put any code before it.
// and is triggered by fred or jim access

#include "Pi1MHz.h"
#include "rpi/base.h"

#define FIQ_SETUP_CODE 1
.macro FIQ_SETUP_M
   mov     R12,#0
   LDR     r11,= Pi1MHz_VPU_RETURN
   mov     r10, # ADDRBUS_MASK>>ADDRBUS_SHIFT
   orr     r10,r10,#NPCFC_MASK>>ADDRBUS_SHIFT
   mov     r10, r10 , LSL # 2
.endm

.macro DMB_MACRO reg
#if (__ARM_ARCH == 6 )
   // dmb   // Only needed on ArmV6 systems
    mcr      p15, 0, \reg, c7, c10, 5
#endif

.endm

//  r8  temp
//  r9  temp
//  r10 Address mask
//  r11 VPU return data
//  r12 doorbell reg
//  r13 stack
//  r14 return address
FIQstart:
   DMB_MACRO r12            // R12 is defined to be zero
                            // This finshes any outstanding read that might be inflight
                            // e.g. from the foreground task
   LDR      r8, [r11]       // get data posted from the VPU.

// Stall as going off chip
// so do something a bit useful
   orr      r12,r12,#(PERIPHERAL_BASE + 0x00B844) & 0xff000000
   orr      r12,r12,#(PERIPHERAL_BASE + 0x00B844) & 0x00ff0000
   orr      r12,r12,#(PERIPHERAL_BASE + 0x00B844) & 0x0000ff00
   orr      r12,r12,#(PERIPHERAL_BASE + 0x00B844) & 0x000000ff

   tst      r8, # RNW_MASK

   LDR      r12,[R12]       // read door bell to ack

   and      r9, r10, r8, LSR # ADDRBUS_SHIFT - 2 // isolate address bus and fred or jim

   orrne    r9, r9, # Pi1MHz_MEM_RNW<<2     // set read flag ready for call back table

   ldr      r9, [r9, #Pi1MHz_CB_BASE]    // load call back pointer

//stall
   mov      r12, #0

   movs     r9, r9
   DMB_MACRO r12           // check doorbell ack has actually left the core.
   subeqs   pc, lr,#4   // no callback

   push    {r0-r3,r12, r14}
   mov     r0, r8, LSR # DATABUS_SHIFT
   blx     r9
   pop     {r0-r3,r12, r14}

   subs    pc, lr, #4
FIQend:
