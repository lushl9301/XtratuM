/*
 * $FILE: hm.c
 *
 * Health Monitor
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
#include <hm.h>
#include <xm_inc/objects/hm.h>

xm_s32_t XM_hm_open(void) {
    if (!(libXmParams.partCtrlTab[XM_get_vcpuid()]->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    return XM_OK;
}

xm_s32_t XM_hm_read(xmHmLog_t *hmLogPtr) {
    xm_s32_t ret;

    if (!(libXmParams.partCtrlTab[XM_get_vcpuid()]->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    if (!hmLogPtr){
        return XM_INVALID_PARAM;
    }
    ret=XM_read_object(OBJDESC_BUILD(OBJ_CLASS_HM, XM_HYPERVISOR_ID, 0), hmLogPtr, sizeof(xmHmLog_t), 0);
    return (ret>0) ? (ret/sizeof(xmHmLog_t)) : ret;

}

xm_s32_t XM_hm_seek(xm_s32_t offset, xm_u32_t whence) {
    if (!(libXmParams.partCtrlTab[XM_get_vcpuid()]->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    return XM_seek_object(OBJDESC_BUILD(OBJ_CLASS_HM, XM_HYPERVISOR_ID, 0), offset, whence);
}

//@ \void{<track id="using-proc">}
xm_s32_t XM_hm_status(xmHmStatus_t *hmStatusPtr) {
    if (!(libXmParams.partCtrlTab[XM_get_vcpuid()]->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    if (!hmStatusPtr) {
        return XM_INVALID_PARAM;
    }
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_HM, XM_HYPERVISOR_ID, 0), XM_HM_GET_STATUS, hmStatusPtr);
}
//@ \void{</track id="using-proc">}
