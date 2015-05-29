/*
 * $FILE: hypervisor.c
 *
 * Hypervisor related functions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xm.h>
#include <hypervisor.h>
#include <xmhypercalls.h>
#include <arch/atomic_ops.h>

#include <xm_inc/bitwise.h>
#include <xm_inc/linkage.h>
#include <xm_inc/hypercalls.h>
#include <xm_inc/objdir.h>
#include <xm_inc/objects/mem.h>

xm_s32_t XM_write_console(char *buffer, xm_s32_t length) {
    return XM_write_object(OBJDESC_BUILD(OBJ_CLASS_CONSOLE, XM_PARTITION_SELF, 0), buffer, length, 0);
}

xm_s32_t XM_memory_copy(xmId_t dstId, xm_u32_t dstAddr, xmId_t srcId, xm_u32_t srcAddr, xm_u32_t size) {
    union memCmd args;
    args.cpyArea.dstId=dstId;
    args.cpyArea.srcId=srcId;
    args.cpyArea.dstAddr=dstAddr;
    args.cpyArea.srcAddr=srcAddr;
    args.cpyArea.size=size;
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_MEM, 0, 0), XM_OBJ_MEM_CPY_AREA, (void *)&args);
}
