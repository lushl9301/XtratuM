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

#define QPORT_NAME      "portQ"
#define SPORT_NAME      "portS"
#ifdef CONFIG_SPARCv8
#define SHARED_ADDRESS  0x40180000
#else
#define SHARED_ADDRESS  0x6300000
#endif

#define PRINT(...) do { \
        printf("[%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

char sMessage[32];
char qMessage[32];
xm_s32_t qDesc, sDesc, seq;

void QueuingExtHandler(trapCtxt_t *ctxt)
{
    if (XM_receive_queuing_message(qDesc, qMessage, sizeof(qMessage)) > 0) {
        PRINT("RECEIVE %s\n", qMessage);
        PRINT("SHM WRITE %d\n", seq);
        *(volatile xm_u32_t *)SHARED_ADDRESS = seq++;
    }
}

void SamplingExtHandler(trapCtxt_t *ctxt)
{
    xm_u32_t flags;

    if (XM_read_sampling_message(sDesc, sMessage, sizeof(sMessage), &flags) > 0) {
        PRINT("RECEIVE %s\n", sMessage);
    }
}

void PartitionMain(void)
{
    PRINT("Opening ports...\n");
    qDesc = XM_create_queuing_port(QPORT_NAME, 16, 128, XM_DESTINATION_PORT);
    if (qDesc < 0) {
        PRINT("error %d\n", qDesc);
        goto end;
    }
    sDesc = XM_create_sampling_port(SPORT_NAME, 128, XM_DESTINATION_PORT, 0);
    if (sDesc < 0) {
        PRINT("error %d\n", sDesc);
        goto end;
    }
    PRINT("done\n");

    InstallTrapHandler(224+XM_VT_EXT_SAMPLING_PORT, SamplingExtHandler);
    InstallTrapHandler(224+XM_VT_EXT_QUEUING_PORT, QueuingExtHandler);
    HwSti();
    XM_clear_irqmask(0, (1<<XM_VT_EXT_SAMPLING_PORT)|(1<<XM_VT_EXT_QUEUING_PORT));/* Unmask port irqs */

    PRINT("Waiting for messages\n");
    while (1);

end:
    XM_halt_partition(XM_PARTITION_SELF);
}
