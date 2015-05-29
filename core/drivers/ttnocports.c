/*
 * $FILE: ttnocports.c
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <boot.h>
#include <rsvmem.h>
#include <kdevice.h>
#include <spinlock.h>
#include <sched.h>
#include <stdc.h>
#include <virtmm.h>
#include <vmmap.h>
#include <hypercalls.h>
#include <arch/physmm.h>
#include <drivers/ttnocports.h>

//#define DEBUG_TTNOC 1
//#define TSIM_SIMULATOR  1

#define TTSOC_ID_RX_NODE_0     1
#define TTSOC_ID_TX_NODE_0     2

#define TTSOC_ID_RX_NODE_1     2
#define TTSOC_ID_TX_NODE_1     1

#define SIZE_WORD_TTNOC 64    // Size is 256 bytes= 64*4

static kDevice_t *ttnocPortTab=0;
xm_u32_t oldHypSeq=0;
xm_u32_t oldPartSeq=0;
struct messageTTNoC xmMessageTTNoC;
struct messageTTNoC xmMessageTTNoCRemoteRx[CONFIG_TTNOC_NODES];
struct messageTTNoC xmMessageTTNoCRemoteTx[CONFIG_TTNOC_NODES];

extern xm_u32_t resetStatusInit[];

static const kDevice_t *GetTTnocPort(xm_u32_t subId) {
    return &ttnocPortTab[subId];
}


void updateInfoNode(){
  int i;
  xmMessageTTNoC.infoNode.nodeId=xmcTab.hpv.nodeId;
  xmMessageTTNoC.infoNode.noParts=xmcTab.noPartitions;
  xmMessageTTNoC.infoNode.noSchedPlans=xmcTab.hpv.cpuTab[0].noSchedCyclicPlans;
  for (i=0;i<CONFIG_TTNOC_NODES;i++)
    xmMessageTTNoCRemoteTx[i].infoNode=xmMessageTTNoC.infoNode;
}

void updateStateHyp(xm_u32_t stateHyp){
  localSched_t *sched=GET_LOCAL_SCHED();
//  xmMessageTTNoC.stateHyp.seq++;
  xmMessageTTNoC.stateHyp.stateHyp=stateHyp;
  if (sched->data->cyclic.plan.current)
     xmMessageTTNoC.stateHyp.currSchedPlan=sched->data->cyclic.plan.current->id;
  else
     xmMessageTTNoC.stateHyp.currSchedPlan=0;
}

void updateStateAllPart(){
  int i;
  for (i=0;i<xmcTab.noPartitions;i++){
	xmMessageTTNoC.statePart.partState&=~(0x3<<(i*2));
    if (AreKThreadFlagsSet(partitionTab[i].kThread[0], KTHREAD_HALTED_F))
      xmMessageTTNoC.statePart.partState|=(0x3&XM_STATUS_HALTED)<<(i*2);
    else if (AreKThreadFlagsSet(partitionTab[i].kThread[0], KTHREAD_SUSPENDED_F))
      xmMessageTTNoC.statePart.partState|=(0x3&XM_STATUS_SUSPENDED)<<(i*2);
    else if (AreKThreadFlagsSet(partitionTab[i].kThread[0], KTHREAD_READY_F))
      xmMessageTTNoC.statePart.partState|=(0x3&XM_STATUS_READY)<<(i*2);
    else
      xmMessageTTNoC.statePart.partState|=(0x3&XM_STATUS_IDLE)<<(i*2);
#ifdef DEBUG_TTNOC
    kprintf(">>> PartId=%d state=0x%x\n",i,xmMessageTTNoC.statePart.partState);
#endif
  }
}

void setManualStatePart(xm_u32_t stateBitMap){
	xmMessageTTNoC.statePart.partState=stateBitMap;
}

void updateStatePart(xm_s32_t partId){
  if (AreKThreadFlagsSet(partitionTab[partId].kThread[0], KTHREAD_HALTED_F))
    xmMessageTTNoC.statePart.partState=(0x3&XM_STATUS_HALTED)<<(partId*2);
  else if (AreKThreadFlagsSet(partitionTab[partId].kThread[0], KTHREAD_SUSPENDED_F))
    xmMessageTTNoC.statePart.partState=(0x3&XM_STATUS_SUSPENDED)<<(partId*2);
  else if (AreKThreadFlagsSet(partitionTab[partId].kThread[0], KTHREAD_READY_F))
    xmMessageTTNoC.statePart.partState=(0x3&XM_STATUS_READY)<<(partId*2);
  else
    xmMessageTTNoC.statePart.partState=(0x3&XM_STATUS_IDLE)<<(partId*2);
}

void setCommandHyp(xm_s32_t nodeId, xm_u32_t cmd){
  xmMessageTTNoCRemoteTx[nodeId].cmdHyp.seq++;
  xmMessageTTNoCRemoteTx[nodeId].cmdHyp.cmdHyp=cmd;
#ifdef DEBUG_TTNOC
  kprintf("set command hypervisor %d \n",cmd);
#endif
  if ((ttnocNodes[nodeId].nodeId!=-1)&&(ttnocNodes[nodeId].txSlot!=NULL)&&(xmcTab.hpv.nodeId!=nodeId))
         KDevWrite(ttnocNodes[nodeId].txSlot, NULL,0);
}

/*command for all partitions in the nodeId*/
void setCommandAllPart(xm_s32_t nodeId, xm_u32_t cmdBitMap){
  xmMessageTTNoCRemoteTx[nodeId].seqs.cmdPartSeq++;
  xmMessageTTNoCRemoteTx[nodeId].cmdPart.cmdPart=cmdBitMap;
  if ((ttnocNodes[nodeId].nodeId!=-1)&&(ttnocNodes[nodeId].txSlot!=NULL)&&(xmcTab.hpv.nodeId!=nodeId))
         KDevWrite(ttnocNodes[nodeId].txSlot, NULL,0);
}

