/*
 * $FILE: segments.h
 *
 * i386 segmentation
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_SEGMENTS_H_
#define _XM_ARCH_SEGMENTS_H_

#if (CONFIG_PARTITION_NO_GDT_ENTRIES<3)
#error Number of GDT entries must be at least 3
#endif

#define EARLY_XM_GDT_ENTRIES 3

// XM GDT's number of entries
#define XM_GDT_ENTRIES 12

#define EARLY_CS_SEL (1<<3) // XM's code segment (Ring 0)
#define EARLY_DS_SEL (2<<3) // XM's data segment (Ring 0)

 // Segment selectors
#define XM_HYPERCALL_CALLGATE_SEL ((1+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)
#define XM_USER_HYPERCALL_CALLGATE_SEL ((2+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)
#define XM_IRET_CALLGATE_SEL ((3+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)

#define CS_SEL ((5+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3) // XM's code segment (Ring 0)
#define DS_SEL ((6+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3) // XM's data segment (Ring 0)
#define GUEST_CS_SEL (((7+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)+1) // Guest's code segment (Ring 1)
#define GUEST_DS_SEL (((8+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)+1) // Guest's data segment (Ring 1)

#define PERCPU_SEL ((9+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)
#define TSS_SEL ((10+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)
#define PCT_SEL (((11+CONFIG_PARTITION_NO_GDT_ENTRIES)<<3)+1) // Guest's PCT access segment (Ring 1)

#ifndef __ASSEMBLY__

struct x86Desc {
    union {
        struct {
            xm_u32_t segLimit15_0:16,
                base15_0:16,
                base23_16:8,
                access:8,
                segLimit19_16:4,
                granularity:4,
                base31_24:8;
        };
        struct {
            xm_u32_t offset15_0:16,
                segSel:16,
                wordCount:8,
                _access:8,  // <-- Matches with above
                offset31_16:16;
        };
        xm_u64_t x;
        struct {
            xm_u32_t high;
            xm_u32_t low;
#define X86DESC_LOW_P_POS 15
#define X86DESC_LOW_P (0x1<<X86DESC_LOW_P_POS)
#define X86DESC_LOW_DPL_POS 13
#define X86DESC_LOW_DPL_MASK (0x3<<X86DESC_LOW_DPL_POS)
#define X86DESC_LOW_S_POS 12
#define X86DESC_LOW_S (0x1<<X86DESC_LOW_S_POS)
#define X86DESC_LOW_TYPE_POS 8
#define X86DESC_LOW_TYPE_MASK (0xf<<X86DESC_LOW_TYPE_POS)
        };
    };
} __PACKED;

struct x86SegByField {
    xm_u32_t base;
    xm_u32_t segLimit;
    xm_u32_t g:1,
             d_b:1,
             l:1,
             avl:1,
             p:1,
             dpl:2,
             s:1,
             type:4;
} __PACKED;

struct x86Gate {
    union {
        struct {
            xm_u32_t offset15_0:16,
                segSel:16,
                wordCount:8,
                access:8,
                offset31_16:16;
        };
        struct {
            xm_u64_t l;
        };
        struct {
            xm_u32_t high0;
            xm_u32_t low0;
#define X86GATE_LOW0_P_POS 15
#define X86GATE_LOW0_P (0x1<<X86GATE_LOW0_P_POS)
        };
    };
} __PACKED;

struct x86Tss {
    xm_u16_t backLink, _blh;
    xm_u32_t sp0;
    xm_u16_t ss0, _ss0h;
    xm_u32_t sp1;
    xm_u16_t ss1, _ss1h;
    xm_u32_t sp2;
    xm_u16_t ss2, _ss2h;
    xm_u32_t cr3;
    xm_u32_t ip;
    xm_u32_t flags;
    xm_u32_t ax;
    xm_u32_t cx;
    xm_u32_t dx;
    xm_u32_t bx;
    xm_u32_t sp;
    xm_u32_t bp;
    xm_u32_t si;
    xm_u32_t di;
    xm_u16_t es, _esh;
    xm_u16_t cs, _csh;
    xm_u16_t ss, _ssh;
    xm_u16_t ds, _dsh;
    xm_u16_t fs, _fsh;
    xm_u16_t gs, _gsh;
    xm_u16_t ldt, _ldth;
    xm_u16_t traceTrap;
    xm_u16_t ioBitmapOffset;
} __PACKED;

struct ioTss {
    struct x86Tss t;
    xm_u32_t ioMap[2048];
};
#endif

#ifdef _XM_KERNEL_
#ifndef __ASSEMBLY__

#ifdef CONFIG_SMP
#define GDT_ENTRY(cpu, e)   (((XM_GDT_ENTRIES+CONFIG_PARTITION_NO_GDT_ENTRIES)*(cpu))+((e)>>3))
#else
#define GDT_ENTRY(cpu, e)        ((e)>>3)
#endif /*CONFIG_SMP*/

#define GetDescBase(d) (((d)->low&0xff000000)|(((d)->low&0xff)<<16)|((d)->high>>16))

#define GetDescLimit(d) ((d)->low&(1<<23))?(((((d)->low&0xf0000)|((d)->high&0xffff))<<12)+0xfff):(((d)->low&0xf0000)|((d)->high&0xffff))

/*
#define GetTssSeg(tssDesc, b, l) do { \
    b=tssDesc.desc.baseLow|(tssDesc.desc.baseMed<<16)|(tssDesc.desc.baseHigh<<24); \
    l=tssDesc.desc.limitLow|(tssDesc.desc.limitHigh<<16); \
} while(0)
*/

#define GetTssDesc(gdtr, tssSel) \
  ((struct x86Desc *)((gdtr)->linearBase))[tssSel/8]

#define TssClearBusy(gdtr, tssSel) \
  GetTssDesc((gdtr), tssSel).access&=~(0x2);

#define TSS_IO_MAP_DISABLED (0xFFFF)

#define DisableTssIoMap(tss)  \
  (tss)->t.ioBitmapOffset=TSS_IO_MAP_DISABLED
  
#define EnableTssIoMap(tss) \
  (tss)->t.ioBitmapOffset= \
    ((xmAddress_t)&((tss)->ioMap)-(xmAddress_t)tss)

#define SetIoMapBit(bit, ioMap) do { \
    xm_u32_t __entry, __offset; \
    __entry=bit/32; \
    __offset=bit%32; \
    ioMap[__entry]|=(1<<__offset); \
} while(0)

#define ClearIoMapBit(bit, ioMap) do { \
    xm_u32_t __entry, __offset; \
    __entry=bit/32; \
    __offset=bit%32; \
    ioMap[__entry]&=~(1<<__offset); \
} while(0)

#define IsIoMapBitSet(bit, ioMap) do { \
    xm_u32_t __entry, __offset; \
    __entry=bit/32; \
    __offset=bit%32; \
    ioMap[__entry]&(1<<__offset); \
} while(0)

extern struct x86Desc gdtTab[];
extern struct x86Gate idtTab[IDT_ENTRIES];

static inline void LoadTssDesc(struct x86Desc *desc, struct ioTss *tss) {
    desc->segLimit15_0=0xffff&(sizeof(struct ioTss)-1);
    desc->segLimit19_16=0xf&((sizeof(struct ioTss)-1)>>16);
    desc->base15_0=(xmAddress_t)tss&0xffff;
    desc->base23_16=((xmAddress_t)tss>>16)&0xff;
    desc->base31_24=((xmAddress_t)tss>>24)&0xff;
    desc->access=0x89;
    desc->granularity=0;
}

#endif /*__ASSEMBLY__*/
#endif /*_XM_KERNEL_*/

#endif
