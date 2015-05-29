/*
 * $FILE: lazy.c
 *
 * Deferred hypercalls
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

typedef __builtin_va_list va_list;

#define va_start(v, l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v,l)

#define XM_BATCH_LEN 1024

static volatile xm_u32_t xmHypercallBatch[XM_BATCH_LEN];
static volatile xm_s32_t batchLen=0, prevBatchLen=0;

/*static struct {
    xm_u32_t noHyp;
    xm_s32_t error;
    };*/

__stdcall xm_s32_t XM_flush_hyp_batch(void) {
    if (batchLen) {
	xm_u32_t r;
	if((r=XM_multicall((void *)xmHypercallBatch, (void *)&xmHypercallBatch[batchLen]))<0) return r;
	prevBatchLen=batchLen;
	batchLen=0;
    }
    return XM_OK;
}

__stdcall void XM_lazy_hypercall(xm_u32_t noHyp, xm_s32_t noArgs, ...) {
    va_list args;
    xm_s32_t e;
    if ((batchLen>=XM_BATCH_LEN)||((batchLen+noArgs+1)>=XM_BATCH_LEN))
	XM_flush_hyp_batch();
    xmHypercallBatch[batchLen++]=noHyp;
    xmHypercallBatch[batchLen++]=noArgs;
    va_start(args, noArgs);
    for (e=0; e<noArgs; e++)
	xmHypercallBatch[batchLen++]=va_arg(args, xm_u32_t);
    va_end(args);
}

void init_batch(void) {
    batchLen=0;
    prevBatchLen=0;
}
