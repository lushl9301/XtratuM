/*
 * $FILE: asm.h
 *
 * assembly macros and functions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_ASM_H_
#define _XM_ARCH_ASM_H_

#ifdef _XM_KERNEL_

#ifndef _GENERATE_OFFSETS_
#include <arch/asm_offsets.h>
#endif

#include <arch/processor.h>

#ifndef __ASSEMBLY__

#define ASM_EXPTABLE(_a, _b) \
    ".section .exptable, \"a\"\n\t" \
    ".align 4\n\t" \
    ".long "#_a"\n\t" \
    ".long "#_b"\n\t" \
    ".previous\n\t"

#define EXPTABLE(_a) \
    __asm__ (ASM_EXPTABLE(_a))

#define PUSH_REGISTERS \
    "pushl %%eax\n\t" \
    "pushl %%ebp\n\t" \
    "pushl %%edi\n\t" \
    "pushl %%esi\n\t" \
    "pushl %%edx\n\t" \
    "pushl %%ecx\n\t" \
    "pushl %%ebx\n\t"

#define FP_OFFSET 108

#define PUSH_FP  \
    "sub $"TO_STR(FP_OFFSET)",%%esp\n\t" \
    "fnsave (%%esp)\n\t" \
    "fwait\n\t"

#define POP_FP  \
    "frstor (%%esp)\n\t" \
    "add $"TO_STR(FP_OFFSET)", %%esp\n\t"

#define POP_REGISTERS \
    "popl %%ebx\n\t" \
    "popl %%ecx\n\t" \
    "popl %%edx\n\t" \
    "popl %%esi\n\t" \
    "popl %%edi\n\t" \
    "popl %%ebp\n\t" \
    "popl %%eax\n\t"

/* <track id="test-context-switch"> */
#define CONTEXT_SWITCH(nKThread, cKThread) \
    __asm__ __volatile__(PUSH_REGISTERS \
                         "movl (%%ebx), %%edx\n\t" \
                         "pushl $1f\n\t" \
                         "movl %%esp, "TO_STR(_KSTACK_OFFSET)"(%%edx)\n\t" \
                         "movl "TO_STR(_KSTACK_OFFSET)"(%%ecx), %%esp\n\t" \
                         "movl %%ecx, (%%ebx)\n\t" \
                         "ret\n\t" \
                         "1:\n\t" \
             POP_REGISTERS \
             : :"c" (nKThread), "b" (cKThread))
/* </track id="test-context-switch"> */

#define JMP_PARTITION(entry, k) \
    __asm__ __volatile__ ("pushl $"TO_STR(GUEST_DS_SEL)"\n\t" /* SS */ \
                          "pushl $0\n\t" /* ESP */ \
              "pushl $"TO_STR(_CPU_FLAG_IF|0x2)"\n\t" /* EFLAGS */ \
              "pushl $"TO_STR(GUEST_CS_SEL)"\n\t" /* CS */ \
                          "pushl %0\n\t" /* EIP */ \
              "movl $"TO_STR(GUEST_DS_SEL)", %%edx\n\t" \
              "movl %%edx, %%ds\n\t" \
              "movl %%edx, %%es\n\t" \
              "xorl %%edx, %%edx\n\t" \
              "movl %%edx, %%fs\n\t" \
              "movl %%edx, %%gs\n\t" \
              "movl %%edx, %%ebp\n\t" \
              "iret" : : "r" (entry), "b" (XM_PCTRLTAB_ADDR) : "edx")

#endif  /*__ASSEMBLY__*/

#define LoadSegSel(_cs, _ds) \
    __asm__ __volatile__ ("ljmp $"TO_STR(_cs)", $1f\n\t" \
              "1:\n\t" \
              "movl $("TO_STR(_ds)"), %%eax\n\t" \
              "mov %%eax, %%ds\n\t" \
              "mov %%eax, %%es\n\t" \
              "mov %%eax, %%ss\n\t" ::)

#define DoNop() __asm__ __volatile__ ("nop\n\t" ::)

#define DoDiv(n,base) ({ \
    xm_u32_t __upper, __low, __high, __mod, __base; \
    __base = (base);                        \
    asm("":"=a" (__low), "=d" (__high):"A" (n));        \
    __upper = __high;                       \
    if (__high) {                       \
    __upper = __high % (__base);                \
    __high = __high / (__base);             \
    }                               \
    asm("divl %2":"=a" (__low), "=d" (__mod):"rm" (__base), "0" (__low), "1" (__upper)); \
    asm("":"=A" (n):"a" (__low),"d" (__high));          \
    __mod;                          \
})

