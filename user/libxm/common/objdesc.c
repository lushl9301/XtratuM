/*
 * $FILE: objdesc.c
 *
 * Object descriptors functions
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
#include <xm_inc/objdir.h>

xm_s32_t XM_objdesc_ready(xmObjDesc_t objDesc) {
    /*   xm_s32_t entry;
    xm_u32_t b, w;
     
    entry=OBJDESC_GET_ENTRY(objDesc);
    if ((entry>=0)&&(entry<=CONFIG_MAX_OBJDESCS)) {
	b=entry&((1<<5)-1); 
	w=entry>>4;
	if (XMAtomicGet(&XmParamsGetPCT()->objDescPend[w])&(1<<b)) {
	    XMAtomicClearMask(1<<b, (xmAtomic_t *)&XmParamsGetPCT()->objDescPend[w]);
	    return 1;
	}
    }
    */
    return 0;
}

