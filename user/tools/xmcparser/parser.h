/*
 * $FILE: parser.h
 *
 * XtratuM's XML configuration parser to C
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _PARSER_H_
#define _PARSER_H_

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlschemas.h>

struct attrXml {
    const xmlChar *name;
    void (*handler)(xmlNodePtr, const xmlChar *);
};

struct nodeXml {
    const xmlChar *name;
    void (*handlerBNode)(xmlNodePtr);
    void (*handlerEAttr)(xmlNodePtr);
    void (*handlerENode)(xmlNodePtr);
    struct attrXml **attrList;
    struct nodeXml **children; //[];
};

extern struct nodeXml *rootHandlers[];

#endif
