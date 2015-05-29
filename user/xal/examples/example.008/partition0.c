/*
 * $FILE: partition0.c
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


#define TPORT_NAME_SRC "ttnocSend1"
#define TPORT_NAME_DST "ttnocReceive1"
#define SIZE_CHANNEL 26

void PartitionMain(void)
{
    char tMessage[SIZE_CHANNEL];
    char rMessage[SIZE_CHANNEL];
    xm_s32_t tDescSrc;
    xm_s32_t tDescDst;
    xm_u32_t flags;
    int rtn,i=0;
    memset(rMessage,0,sizeof(rMessage));
    tDescSrc = XM_create_ttnoc_port(TPORT_NAME_SRC, SIZE_CHANNEL, XM_SOURCE_PORT, 0);
    if (tDescSrc < 0) {
        PRINT("Error creating port %s: %d\n", TPORT_NAME_SRC,tDescSrc);
        return;
    }
    PRINT("Port %s created\n",TPORT_NAME_SRC);

    tDescDst = XM_create_ttnoc_port(TPORT_NAME_DST, SIZE_CHANNEL, XM_DESTINATION_PORT, 0);
    if (tDescDst < 0) {
        PRINT("Error creating port %s: %d\n", TPORT_NAME_DST,tDescDst);
        return;
    }
    PRINT("Port %s created\n",TPORT_NAME_DST);
    sprintf(tMessage,"1234567890ABCDEFGHIJKLM");

while (1){
/*    PRINT("active\n");
    XM_idle_self(); 

    XM_idle_self(); 
    XM_idle_self();
*/
    do{
    rtn=XM_write_ttnoc_message(tDescSrc, tMessage, sizeof(tMessage));
    if (rtn<0)
       PRINT("Error writting ttnoc msg\n");
    else
       PRINT("Write ttnoc rtn=%d msg=->%s<-\n",rtn,tMessage);
    }while(rtn<0);
   
    XM_idle_self(); 
   
    if (i==1)
       sprintf(tMessage,"1234567890ABCDEFGHIJKLM");
    else
       sprintf(tMessage,"Hola mundo - hola mundo");
    i=(++i%2);

    do{ 
    rtn=XM_read_ttnoc_message(tDescDst, rMessage, sizeof(rMessage),&flags);
    if (rtn<0)
       PRINT("Error reading ttnoc msg\n");
    else
       PRINT("Read ttnoc rtn=%d msg=->%s<-\n",rtn,rMessage);
    }while(rtn<0);
}
    PRINT("Halting\n");
    XM_halt_partition(XM_PARTITION_SELF);
}