void setCommandPart(xm_s32_t nodeId, xm_u32_t partId, xm_u32_t cmd){
  xmMessageTTNoCRemoteTx[nodeId].seqs.cmdPartSeq++;
  xmMessageTTNoCRemoteTx[nodeId].cmdPart.cmdPart&=~(0x3<<(partId*2));
  xmMessageTTNoCRemoteTx[nodeId].cmdPart.cmdPart=(0x3&cmd)<<(partId*2);
#ifdef DEBUG_TTNOC
  kprintf("set command partition %d \n",cmd);
#endif
  if ((ttnocNodes[nodeId].nodeId!=-1)&&(ttnocNodes[nodeId].txSlot!=NULL)&&(xmcTab.hpv.nodeId!=nodeId))
         KDevWrite(ttnocNodes[nodeId].txSlot, NULL,0);
}

void setCommandNewSchedPlan(xm_s32_t nodeId, xm_s32_t newSchedPlan){
  xmMessageTTNoCRemoteTx[nodeId].seqs.cmdSchedSeq++;
  xmMessageTTNoCRemoteTx[nodeId].cmdHyp.newSchedPlan=newSchedPlan;
#ifdef DEBUG_TTNOC
  kprintf("set command new sched plan: %d \n",newSchedPlan);
#endif
  if ((ttnocNodes[nodeId].nodeId!=-1)&&(ttnocNodes[nodeId].txSlot!=NULL)&&(xmcTab.hpv.nodeId!=nodeId))
         KDevWrite(ttnocNodes[nodeId].txSlot, NULL,0);
}

void sendStateToAllNodes(){
  xm_s32_t nodeId;
  for (nodeId=0;nodeId<CONFIG_TTNOC_NODES;nodeId++){
    if ((ttnocNodes[nodeId].nodeId!=-1)&&(ttnocNodes[nodeId].txSlot!=NULL)&&(xmcTab.hpv.nodeId!=nodeId)){
       if (KDevWrite(ttnocNodes[nodeId].txSlot, NULL,0)<0)
    	   KDevWrite(ttnocNodes[nodeId].txSlot, NULL,0);
#ifdef DEBUG_TTNOC
       kprintf(">>>[%d]Tx cmdHyp=%d cmdPart=%d\n",nodeId,xmMessageTTNoCRemoteTx[nodeId].cmdHyp.cmdHyp,xmMessageTTNoCRemoteTx[nodeId].cmdPart.cmdPart);
#endif
//       xmMessageTTNoCRemoteTx[nodeId].cmdPart.cmdPart=0;
//       xmMessageTTNoCRemoteTx[nodeId].cmdHyp.setNewPlan=0;
//       xmMessageTTNoCRemoteTx[nodeId].cmdHyp.cmdHyp=0;
    }
  }
}

