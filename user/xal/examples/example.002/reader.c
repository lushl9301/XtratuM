/*
 * $FILE: reader.c
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
#include <stdio.h>
#include <xm.h>

#define PRINT(...) do { \
        printf("[%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

#define HALTED 3

static void PrintHmLog(xmHmLog_t *hmLog) {
    printf("part_Id: 0x%x eventId: 0x%x timeStamp: %lld\n", hmLog->opCodeL & HMLOG_OPCODE_PARTID_MASK, (hmLog->opCodeL & HMLOG_OPCODE_EVENT_MASK)>>HMLOG_OPCODE_EVENT_BIT,
            hmLog->timestamp);
}

void PartitionMain(void) {

    xmPartitionStatus_t partStatus;
    xmHmStatus_t hmStatus;
    xmHmLog_t hmLog;

    XM_idle_self();

    PRINT(" --------- Health Monitor Log ---------------\n");
    while (1) {
        XM_hm_status(&hmStatus);
        while (XM_hm_read(&hmLog)) {
            PRINT("Log => ");
            PrintHmLog(&hmLog);
        }
        /*XM_get_partition_status(1, &partStatus);
        if (partStatus.state == HALTED) {
            XM_halt_partition(XM_PARTITION_SELF);
        }*/
        XM_idle_self();
    }
    PRINT("--------- Health Monitor Log ---------------\n");
}

