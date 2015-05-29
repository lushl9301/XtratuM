/*
 * $FILE: queue.h
 *
 * Queue
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_QUEUE_H_
#define _XM_QUEUE_H_

#if 0
#include <assert.h>

struct queue {
    xm_s32_t tail, elem, head, maxNoElem;
    xmSize_t elemSize;
    void *buffer;
};

static inline void QueueInit(struct queue *q, xm_s32_t maxNoElem, xmSize_t elemSize, void *buffer) {
    ASSERT(maxNoElem>0);
    ASSERT(elemSize>0);
    ASSERT(buffer);
    q->head=0;
    q->tail=0;
    q->elem=0;
    q->elemSize=elemSize;
    q->maxNoElem=maxNoElem;
    q->buffer=buffer;
}

static inline xm_s32_t QueueInsertElement(struct queue *q, void *element) {
    if (q->elem>=q->maxNoElem) 
        return -1;
    memcpy((void *)((xmAddress_t)q->buffer+(q->tail*q->elemSize)), element, q->elemSize);
    q->tail=((q->tail+1)<q->maxNoElem)?q->tail+1:0;
    q->elem++;

    return 0;
}

static inline xm_s32_t QueueExtractElement(struct queue *q, void *element) {
    if (q->elem>0) {
        memcpy(element, (void *)((xmAddress_t)q->buffer+(q->head*q->elemSize)), q->elemSize);
        q->head=((q->head+1)<q->maxNoElem)?q->head+1:0;
        q->elem--;
        return 0;
    }
    return -1;
}
#endif
#endif