void receiveStateFromAllNodes(){
  xm_s32_t nodeId;
  for (nodeId=0;nodeId<CONFIG_TTNOC_NODES;nodeId++){
	if ((ttnocNodes[nodeId].nodeId!=-1)&&(ttnocNodes[nodeId].rxSlot!=NULL)&&(xmcTab.hpv.nodeId!=nodeId)){
	   KDevRead(ttnocNodes[nodeId].rxSlot, NULL,0);
#ifdef DEBUG_TTNOC
	   kprintf(">>>Rx cmdHyp=%d cmdPart=%d seqHyp=%d seqPart=%d seqSched=%d\n",xmMessageTTNoCRemoteRx[nodeId].cmdHyp.cmdHyp,xmMessageTTNoCRemoteRx[nodeId].cmdPart.cmdPart,xmMessageTTNoCRemoteRx[nodeId].cmdHyp.seq,xmMessageTTNoCRemoteRx[nodeId].seqs.cmdPartSeq,xmMessageTTNoCRemoteRx[nodeId].seqs.cmdSchedSeq);
#endif
	   checkCmdFromNode(nodeId);
	}
  }
}

void receiveInitStateFromAllNodes(){
  xm_s32_t nodeId;
  for (nodeId=0;nodeId<CONFIG_TTNOC_NODES;nodeId++){
	if ((ttnocNodes[nodeId].nodeId!=-1)&&(ttnocNodes[nodeId].rxSlot!=NULL)&&(xmcTab.hpv.nodeId!=nodeId)){
	   KDevRead(ttnocNodes[nodeId].rxSlot, NULL,0);
	   ttnocNodes[nodeId].seqCmdHyp=xmMessageTTNoCRemoteRx[nodeId].cmdHyp.seq;
	   ttnocNodes[nodeId].seqCmdPart=xmMessageTTNoCRemoteRx[nodeId].seqs.cmdPartSeq;
	   ttnocNodes[nodeId].seqCmdSched=xmMessageTTNoCRemoteRx[nodeId].seqs.cmdSchedSeq;
#ifdef DEBUG_TTNOC
	   kprintf("Init seqHyp=%d seqPart=%d\n",ttnocNodes[nodeId].seqCmdHyp,ttnocNodes[nodeId].seqCmdPart);
#endif
	}
  }
}

