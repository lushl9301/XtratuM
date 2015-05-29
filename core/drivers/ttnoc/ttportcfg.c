/*
 * ttportcfg.c
 * Javier Coronel (jcorone@fentiss.com)
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

#ifdef XM_SRC_PARTITION
#include <xm.h>
#include <tiss_driver.h>
#include <stdio.h>
#else
#ifdef _XM_KERNEL_
#include <drivers/ttnocports.h>
#include <stdc.h>
#else
#include <tiss_driver.h>>
#endif
#endif


//====================================================

void SetupTTPortCfg(ttsoc_portid slotID, ttsoc_ci_addr msgSize, ttsoc_direction dir){
    ttsoc_port *port;
    (void) ttsoc_getPortCfg(0,slotID);
    port = ttsoc_getPort(0,slotID);
    
    port->config.pcfg.PE = 1;
//     port->config.pcfg.MT = 1;   //Sporadic
    port->config.pcfg.MT = 0;   //Periodic
    port->msgsize = msgSize;
    port->dir = dir;
    (void) ttsoc_setPortCfg(0,slotID);
}

void TISSConfiguration(){
    //TISS configuration
    ttsoc_register reg;
    ttsoc_timestamp pPattern;
    ttsoc_timestamp pMask;

    reg.value = ttsoc_getErrorStatus(0);
    ttsoc_getTimerSettings(0,&pPattern,&pMask);
    reg.errstat.TimerEna = 1;
    reg.errstat.CommDis = 0;
    pPattern.Upper=0;
    pPattern.Lower=0x20;

    ttsoc_setTimerSettings(0,pPattern, pMask);
    ttsoc_setErrorStatus(0, reg.value);
    
   
}

void PrintTTPortCfg(ttsoc_portid slotID){
  ttsoc_port *port;
  port = ttsoc_getPort(0,slotID);
  eprintf("Port Configuration Memory\n");  
  eprintf("Empty=%d\n",port->config.pcfg.empty);
  eprintf("QLength=%d\n",port->config.pcfg.QLength);
  eprintf("offset sporadic msg - BaseAddr=0x%x\n",port->config.pcfg.BaseAddr);
  eprintf("Timestamp (0:disabled/1:enabled)=%d\n",port->config.pcfg.TS);
  eprintf("InterruptEnable (0:disabled/1:enabled)=%d\n",port->config.pcfg.IE);
  eprintf("Port Sync (0:explicit/1:implicit)=%d\n",port->config.pcfg.PS);
  eprintf("Message Type (0:periodic/1:sporadic)=%d\n",port->config.pcfg.MT);
  eprintf("Port Enable (0:disabled/1:enabled)=%d\n",port->config.pcfg.PE);
  eprintf("----\n");
}

