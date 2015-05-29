/*
 * $FILE: ttnocports.h
 *
 * TTNoC ports driver
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_TTNOCPORTS_DRV_H_
#define _XM_TTNOCPORTS_DRV_H_

#include <xmconf.h>
#include <kdevice.h>
#include <drivers/ttnoc/tiss_driver.h>

typedef struct {
    xm_u32_t unsued: 24,
        nodeId: 2,
        noParts:3,
        noSchedPlans:3;
} xmTTNoCInfoNode_t;

#define TTNOC_CMD_NOTHING      0
#define TTNOC_CMD_COLD_RESET   1
#define TTNOC_CMD_WARM_RESET   2
#define TTNOC_CMD_HALT         3

typedef struct {
    xm_u32_t unused: 24,
        seq: 3,
        newSchedPlan: 3,
        cmdHyp:2;               //0:Nothing | 1: COLD_RESET | 2: WARM_RESET | 3: HALT_RESET
} xmTTNoCCmdHyp_t;

typedef struct {
    xm_u32_t unused: 24,
        cmdPart: 8;   //2 bit by partition -> high partition0 |low partition3  //0:Nothing | 1: COLD_RESET | 2: WARM_RESET | 3: HALT_RESET

} xmTTNoCCmdPart_t;

typedef struct {
    xm_u32_t unused: 24,
        seq: 4,
        stateHyp: 2,
        currSchedPlan: 2;
} xmTTNoCStateHyp_t;

typedef struct {
    xm_u32_t unused: 24,
        partState: 8; //2 bit by partition -> high partition0 |low partition3
} xmTTNoCStatePart_t;

typedef struct {
    xm_u32_t unused: 24,
        cmdPartSeq: 4,
        cmdSchedSeq: 4;
} xmTTNoCCmdSeqs_t;


#define MAX_SIZE_TTNOC_MSG		64
#define MAX_SIZE_XM_TTNOC_CHANNEL 	57


/*Structure to 1 channel by node and 64Bytes availables - See below the restrictions for this implementation*/
struct messageTTNoC{
   xm_u32_t msgXmTTnocChannel[MAX_SIZE_XM_TTNOC_CHANNEL];
   xmTTNoCCmdSeqs_t seqs;           // [57]
   xmTTNoCCmdPart_t cmdPart;        // [58]
   xmTTNoCCmdHyp_t cmdHyp;          // [59]
   xmTTNoCStatePart_t statePart;	// [60]  Update   
   xmTTNoCStateHyp_t stateHyp;		// [61]  Update
   xmTTNoCInfoNode_t infoNode;		// [62]  Update 1 vez  
   xm_u32_t nothing;			// [63]
};

struct nodeInfoSys{
    xmId_t nodeId;
    xmDev_t devIdTx;
    xmDev_t devIdRx;
    kDevice_t *txSlot;
    kDevice_t *rxSlot;
    xm_u32_t seqCmdHyp;
    xm_u32_t seqCmdPart;
    xm_u32_t seqCmdSched;
} ttnocNodes[CONFIG_TTNOC_NODES];

void SetupTTPortCfg(ttsoc_portid slotID, ttsoc_ci_addr msgSize, ttsoc_direction dir);
void PrintTTPortCfg(ttsoc_portid slotID);
void TISSConfiguration();

void updateStateHyp(xm_u32_t stateHyp);
void updateInfoNode();
void updateStateAllPart();
void updateStatePart(xm_s32_t partId);
void setManualStatePart(xm_u32_t stateBitMap);

void sendStateToAllNodes();
void receiveStateFromAllNodes();
void receiveInitStateFromAllNodes();

void setCommandHyp(xm_s32_t nodeId, xm_u32_t cmd);
void setCommandAllPart(xm_s32_t nodeId, xm_u32_t cmdBitMap);
void setCommandPart(xm_s32_t nodeId, xm_u32_t partId, xm_u32_t cmd);
void setCommandNewSchedPlan(xm_s32_t nodeId, xm_s32_t newSchedPlan);
void checkCmdFromNode(xmId_t nodeId);


/*
Constrains:

- Max 4 partitions by node: 0, 1, 2, 3
- Max 4 scheduling plans: 0, 1, 2, 3
- Max 3 nodes: 0, 1, 2, 4
- Only 1 TTNoC-channel by every node: Max length-> 57 Bytes
- A single transmitter and one or more receivers
- When size TTNoC is 64Bytes:[0-63] --> 256 words but only is valid the first byte, therofore, only actual data to be sent: 256/4=64B  -> Byte [63] podría no ser tomado en cuenta
    * Info node:
      Byte [62]:
	Low->	2 bits: NodeId -> 0, 1, 2
		3 bits: Number of partitions -> 0-4
		3 bits: Number of scheduling plans -> 0-4
    * State Hypervisor/Partition:
      Byte [61]:
	Low->	4 bits: Sequence
		2 bits: State Hypervisor-> 0: Idle | 1: Ready | 2: Suspend | 3: Halted
	High->	2 bits: Current Scheduling plan-> 0-3
      Byte [60]:
	Low->  2 bits: State P0-> 0: Idle | 1: Ready | 2: Suspend | 3: Halted
		2 bits: State P0-> 0: Idle | 1: Ready | 2: Suspend | 3: Halted
		2 bits: State P0-> 0: Idle | 1: Ready | 2: Suspend | 3: Halted
	High->	2 bits: State P0-> 0: Idle | 1: Ready | 2: Suspend | 3: Halted
    * Command to modify Hypervisor/partitions:
      Byte [59]:
	Low->	3 bits: Sequence
	        3 bits: Number scheduling plan
	High->	2 bits: Command Hypervisor-> 0:Nothing | 1: COLD_RESET | 2: WARM_RESET | 3: HALT_RESET
      Byte [58]:
	Low->  2 bits: Command P0-> 0:Nothing | 1: COLD_RESET | 2: WARM_RESET | 3: HALT_RESET
		2 bits: Command P1-> 0:Nothing | 1: COLD_RESET | 2: WARM_RESET | 3: HALT_RESET
		2 bits: Command P2-> 0:Nothing | 1: COLD_RESET | 2: WARM_RESET | 3: HALT_RESET
	High->	2 bits: Command P3-> 0:Nothing | 1: COLD_RESET | 2: WARM_RESET | 3: HALT_RESET
*/

#endif