void checkCmdFromNode(xmId_t nodeId){
	xmTTNoCCmdHyp_t *cmdHyp=&xmMessageTTNoCRemoteRx[nodeId].cmdHyp;
	xmTTNoCCmdPart_t *cmdPart=&xmMessageTTNoCRemoteRx[nodeId].cmdPart;
#ifdef DEBUG_TTNOC
	kprintf("Check seqHypMem=%d seqHypMsg=%d\n",ttnocNodes[nodeId].seqCmdHyp,cmdHyp->seq);
#endif
	/*Check commands to the hypervisor*/
   if ((cmdHyp->cmdHyp!=TTNOC_CMD_NOTHING)&&(cmdHyp->seq!=ttnocNodes[nodeId].seqCmdHyp)){
	   ttnocNodes[nodeId].seqCmdHyp=cmdHyp->seq;
	   switch (cmdHyp->cmdHyp){
	   case TTNOC_CMD_HALT:
		   HwCli();
#ifdef CONFIG_DEBUG
           kprintf("Remote ");
#endif
		   HaltSystem();
		   break;
	   case TTNOC_CMD_COLD_RESET:
		   resetStatusInit[0]=(XM_RESET_STATUS_PARTITION_NORMAL_START<<XM_HM_RESET_STATUS_USER_CODE_BIT);
		   ResetSystem(XM_COLD_RESET);
		   break;
	   case TTNOC_CMD_WARM_RESET:
		   resetStatusInit[0]=(XM_RESET_STATUS_PARTITION_NORMAL_START<<XM_HM_RESET_STATUS_USER_CODE_BIT);
		   ResetSystem(XM_WARM_RESET);
		   break;
	   }
   }
#ifdef DEBUG_TTNOC
	kprintf("newSchedPlan=%d Check seqSchedMem=%d seqSchedMsg=%d\n",cmdHyp->newSchedPlan, ttnocNodes[nodeId].seqCmdSched,xmMessageTTNoCRemoteRx[nodeId].seqs.cmdSchedSeq);
#endif
	/*Check commands to switch scheduling plan*/
   if (xmMessageTTNoCRemoteRx[nodeId].seqs.cmdSchedSeq!=ttnocNodes[nodeId].seqCmdSched){
	   ttnocNodes[nodeId].seqCmdSched=xmMessageTTNoCRemoteRx[nodeId].seqs.cmdSchedSeq;
	   if ((cmdHyp->newSchedPlan>=0)&&(cmdHyp->newSchedPlan<xmcTab.hpv.cpuTab[GET_CPU_ID()].noSchedCyclicPlans)){
		   xm_s32_t currentPlanId;
#ifdef DEBUG_TTNOC
		   kprintf(">>>switchplan\n");
#endif
		   SwitchSchedPlan(cmdHyp->newSchedPlan, &currentPlanId);
	   }
#ifdef DEBUG_TTNOC
	   else
		   kprintf(">>>No switchplan\n");
#endif
   }
#ifdef DEBUG_TTNOC
   kprintf("Check seqPartMem=%d seqPartMsg=%d\n",ttnocNodes[nodeId].seqCmdPart,xmMessageTTNoCRemoteRx[nodeId].seqs.cmdPartSeq);
#endif
	/*Check commands to the partitions*/
   if ((cmdPart->cmdPart!=TTNOC_CMD_NOTHING)&&(xmMessageTTNoCRemoteRx[nodeId].seqs.cmdPartSeq!=ttnocNodes[nodeId].seqCmdPart)){
	   int i,rtn;//,flag=0;;

	   ttnocNodes[nodeId].seqCmdPart=xmMessageTTNoCRemoteRx[nodeId].seqs.cmdPartSeq;
	   for (i=0;i<xmcTab.noPartitions;i++){
		   int cmdParts=0;
		   cmdParts=(cmdPart->cmdPart)&(0x3<<(i*2));
		   switch (cmdParts){
		   case TTNOC_CMD_NOTHING:
			   break;
		   case TTNOC_CMD_COLD_RESET:
			   rtn=ResetPartition(&partitionTab[i], XM_COLD_RESET&XM_RESET_MODE, 0);
//			   flag++;
			   break;
		   case TTNOC_CMD_WARM_RESET:
			   rtn=ResetPartition(&partitionTab[i], XM_WARM_RESET&XM_RESET_MODE, 0);
//			   flag++;
			   break;
		   case TTNOC_CMD_HALT:
			   HALT_PARTITION(i);
#ifdef CONFIG_DEBUG
               kprintf("[HYPERCALL] (0x%x) Remote Halted\n", i);
#endif
//			   flag++;
			   break;
		   }
        }
//	   if (flag)
//		   Schedule();
   }
}

