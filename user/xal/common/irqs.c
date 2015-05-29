/*
 * $FILE: irqs.c
 *
 * Generic traps' handler
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
#include <stdio.h>
#include <irqs.h>

trapHandler_t trapHandlersTab[TRAPTAB_LENGTH];

static void UnexpectedTrap(trapCtxt_t *ctxt) {
#ifdef CONFIG_SPARCv8
    printf("[P%d:%d] Unexpected trap 0x%x (pc: 0x%x)\n", XM_PARTITION_SELF,XM_get_vcpuid(), ctxt->irqNr, ctxt->pc);
#endif
#ifdef CONFIG_x86
    printf("[P%d:%d] Unexpected trap 0x%x (ip: 0x%x)\n", XM_PARTITION_SELF,XM_get_vcpuid(), ctxt->irqNr, ctxt->ip);
#endif
}

xm_s32_t InstallTrapHandler(xm_s32_t trapNr, trapHandler_t handler) {
    if (trapNr<0||trapNr>TRAPTAB_LENGTH)
        return -1;

    if (handler)
        trapHandlersTab[trapNr]=handler;
    else
        trapHandlersTab[trapNr]=UnexpectedTrap;
    return 0;
}

void SetupIrqs(void) {
    xm_s32_t e;

    if (XM_get_vcpuid()==0)
       for (e=0; e<TRAPTAB_LENGTH; e++)
            trapHandlersTab[e]=UnexpectedTrap;

    for (e=0; e<XM_VT_EXT_MAX; e++) 
        XM_route_irq(XM_EXTIRQ_TYPE, e, 224+e);
}