#ifndef __ASSEMBLY__

static inline void SaveFpuState(xm_u8_t *addr) {
    __asm__ __volatile__("fnsave (%0)\n\t"
                         "fwait\n\t" : : "r" (addr));

}

static inline void RestoreFpuState(xm_u8_t *addr) {
    __asm__ __volatile__("frstor (%0)\n\t" : : "r" (addr));

}

static inline xmWord_t SaveStack(void) {
    xmWord_t sp;
    __asm__ __volatile__ ("movl %%esp, %0\n\t" : "=r" (sp) :: "memory");
    return sp;
}

static inline xmWord_t SaveBp(void) {
    xmWord_t bp;
    __asm__ __volatile__ ("movl %%ebp, %0\n\t" : "=a" (bp));
    return bp;
}

#define HwCli() __asm__ __volatile__ ("cli\n\t":::"memory")
#define HwSti() __asm__ __volatile__ ("sti\n\t":::"memory")

#define HwRestoreFlags(flags) \
    __asm__ __volatile__("push %0\n\t" \
			 "popf\n\t": :"g" (flags):"memory", "cc")

#define HwSaveFlags(flags) \
    __asm__ __volatile__("pushf\n\t" \
 		         "pop %0\n\t" :"=rm" (flags): :"memory")

#define LoadGdt(gdt) \
    __asm__ __volatile__ ("lgdt %0\n\t" : : "m" (gdt))

#define LoadIdt(idt) \
    __asm__ __volatile__ ("lidt %0\n\t" : : "m" (idt))

#define LoadTr(seg) \
    __asm__ __volatile__ ("ltr %0\n\t" : : "rm" ((xm_u16_t)(seg)))

#define DEC_LOAD_CR(_r) \
static inline void LoadCr##_r(xm_u32_t cr##_r) { \
    __asm__ __volatile__ ("mov %0, %%cr"#_r"\n\t" : : "r" ((xmWord_t)cr##_r)); \
}

#define DEC_SAVE_CR(_r) \
static inline xmWord_t SaveCr##_r(void) { \
    xmWord_t cr##_r; \
    __asm__ __volatile__ ("mov %%cr"#_r", %0\n\t" : "=r" (cr##_r)); \
    return cr##_r; \
}

DEC_LOAD_CR(0);
DEC_LOAD_CR(3);
DEC_LOAD_CR(4);

DEC_SAVE_CR(0);
DEC_SAVE_CR(2);
DEC_SAVE_CR(3);
DEC_SAVE_CR(4);

#define LoadPtdL1(ptdL1, _id) LoadCr3(ptdL1)
#define LoadXmPageTable() LoadCr3(_VIRT2PHYS((xm_u32_t)_pgTables))

#define LoadPartitionPageTable(k) LoadCr3(k->ctrl.g->kArch.ptdL1)

#define LoadGs(gs) do {	\
    __asm__ __volatile__ ("mov %0, %%gs\n\t" : : "r" (gs)); \
} while(0)

#define SaveSs(ss) do {	\
    __asm__ __volatile__ ("mov %%ss, %0\n\t" : "=a" (ss)); \
} while(0)

#define SaveCs(cs) do { \
    __asm__ __volatile__ ("mov %%cs, %0\n\t" : "=a" (cs)); \
} while(0)

#define SaveEip(eip) do { \
    __asm__ __volatile__ ("movl $1f, %0\n\t"    \
                          "1:\n\t" : "=r"(eip));    \
} while(0)

static inline void FlushTlb(void) {
    LoadCr3(SaveCr3());
}

static inline void FlushTlbGlobal(void) {
    xmWord_t cr4;
    cr4=SaveCr4();
    LoadCr4(cr4&~_CR4_PGE);
    FlushTlb();
    LoadCr4(cr4);
}

#define DisablePaging() LoadCr0(SaveCr0() | _CR0_PG);
#define EnablePaging() LoadCr0(SaveCr0() & ~_CR0_PG)

