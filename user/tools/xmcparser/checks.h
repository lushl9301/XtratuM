/*
 * $FILE: checks.h
 *
 * checks definitions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _CHECKS_H_
#define _CHECKS_H_
#include "xmc.h"

extern void CheckAllMemAreas(struct xmcMemoryArea *mA, struct xmcMemoryAreaNoL *mANoL, int len);
extern void CheckAllMemReg(void);
extern void CheckHwIrq(int line, int lineNo);
extern void CheckPortName(int port, int partition);
extern void CheckMemoryRegion(int region);
extern int CheckPhysMemArea(int memArea);
extern void CheckMemAreaPerPart(void);
extern void CheckMemBlockReg(void);
extern void CheckMemBlock(int mB);
extern void CheckHpvMemAreaFlags(void);
extern void CheckUartId(int uartId, int line);
extern void CheckIoMmuArea(int memArea);
#ifdef CONFIG_CYCLIC_SCHED
extern void CheckSchedCyclicPlan(struct xmcSchedCyclicPlan *plan, struct xmcSchedCyclicPlanNoL *planNoL);
extern void CheckCyclicPlanPartitionId(void);
extern void CheckCyclicPlanVCpuId(void);
extern void CheckSmpCyclicRestrictions(void);
#endif
extern void CheckPartitionName(char *name, int line);
extern void HmHpvIsActionPermittedOnEvent(int event, int action, int line);
extern void HmPartIsActionPermittedOnEvent(int event, int action, int line);
extern void CheckIoPorts(void);
extern void HmCheckExistMaintenancePlan(void);
extern void CheckIpviTab(void);
extern void CheckMaxNoKThreads(void);
extern void CheckPartNotAllocToMoreThanACpu(void);
#ifdef CONFIG_FP_SCHED
extern void CheckFPVCpuId(void);
#endif

#endif