/*By optimization and due to operation of TTNoC, the input "buffer" is only used by xmTTnocChannels - in other uses "len" should be equal 0*/
static xm_s32_t ReadTTnocPort(const kDevice_t *kDev, xm_u8_t *buffer, xmSSize_t len) {
    xm_s32_t rtn,i;

#ifdef DEBUG_TTNOC
    eprintf("[TTNoCDriver]Reading on port=%d\n",xmcTTnocSlotTab[kDev->subId].ttsocId);
#endif
    if (xmcTTnocSlotTab[kDev->subId].type==XM_SOURCE_PORT){
#ifdef CONFIG_DEBUG
       eprintf("TTNoC driver: Error reading from port %d. Type of port incorrect: XM_SOURCE_PORT\n",xmcTTnocSlotTab[kDev->subId].ttsocId);
#endif
       return -1;
    }
    
#ifndef TSIM_SIMULATOR
    rtn=ttsoc_receiveMsg(0, xmcTTnocSlotTab[kDev->subId].ttsocId, (xm_u32_t *)&xmMessageTTNoCRemoteRx[xmcTTnocSlotTab[kDev->subId].nodeId], 0);
#else
    eprintf("[TTNoCDriver]receiving-tmpBuffer=%s\n",xmMessageTTNoCRemoteRx[xmcTTnocSlotTab[kDev->subId].nodeId].msgXmTTnocChannel);
    rtn=0;
#endif
    if (len>MAX_SIZE_XM_TTNOC_CHANNEL)
      len=MAX_SIZE_XM_TTNOC_CHANNEL;
  
    for (i=0;i<len;i++)
        buffer[i]=(xm_u8_t)xmMessageTTNoCRemoteRx[xmcTTnocSlotTab[kDev->subId].nodeId].msgXmTTnocChannel[i];

    if (rtn<0){
#ifdef CONFIG_DEBUG
       eprintf("TTNoC driver: Error reading from port %d\n",xmcTTnocSlotTab[kDev->subId].ttsocId);
#endif
       return -1;
    }
    else
       return len;
}

/*By optimization and due to operation of TTNoC, the input "buffer" is only used by xmTTnocChannels - in other uses "len" should be equal 0*/
static xm_s32_t WriteTTnocPort(const kDevice_t *kDev, xm_u8_t *buffer, xmSSize_t len) {
    xm_s32_t rtn,i;

#ifdef DEBUG_TTNOC
    eprintf("[TTNoCDriver]writing on port=%d\n",xmcTTnocSlotTab[kDev->subId].ttsocId);
#endif
    
    if (xmcTTnocSlotTab[kDev->subId].type==XM_DESTINATION_PORT){
#ifdef CONFIG_DEBUG
       eprintf("TTNoC driver: Error writting to port %d. Type of port incorrect: XM_DESTINATION_PORT\n",xmcTTnocSlotTab[kDev->subId].ttsocId);
#endif
       return -1;
    }

    if (len>MAX_SIZE_XM_TTNOC_CHANNEL)
      len=MAX_SIZE_XM_TTNOC_CHANNEL;
 
    for (i=0;i<len;i++)
        xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId].msgXmTTnocChannel[i]=(xm_u32_t)buffer[i];

    xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId].stateHyp=xmMessageTTNoC.stateHyp;
    xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId].statePart=xmMessageTTNoC.statePart;
       //memcpy(tmpBuffer,buffer,len);
#ifdef TSIM_SIMULATOR
       char *charc;
       charc=(char *)&xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId];
       eprintf("[TTNoCDriver]sendMsg-tmpBuffer\n");
       rtn=0;
//        kprintf("[TTNoCDriver]stateHyp=%d noPartitions=%d noSchedPlans=%d\n",xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId].stateHyp.stateHyp,xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId].infoNode.noParts,xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId].infoNode.noSchedPlans);
#else
       rtn=ttsoc_sendMsg(0, xmcTTnocSlotTab[kDev->subId].ttsocId, (xm_u32_t *)&xmMessageTTNoCRemoteTx[xmcTTnocSlotTab[kDev->subId].nodeId], 0);
#endif

    if (rtn<0){
#ifdef CONFIG_DEBUG
       eprintf("TTNoC driver: Error %d writing on port %d\n",rtn,xmcTTnocSlotTab[kDev->subId].ttsocId);
#endif
       return -1;
    }
    else
       return len;
}


