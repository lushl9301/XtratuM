/*
 * $FILE: conv.h
 *
 * convertion definitions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _CONV_H_
#define _CONV_H_

extern xm_u32_t ToRegionFlags(char *s);
extern xm_u32_t ToVersion(char *s);
extern xm_u32_t ToU32(char *s, int base);
extern xm_u32_t ToFreq(char *s);
extern xm_u32_t ToTime(char *s);
extern xmSize_t ToSize(char *s);
extern xm_u32_t ToPartitionFlags(char *s, int line);
extern xm_u32_t ToPhysMemAreaFlags(char *s, int line);
extern xm_u32_t ToHmAction(char *s, int line);
extern xm_u32_t ToHmEvent(char *s, int line);
extern xm_u32_t ToBitmaskTraceHyp(char *s, int line);
extern void ToHwIrqLines(char *s, int lineNo);
extern int ToYNTF(char *s, int line);
extern xm_u32_t ToCommPortDirection(char *s, int line);
extern xm_u32_t ToCommPortType(char *s, int line);
extern void CheckNoNodeId(int nodeId, int lineNo);
extern void ProcessIdList(char *s, void (*CallBack)(int, char *), int line);

#endif
