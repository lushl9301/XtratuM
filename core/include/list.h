/*
 * $FILE: list.h
 *
 * List
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_LIST_H_
#define _XM_LIST_H_

#ifndef __ASSEMBLY__
#include <assert.h>
#include <spinlock.h>

struct dynList;

struct dynListNode {
    struct dynList *list;
    struct dynListNode *prev, *next;
};

struct dynList {
    struct dynListNode *head;
    xm_s32_t noElem;
    spinLock_t lock;
};

static inline void DynListInit(struct dynList *l) {
    l->lock=SPINLOCK_INIT;
    SpinLock(&l->lock);
    l->noElem=0;
    l->head=0;
    SpinUnlock(&l->lock);
}

static inline xm_s32_t DynListInsertHead(struct dynList *l, struct dynListNode *e) {
    if (e->list) {
	ASSERT(e->list==l);
	return 0;
    }
    ASSERT(!e->next&&!e->prev);
    SpinLock(&l->lock);
    if (l->head) {
        ASSERT_LOCK(l->noElem>0, &l->lock);
	e->next=l->head;
	e->prev=l->head->prev;
	l->head->prev->next=e;
	l->head->prev=e;	
    } else {	
	ASSERT_LOCK(!l->noElem, &l->lock);
	e->prev=e->next=e;
    }
    l->head=e;
    l->noElem++;
    e->list=l;
    SpinUnlock(&l->lock);
    ASSERT(l->noElem>=0);
    return 0;
}

static inline xm_s32_t DynListInsertTail(struct dynList *l, struct dynListNode *e) {
    if (e->list) {
	ASSERT(e->list==l);
	return 0;
    }
    ASSERT(!e->next&&!e->prev);
    SpinLock(&l->lock);
    if (l->head) {
	e->next=l->head;
	e->prev=l->head->prev;
	l->head->prev->next=e;
	l->head->prev=e;	
    } else {
	e->prev=e->next=e;
	l->head=e;
    }
    l->noElem++;
    e->list=l;
    SpinUnlock(&l->lock);
    ASSERT(l->noElem>=0);

    return 0;
}

static inline void *DynListRemoveHead(struct dynList *l) {
    struct dynListNode *e=0;
    SpinLock(&l->lock);
    if (l->head) {
	e=l->head;
	l->head=e->next;
	e->prev->next=e->next;
	e->next->prev=e->prev;
	e->prev=e->next=0;
	e->list=0;
	l->noElem--;
	if (!l->noElem)
	    l->head=0;
    }
    SpinUnlock(&l->lock);
    ASSERT(l->noElem>=0);

    return e;
}

static inline void *DynListRemoveTail(struct dynList *l) {
    struct dynListNode *e=0;
    SpinLock(&l->lock);
    if (l->head) {
	e=l->head->prev;
	e->prev->next=e->next;
	e->next->prev=e->prev;
	e->prev=e->next=0;
	e->list=0;
	l->noElem--;
	if (!l->noElem)
	    l->head=0;
    }
    SpinUnlock(&l->lock);
    ASSERT(l->noElem>=0);

    return e;
}

static inline xm_s32_t DynListRemoveElement(struct dynList *l, struct dynListNode *e) {
    ASSERT(e->list==l);
    ASSERT(e->prev&&e->next);
    SpinLock(&l->lock);
    e->prev->next=e->next;
    e->next->prev=e->prev;
    if (l->head==e)
	l->head=e->next;
    e->prev=e->next=0;
    e->list=0;
    l->noElem--;
    if (!l->noElem)
	l->head=0;
    SpinUnlock(&l->lock);
    ASSERT(l->noElem>=0);

    return 0;
}

#define DYNLIST_FOR_EACH_ELEMENT_BEGIN(_l, _element, _cond) do { \
    xm_s32_t __e; \
    struct dynListNode *__n; \
    SpinLock(&(_l)->lock); \
    for (__e=(_l)->noElem, __n=(_l)->head, _element=(void *)__n; __e && (_cond); __e--, __n=__n->next, _element=(void *)__n) {


#define DYNLIST_FOR_EACH_ELEMENT_END(_l) \
    } \
    SpinUnlock(&(_l)->lock); \
} while(0)

#define DYNLIST_FOR_EACH_ELEMENT_EXIT(_l) SpinUnlock(&(_l)->lock)

#endif
#endif
