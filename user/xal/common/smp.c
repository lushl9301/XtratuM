/*
 * $FILE: smp.c
 *
 * Generic routines smp
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
#include <config.h>

extern struct xmImageHdr xmImageHdr;    
extern xmAddress_t start[];

void SetupVCpus(void){
  int i;
  if (XM_get_vcpuid()==0)
       for (i=1;i<XM_get_number_vcpus();i++)
          XM_reset_vcpu(i, xmImageHdr.pageTable, (xmAddress_t)start, 0);
}

