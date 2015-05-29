/*
 * $FILE: main.c
 *
 * xmproject written with XML2 libs
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "parser.h"
#include "xmc.h"

#define TOOL_NAME "xmproject"
#define USAGE "usage: "TOOL_NAME" [-d] [-s <xsd_file>] [-o output_folder] <XM_CF.xml>\n"

char *inFile;

void LineError(int nLine, char *fmt, ...) {
    va_list args;
    
    fflush(stdout);
    fprintf(stderr, "%s:%d: ", inFile, nLine);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    exit(2);
}

void EPrintF(char *fmt, ...) {
    va_list args;
    
    fflush(stdout);
    if(TOOL_NAME != NULL)
        fprintf(stderr, "%s: ", TOOL_NAME);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
        fprintf(stderr, " %s", strerror(errno));
    fprintf(stderr, "\n");
    exit(2); /* conventional value for failed execution */
}

extern const char sXsd[];

static void ProcessXmlTree(xmlNodePtr root, struct nodeXml *handlers[]) {
    xmlNodePtr node;
    int e, attr;

    for (node=root; node; node=node->next) {
	if (node->type==XML_ELEMENT_NODE) {
	    for (e=0; handlers[e]; e++) {
		if (!xmlStrcasecmp(node->name, handlers[e]->name)) {
		    if (handlers[e]->handlerBNode)
			(handlers[e]->handlerBNode)(node);
		    for (attr=0; handlers[e]->attrList&&handlers[e]->attrList[attr]; attr++) {
			if (xmlHasProp(node, handlers[e]->attrList[attr]->name)) {	    
			    if (handlers[e]->attrList[attr]->handler)
				(handlers[e]->attrList[attr]->handler)(node, xmlGetProp(node, handlers[e]->attrList[attr]->name));
			}
		    }
		    if (handlers[e]->handlerEAttr)
			(handlers[e]->handlerEAttr)(node);
		    ProcessXmlTree(node->children, handlers[e]->children);

		    if (handlers[e]->handlerENode)
			(handlers[e]->handlerENode)(node);
		}
	    }
	}
    }
}

static void ParseXmlFile(const char *xmlFile, const char *xsdFile) {
    xmlSchemaValidCtxtPtr validSchema;
    xmlSchemaParserCtxtPtr xsdParser;
    xmlSchemaPtr schema;
    xmlNodePtr cur;
    xmlDocPtr doc;

    if (xsdFile) {
	fprintf(stderr, "XSD overrided by \"%s\"\n", xsdFile);
	if (!(xsdParser=xmlSchemaNewParserCtxt(xsdFile)))
	    EPrintF("Invalid XSD definition");
    } else {
	if (!(xsdParser=xmlSchemaNewMemParserCtxt(sXsd, strlen(sXsd))))
	    EPrintF("Invalid XSD definition");
    }
    if (!(schema=xmlSchemaParse(xsdParser)))
	EPrintF("Invalid XSD definition");
    validSchema=xmlSchemaNewValidCtxt(schema);

    if (!(doc=xmlParseFile(xmlFile)))
	EPrintF("XML file \"%s\" not found", xmlFile);
    
    if (xmlSchemaValidateDoc(validSchema, doc))
	EPrintF("XML file \"%s\" invalid", xmlFile);
    
    cur=xmlDocGetRootElement(doc);
    ProcessXmlTree(cur, rootHandlers);
    
    xmlSchemaFreeValidCtxt(validSchema);
    xmlSchemaFreeParserCtxt(xsdParser);
    xmlSchemaFree(schema);
    
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

int main(int argc, char **argv)  {
    extern void GenerateProject(char *inFile, char *folder);
    char *folder, defFolder[]="./";
    char *xsdFile=NULL;
    FILE *outFile=stderr;
    int opt;

    LIBXML_TEST_VERSION;

    folder=defFolder;

    while ((opt=getopt(argc, argv, "do:s:"))!=-1) {
        switch (opt) {
	case 'd':
	    fprintf(stderr, "%s\n", sXsd);
	    exit(0);
	    break;
	case 'o':
	    folder=optarg;
            break;
	case 's':
	    DO_MALLOC(xsdFile, strlen(optarg)+1);
	    strcpy(xsdFile, optarg);
	    break;
        default: /* ? */
            fprintf(stderr, USAGE);
	    exit(0);
        }
    }
    
    if (optind>=argc) {
        fprintf(stderr, USAGE);
	exit(0);
    }
   
    DO_MALLOC(inFile, strlen(argv[optind])+1);
    strcpy(inFile, argv[optind]);
    ParseXmlFile(inFile, xsdFile);
    GenerateProject(inFile, folder);
    //GenerateCFile(outFile);
    
    return 0;
}