xm_s32_t __VBOOT InitTTnocPort(void) {
    xm_s32_t e;
    xm_u8_t ttsocIdRx,ttsocIdTx;

    eprintf("[TTNoCDriver]Init TTNoCPort\n");

    for (e=0;e<CONFIG_TTNOC_NODES;e++){
        ttnocNodes[e].nodeId=-1;
        ttnocNodes[e].devIdTx.id=-1;
	    ttnocNodes[e].devIdTx.subId=-1;
        ttnocNodes[e].devIdRx.id=-1;
	ttnocNodes[e].devIdRx.subId=-1;
	ttnocNodes[e].txSlot=NULL;
	ttnocNodes[e].rxSlot=NULL;
	ttnocNodes[e].seqCmdHyp=0;
	ttnocNodes[e].seqCmdPart=0;
    }
    memset(&xmMessageTTNoC, 0, sizeof(struct messageTTNoC));
    memset(xmMessageTTNoCRemoteRx, 0, CONFIG_TTNOC_NODES*sizeof(struct messageTTNoC));
    memset(xmMessageTTNoCRemoteTx, 0, CONFIG_TTNOC_NODES*sizeof(struct messageTTNoC));

    GET_MEMZ(ttnocPortTab, sizeof(kDevice_t)*xmcTab.deviceTab.noTTnocSlots);
    for (e=0; e<xmcTab.deviceTab.noTTnocSlots; e++) {


        ttnocPortTab[e].subId=e;
//        ttnocPortTab[e].Reset=;
        ttnocPortTab[e].Write=WriteTTnocPort;
        ttnocPortTab[e].Read=ReadTTnocPort;
//        ttnocPortTab[e].Seek=;
      
        if ((xmcTTnocSlotTab[e].nodeId!=-1)&&(xmcTTnocSlotTab[e].nodeId<CONFIG_TTNOC_NODES)){
           ttnocNodes[xmcTTnocSlotTab[e].nodeId].nodeId=xmcTTnocSlotTab[e].nodeId;
           if (xmcTTnocSlotTab[e].type==XM_SOURCE_PORT){
              ttnocNodes[xmcTTnocSlotTab[e].nodeId].devIdTx=xmcTTnocSlotTab[e].devId;
              ttnocNodes[xmcTTnocSlotTab[e].nodeId].txSlot=&ttnocPortTab[e];
           }
           else{
              ttnocNodes[xmcTTnocSlotTab[e].nodeId].devIdRx=xmcTTnocSlotTab[e].devId;
              ttnocNodes[xmcTTnocSlotTab[e].nodeId].rxSlot=&ttnocPortTab[e];
           }
        }

        /** Init hardware **/
#ifndef TSIM_SIMULATOR
        if (xmcTTnocSlotTab[e].type==XM_DESTINATION_PORT){
	   eprintf("[TTNoCDriver] SetupTTPortCfg Start Incoming ttsocId=%d\n",xmcTTnocSlotTab[e].ttsocId);
           SetupTTPortCfg(xmcTTnocSlotTab[e].ttsocId, xmcTTnocSlotTab[e].size>>2, incoming);
	}
        else{
	   eprintf("[TTNoCDriver] SetupTTPortCfg Start Outgoing ttsocId=%d\n",xmcTTnocSlotTab[e].ttsocId);
           SetupTTPortCfg(xmcTTnocSlotTab[e].ttsocId, xmcTTnocSlotTab[e].size>>2, outgoing);
	}
#ifdef DEBUG_TTNOC
	eprintf("[TTNoCDriver] SetupTTPortCfg Done\n");
    PrintTTPortCfg(xmcTTnocSlotTab[e].ttsocId);
#endif
#endif
    }
#ifndef TSIM_SIMULATOR
    /** TISS configuration **/
    TISSConfiguration();
#endif
#ifdef DEBUG_TTNOC
        for (e=0;e<CONFIG_TTNOC_NODES;e++){
	  eprintf("[TTNoCDriver] %d - nodeId=%d - devIdTx=%d -%d devRx=%d - %d\n",e,ttnocNodes[e].nodeId, ttnocNodes[e].devIdTx.id,ttnocNodes[e].devIdTx.subId,ttnocNodes[e].devIdRx.id,ttnocNodes[e].devIdRx.subId);
	  eprintf("[TTNoCDriver] rxSlot=0x%x txSlot=0x%x\n",ttnocNodes[e].rxSlot,ttnocNodes[e].txSlot);
	}
#endif
	  



    GetKDevTab[XM_DEV_TTNOC_ID]=GetTTnocPort;

    return 0;
}

#ifdef CONFIG_DEV_TTNOC_MODULE
XM_MODULE("TTnocPort", InitTTnocPort, DRV_MODULE);
#else
REGISTER_KDEV_SETUP(InitTTnocPort);
#endif

