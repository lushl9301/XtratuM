/*
 * $FILE: irqs.h
 *
 * IRQS
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_IRQS_H_
#define _XM_ARCH_IRQS_H_

/* existing traps (just 32 in the ia32 arch) */
#define NO_TRAPS 32

#include __XM_INCFLD(arch/xm_def.h)
#include __XM_INCFLD(guest.h)

#ifndef __ASSEMBLY__
struct trapHandler {
    xm_u16_t cs;
    xmAddress_t ip;
};
#endif

#ifdef _XM_KERNEL_
#define IDT_ENTRIES 256

/* existing hw interrupts (required by PIC and APIC) */
#define FIRST_EXTERNAL_VECTOR 0x20
#define FIRST_USER_IRQ_VECTOR 0x30

#ifdef CONFIG_SMP
#define NR_IPIS 4
#endif

#define UD_HNDL 0x6
#define NM_HNDL 0x7
#define GP_HNDL 0xd
#define PF_HNDL 0xe

#ifndef __ASSEMBLY__

struct x86IrqStackFrame {
    xmWord_t ip;
    xmWord_t cs;
    xmWord_t flags;
    xmWord_t sp;
    xmWord_t ss;
};

typedef struct _cpuCtxt {
    xmWord_t bx;
    xmWord_t cx;
    xmWord_t dx;
    xmWord_t si;
    xmWord_t di;
    xmWord_t bp;
    xmWord_t ax;
    xmWord_t ds;
    xmWord_t es;
    xmWord_t fs;
    xmWord_t gs;
    struct _cpuCtxt *prev;
    xmWord_t irqNr;
    xmWord_t eCode;

    xmWord_t ip;
    xmWord_t cs;
    xmWord_t flags;
    xmWord_t sp;
    xmWord_t ss;
} cpuCtxt_t;

#define GET_ECODE(ctxt) ctxt->eCode
#define GET_CTXT_PC(ctxt) ctxt->ip
#define SET_CTXT_PC(ctxt, _pc) do { \
    (ctxt)->ip=(_pc); \
} while(0)

#define CpuCtxt2HmCpuCtxt(cpuCtxt, hmCpuCtxt) do { \
    (hmCpuCtxt)->pc=(cpuCtxt)->ip; \
    (hmCpuCtxt)->psr=(cpuCtxt)->flags; \
} while(0)

#define PrintHmCpuCtxt(ctxt) \
    kprintf("ip: 0x%lx flags: 0x%lx\n", (ctxt)->pc, (ctxt)->psr)

#ifdef CONFIG_SMP

typedef struct {
// saved registers
    xm_u32_t bx;
    xm_u32_t cx;
    xm_u32_t dx;
    xm_u32_t si;
    xm_u32_t di;
    xm_u32_t bp;
    xm_u32_t ax;
    xm_u32_t ds;
    xm_u32_t es;
    xm_u32_t fs;
    xm_u32_t gs;
// ipi
    xm_u32_t noIpi;
// processor state frame
    xm_u32_t ip;
    xm_u32_t cs;
    xm_u32_t eflags;
    xm_u32_t sp;
    xm_u32_t ss;
} ipiCtxt_t;

typedef void (*IpiIrq_t)(ipiCtxt_t *ctxt, void *data);

extern IpiIrq_t ipiTable[NR_IPIS];
#endif

static inline void InitPCtrlTabIrqs(xm_u32_t *iFlags) {
    (*iFlags)&=~_CPU_FLAG_IF;
}

static inline xm_s32_t ArePCtrlTabIrqsSet(xm_u32_t iFlags) {
    return (iFlags&_CPU_FLAG_IF)?1:0;
}

static inline void DisablePCtrlTabIrqs(xm_u32_t *iFlags) {
    //(*iFlags)&=~_CPU_FLAG_IF;
    // x86 does not require it
}

static inline xm_s32_t ArePCtrlTabTrapsSet(xm_u32_t iFlags) {
    return 1;
}

static inline void MaskPCtrlTabIrq(xm_u32_t *mask, xm_u32_t bitmap) {
    // doing nothing
}

static inline xm_s32_t IsSvIrqCtxt(cpuCtxt_t *ctxt) {    
    return (ctxt->cs&0x3)?0:1;
}

extern void EarlySetupIrqs(void);

extern void FixStack(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr, xm_s32_t vector, xm_s32_t trap);

static inline xm_s32_t ArchEmulTrapIrq(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr) {
    FixStack(ctxt, partCtrlTab, irqNr, partCtrlTab->trap2Vector[irqNr], 1);
    return partCtrlTab->trap2Vector[irqNr];
}

static inline xm_s32_t ArchEmulHwIrq(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr) {
    FixStack(ctxt, partCtrlTab, irqNr, partCtrlTab->hwIrq2Vector[irqNr], 0);
    return partCtrlTab->hwIrq2Vector[irqNr];
}

static inline xm_s32_t ArchEmulExtIrq(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr) {
    FixStack(ctxt, partCtrlTab, irqNr, partCtrlTab->extIrq2Vector[irqNr], 0);
    return partCtrlTab->extIrq2Vector[irqNr];
}

#define SAVE_REG(reg, field) \
        __asm__ __volatile__("mov %%"#reg", %0\n\t" : "=m" (field) : : "memory");

static inline void GetCpuCtxt(cpuCtxt_t *ctxt) {
    SAVE_REG(ebx, ctxt->bx);
    SAVE_REG(ecx, ctxt->cx);
    SAVE_REG(edx, ctxt->dx);
    SAVE_REG(esi, ctxt->si);
    SAVE_REG(edi, ctxt->di);
    SAVE_REG(ebp, ctxt->bp);
    SAVE_REG(eax, ctxt->ax);
    SAVE_REG(ds, ctxt->ds);
    SAVE_REG(es, ctxt->es);
    SAVE_REG(fs, ctxt->fs);
    SAVE_REG(gs, ctxt->gs);
    SAVE_REG(cs, ctxt->cs);
    SAVE_REG(esp, ctxt->sp);
    SAVE_REG(ss, ctxt->ss);
    SaveEip(ctxt->ip);
    HwSaveFlags(ctxt->flags);
}

extern xm_u32_t x86HwIrqsMask[CONFIG_NO_CPUS];

#endif  //__ASSEMBLY__
#endif  //_XM_KERNEL_

/* <track id="x86-exception-list"> */
#define DIVIDE_ERROR 0
#define DEBUG_EXCEPTION 1
#define NMI_INTERRUPT 2
#define BREAKPOINT_EXCEPTION 3
#define OVERFLOW_EXCEPTION 4
#define BOUNDS_EXCEPTION 5
#define INVALID_OPCODE 6
#define DEVICE_NOT_AVAILABLE 7
#define DOUBLE_FAULT 8
#define COPROCESSOR_OVERRRUN 9
#define INVALID_TSS 10
#define SEGMENT_NOT_PRESENT 11
#define STACK_SEGMENT_FAULT 12
#define GENERAL_PROTECTION_FAULT 13
#define PAGE_FAULT 14
#define RESERVED2_EXCEPTION 15
#define FLOATING_POINT_ERROR 16
#define ALIGNMENT_CHECK 17
#define MACHINE_CHECK 18
#define SIMD_EXCEPTION 19
/* </track id="x86-exception-list"> */

#endif

