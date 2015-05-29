/*
 * $FILE: vga.c
 *
 * Generic code to access the vga
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifdef CONFIG_OUTPUT_ENABLED

#define COLUMNS                 80
/* The number of lines. */
#define LINES                   24
/* The attribute of an character. */
#define ATTRIBUTE               7
/* The video memory address. */
#define VIDEO                   0xB8000

/* Variables. */
/* Save the X position. */
static int xpos;
/* Save the Y position. */
static int ypos;
/* Point to the video memory. */
static volatile unsigned char *video = (unsigned char *)VIDEO;

/* Clear the screen and initialize VIDEO, XPOS and YPOS. */
void InitOutput(void) {
    int i;
    
    for (i = 0; i < COLUMNS * LINES * 2; i++)
	*(video+i) = 0;
  
    xpos = 0;
    ypos = 0;
}

/* Put the character C on the screen. */
void xputchar(int c) {
    if (c == '\n' || c == '\r') {
    newline:
	xpos = 0;
	ypos++;
	if (ypos >= LINES)
	    ypos = 0;
	return;
    }
  
    *(video + (xpos + ypos * COLUMNS) * 2) = c & 0xFF;
    *(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;
  
    xpos++;
    if (xpos >= COLUMNS)
	goto newline;
}

#else

void InitOutput(void) {}
void xputchar(int c) {}

#endif