static inline void FlushTlbEntry(xmAddress_t addr) {
    __asm__ __volatile__ ("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline xm_u64_t RdTscLL(void) {
    xm_u32_t l, h;
    __asm__ __volatile__("rdtsc" : "=a" (l), "=d" (h));
    return (l|((xm_u64_t)h<<32));
}

static inline void RdTsc(xm_u32_t *l, xm_u32_t *h) {
    __asm__ __volatile__("rdtsc" : "=a" (*l), "=d" (*h));
}

static inline xm_u64_t ReadMsr(xm_u32_t msr) {
    xm_u32_t l, h;
    
    __asm__ __volatile__("rdmsr\n\t" : "=a" (l), "=d" (h) : "c" (msr));
    return (l|((xm_u64_t)h<<32));
}

static inline xmAddress_t SaveGdt(void) {
    xmAddress_t gdt;
    __asm__ __volatile__ ("sgdt %0\n\t" : "=m" (gdt));
    return gdt;
}

static inline void WriteMsr(xm_u32_t msr, xm_u32_t l, xm_u32_t h) {
    __asm__ __volatile__ ("wrmsr\n\t"::"c"(msr), "a"(l), "d"(h):"memory");
}

static inline void CpuId(xm_u32_t op, xm_u32_t *eax, xm_u32_t *ebx, xm_u32_t *ecx, xm_u32_t *edx) {
    *eax=op;
    *ecx=0;
    __asm__ __volatile__ ("cpuid\n\t" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), \
			  "=d" (*edx) : "0" (*eax), "2" (*ecx));
}

/*
static inline void SetVme(void) {
    xm_u32_t tmpreg;
    __asm__ __volatile__ ("movl %%cr4, %0\n\t" : "=r" (tmpreg));
    tmpreg|=_CR4_VME;
	__asm__ __volatile__ ("movl %0, %%cr4\n\t" : : "r" (tmpreg));
}

static inline void SetPse(void) {
    xm_u32_t tmpreg;
    __asm__ __volatile__ ("movl %%cr4, %0\n\t" : "=r" (tmpreg));
    tmpreg|=_CR4_PSE;
    __asm__ __volatile__ ("movl %0, %%cr4\n\t" : : "r" (tmpreg));
}

static inline void SetPge(void) {
    xm_u32_t tmpreg;
    __asm__ __volatile__ ("movl %%cr4, %0\n\t" : "=r" (tmpreg));
    tmpreg|=_CR4_PGE;
    __asm__ __volatile__ ("movl %0, %%cr4\n\t" : : "r" (tmpreg));
}
*/

#define FNINIT() __asm__ __volatile__ ("fninit\n\t" ::)
#define CLTS() __asm__ __volatile__ ("clts\n\t" ::)

static inline void SetWp(void) {
    xm_u32_t tmpreg;
    __asm__ __volatile__ ("movl %%cr0, %0\n\t": "=r" (tmpreg));
    tmpreg|=_CR0_WP;
    __asm__ __volatile__ ("movl %0, %%cr0\n\t" : : "r" (tmpreg));
}

static inline void ClearWp(void) {
    xm_u32_t tmpreg;
    __asm__ __volatile__ ("movl %%cr0, %0\n\t": "=r" (tmpreg));
    tmpreg&=(~_CR0_WP);
	__asm__ __volatile__ ("movl %0, %%cr0\n\t" : : "r" (tmpreg));
}

static inline xm_s32_t AsmRWCheck(xmAddress_t param, xmSize_t size, xm_u32_t aligment) {

    xmAddress_t addr;
    xm_s32_t ret;
    xm_u8_t tmp;

    for (addr=param; addr<param+size; addr=(addr&PAGE_MASK)+PAGE_SIZE) {
        __asm__ __volatile__("mov $1, %0\n\t"
                             "1: movb %2, %1\n\t" \
                             "2: movb %1, %2\n\t" \
                             "xor %0, %0\n\t" \
                             "3:\n\t" \
                             ASM_EXPTABLE(1b, 3b) \
                             ASM_EXPTABLE(2b, 3b) \
                             : "=b" (ret), "=c" (tmp) : "m" (*(xm_u8_t *)addr));
        if (ret)
            return -1;
    }

    return 0;
} 

static inline xm_s32_t AsmROnlyCheck(xmAddress_t param, xmSize_t size, xm_u32_t aligment) {
    xmAddress_t addr;
    xm_s32_t ret;
    xm_u8_t tmp;

    for (addr=param; addr<param+size; addr=(addr&PAGE_MASK)+PAGE_SIZE) {
        __asm__ __volatile__("mov $1, %0\n\t"
                             "1: mov %2, %1\n\t" \
                             "xor %0, %0\n\t" \
                             "2:\n\t" \
                             ASM_EXPTABLE(1b, 2b) \
                             : "=a" (ret), "=b" (tmp) : "m" (*(xm_u8_t *)addr));
        if (ret)
            return -1;
    }

    return 0;
}

#endif
#endif
#endif
