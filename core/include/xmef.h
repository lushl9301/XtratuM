/*
 * $FILE: xmef.h
 *
 * XM's executable format
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_XMEF_H_
#define _XM_XMEF_H_
/* <track id="build-version-nr"> */
#define XM_SET_VERSION(_ver, _subver, _rev) ((((_ver)&0xFF)<<16)|(((_subver)&0xFF)<<8)|((_rev)&0xFF))
#define XM_GET_VERSION(_v) (((_v)>>16)&0xFF)
#define XM_GET_SUBVERSION(_v) (((_v)>>8)&0xFF)
#define XM_GET_REVISION(_v) ((_v)&0xFF)
/* </track id="build-version-nr"> */

#ifndef __ASSEMBLY__

#include __XM_INCFLD(linkage.h)

//@ \void{<track id="xefCustomFile">}
struct xefCustomFile {
    xmAddress_t sAddr;
    xmSize_t size;
} __PACKED;
//@ \void{</track id="xefCustomFile">}

#define __XMHDR __attribute__((section(".xmHdr")))
#define __XMIHDR __attribute__((section(".xmImageHdr")))

/* <track id="xmHdr"> */
struct xmHdr {
#define XMEF_XM_MAGIC 0x24584d68 // $XMh
    xm_u32_t sSignature;
    xm_u32_t compilationXmAbiVersion; // XM's abi version
    xm_u32_t compilationXmApiVersion; // XM's api version
    xm_u32_t noCustomFiles;
    struct xefCustomFile customFileTab[CONFIG_MAX_NO_CUSTOMFILES];
    xm_u32_t eSignature;
} __PACKED;
/* </track id="xmHdr"> */

/* <track id="imageHeader"> */
struct xmImageHdr {
#define XMEF_PARTITION_MAGIC 0x24584d69 // $XMi
    xm_u32_t sSignature;
    xm_u32_t compilationXmAbiVersion; // XM's abi version
    xm_u32_t compilationXmApiVersion; // XM's api version
/* pageTable is unused when MPU is set */
    xmAddress_t pageTable; // Physical address
/* pageTableSize is unused when MPU is set */
    xmSize_t pageTableSize;
    xm_u32_t noCustomFiles;
    struct xefCustomFile customFileTab[CONFIG_MAX_NO_CUSTOMFILES];
    xm_u32_t eSignature;
} __PACKED;
/* </track id="imageHeader"> */

/*  <track id="xmefFile"> */
struct xmefFile {
    xmAddress_t offset;
    xmSize_t size;
    xmAddress_t nameOffset;
} __PACKED;
/*  </track id="xmefFile"> */

/*  <track id="xmefPartition"> */
struct xmefPartition {
    xm_s32_t id;
    xm_s32_t file;
    xm_u32_t noCustomFiles;
    xm_s32_t customFileTab[CONFIG_MAX_NO_CUSTOMFILES];
} __PACKED;
/*  </track id="xmefPartition"> */

#define XM_DIGEST_BYTES 16
#define XM_PAYLOAD_BYTES 16

/*  <track id="xmefContainerHdr"> */
struct xmefContainerHdr {
    xm_u32_t signature;
#define XM_PACKAGE_SIGNATURE 0x24584354 // $XCT
    xm_u32_t version;
#define XMPACK_VERSION 3
#define XMPACK_SUBVERSION 0
#define XMPACK_REVISION 0
    xm_u32_t flags;
#define XMEF_CONTAINER_DIGEST 0x1
    xm_u8_t digest[XM_DIGEST_BYTES];
    xm_u32_t fileSize;
    xmAddress_t partitionTabOffset;
    xm_s32_t noPartitions;
    xmAddress_t fileTabOffset;
    xm_s32_t noFiles;
    xmAddress_t strTabOffset;
    xm_s32_t strLen;
    xmAddress_t fileDataOffset;
    xmSize_t fileDataLen;    
} __PACKED;
/*  </track id="xmefContainerHdr"> */

#define XEF_VERSION 1
#define XEF_SUBVERSION 1
#define XEF_REVISION 0

/* <track id="xefHdr"> */
struct xefHdr {
#define XEF_SIGNATURE 0x24584546
    xm_u32_t signature;
    xm_u32_t version;
#define XEF_DIGEST 0x1
#define XEF_COMPRESSED 0x4
#define XEF_RELOCATABLE 0x10

#define XEF_TYPE_MASK 0xc0
#define XEF_TYPE_HYPERVISOR 0x00
#define XEF_TYPE_PARTITION 0x40
#define XEF_TYPE_CUSTOMFILE 0x80

#define XEF_ARCH_SPARCv8 0x400
#define XEF_ARCH_MASK 0xff00
    xm_u32_t flags;
    xm_u8_t digest[XM_DIGEST_BYTES];
    xm_u8_t payload[XM_PAYLOAD_BYTES];
    xmSize_t fileSize;
    xmAddress_t segmentTabOffset;
    xm_s32_t noSegments;
    xmAddress_t customFileTabOffset;
    xm_s32_t noCustomFiles;
    xmAddress_t imageOffset;
    xmSize_t imageLength;
    xmSize_t deflatedImageLength;
    xmAddress_t pageTable;
    xmSize_t pageTableSize;
    xmAddress_t xmImageHdr;
    xmAddress_t entryPoint;
} __PACKED;
/* </track id="xefHdr"> */
/* <track id="xefSegment"> */
struct xefSegment {
    xmAddress_t physAddr;
    xmAddress_t virtAddr;
    xmSize_t fileSize;
    xmSize_t deflatedFileSize;
    xmAddress_t offset;
} __PACKED;
/* </track id="xefSegment"> */

#endif

#endif
