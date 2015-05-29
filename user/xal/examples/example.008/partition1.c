/*
 * $FILE: partition1.c
 *
 * Fent Innovative Software Solutions
 *
 * $LICENSE:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include <stdio.h>
#include <xm.h>
#include <irqs.h>

#define PRINT(...) do { \
        printf("[%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

#define TPORT_NAME_SRC "ttnocSend3"
#define TPORT_NAME_DST_2 "ttnocReceive2"
#define TPORT_NAME_DST_1 "ttnocReceive3"

#define NODE_1_PLAN_0   0
#define NODE_1_PLAN_1   1

volatile xm_s32_t lock;

void ExecTimerHandler(trapCtxt_t *ctxt)                                     /* XAL trap API */
{
    xmTime_t hw, exec;

    XM_get_time(XM_EXEC_CLOCK, &exec);
    XM_get_time(XM_HW_CLOCK, &hw);
    PRINT("[%lld:%lld] IRQ EXEC Timer\n", hw, exec);
    ++lock;
}

#define ID_NODE_REMOTE 1
#define ID_PARTITION_REMOTE 0

//#define XM_STATUS_IDLE 0x0
//#define XM_STATUS_READY 0x1
//#define XM_STATUS_SUSPENDED 0x2
//#define XM_STATUS_HALTED 0x3

void printStatusNode(xmSystemStatusRemote_t *node){
   PRINT("*** REMOTE NODE STATUS ***\n");
   PRINT("Id=%d\n",node->nodeId);
   PRINT("State=%d\n",node->state);
   PRINT("No Partitions=%d\n",node->noPartitions);
   PRINT("No Sched Plans=%d\n",node->noSchedPlans);
   PRINT("Current Plan=%d\n",node->currentPlan);
   PRINT("**************************\n");
}

void PartitionMain(void)
{
    char tMessage[26];
    xm_s32_t tDescSrc;
    xm_s32_t tDescDst;
    xm_s32_t tDescDst1;
    xm_u32_t flags;	
    xmSystemStatusRemote_t nodeStatus;
    xmPartitionStatusRemote_t partitionStatus;
    int rtn,oldPlan;


    PRINT("Init Partition\n");
    XM_idle_self();
    XM_idle_self();

    rtn=XM_get_system_node_status(ID_NODE_REMOTE,&nodeStatus);
    if (rtn<0)
        PRINT("Error get node status=%d\n",rtn);
    else
        printStatusNode(&nodeStatus);

    rtn=XM_get_partition_node_status(ID_NODE_REMOTE,ID_PARTITION_REMOTE,&partitionStatus);
    if (rtn<0)
        PRINT("Error get remote partition %d status=%d\n",ID_PARTITION_REMOTE,rtn);
    else
       PRINT("State remote partition %d=%d\n",ID_PARTITION_REMOTE,partitionStatus.state);

    XM_idle_self();
    XM_idle_self();
    rtn=XM_switch_sched_plan_node(ID_NODE_REMOTE,NODE_1_PLAN_1,&oldPlan);
    if (rtn<0)
        PRINT("Error switching remote sched plan on node %d=%d\n",ID_NODE_REMOTE,rtn);
    else
        PRINT("Switching remote node %d - newPlan=%d\n",ID_NODE_REMOTE,NODE_1_PLAN_1);

    XM_idle_self();
    PRINT("Idle Time\n");
    XM_idle_self();
    XM_idle_self();

    rtn=XM_get_system_node_status(ID_NODE_REMOTE,&nodeStatus);
    if (rtn<0)
        PRINT("Error get node status=%d\n",rtn);
    else
        printStatusNode(&nodeStatus);

    rtn=XM_switch_sched_plan_node(ID_NODE_REMOTE,NODE_1_PLAN_0,&oldPlan);
    if (rtn<0)
        PRINT("Error switching remote sched plan on node %d=%d\n",ID_NODE_REMOTE,rtn);
    else
        PRINT("Switching remote node %d - newPlan=%d\n",ID_NODE_REMOTE,NODE_1_PLAN_0);

    XM_idle_self();
    PRINT("Idle Time\n");
    XM_idle_self();
    PRINT("Idle Time\n");
    XM_idle_self();
    XM_idle_self();
    rtn=XM_get_system_node_status(ID_NODE_REMOTE,&nodeStatus);
    if (rtn<0)
        PRINT("Error get node status=%d\n",rtn);
    else
        printStatusNode(&nodeStatus);
    XM_idle_self();

    rtn=XM_halt_partition_node(1,0);
    if (rtn<0)
        PRINT("Error halting remote partition %d=%d\n",ID_PARTITION_REMOTE,rtn);
    else
        PRINT("Remote Paritition %d halted\n",ID_PARTITION_REMOTE);
       
    XM_idle_self();

    rtn=XM_get_partition_node_status(ID_NODE_REMOTE,ID_PARTITION_REMOTE,&partitionStatus);
    if (rtn<0)
        PRINT("Error get remote partition %d status=%d\n",ID_PARTITION_REMOTE,rtn);
    else
       PRINT("State remote partition %d=%d\n",ID_PARTITION_REMOTE,partitionStatus.state);

    XM_idle_self();
    PRINT("Idle Time\n");
    XM_idle_self();
    rtn=XM_halt_system_node(ID_NODE_REMOTE);
    if (rtn<0)
       PRINT("Error halting remote system node %d= %d\n",ID_NODE_REMOTE,rtn);
    else
       PRINT("Halting remote system node %d\n",ID_NODE_REMOTE);
    while(1);
}
