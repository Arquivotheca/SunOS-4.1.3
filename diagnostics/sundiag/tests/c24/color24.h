/*
 * color24.h:  header file for ibis test (color24.c)
 *
 * @(#) color24.h 1.1 7/30/92 Sun Microsystems Inc
 */

/* #define RED 0x40
 * #define BLUE 0x80
 * #define GREEN 0x20
 * #define WHITE 0xFF
 * #define YELLOW 0xC0
 * #define CYAN 0x60
 * #define MAGENTA 0xA0
 * #define BLACK 0
 * #define ORANGE 0xE0
 * #define BROWN 0x10
 * #define DARKGREEN 0x50
 * #define GRAY 0x90
 * #define LIGHTGRAY 0xD0
 * #define DARKGRAY 0x30
 * #define PURPLE 0x70
 * #define PINK 0xB0
 * #define LIGHTGREEN 0xF0
*/
#define RED 0x0000FF
#define BLUE 0xFF0000
#define GREEN 0x00FF00
#define WHITE 0xFFFFFF
#define YELLOW 0x00FFFF
#define CYAN 0xFFFF00
#define MAGENTA 0xFF00FF
#define BLACK 0
#define ORANGE 0x20B1FF
#define BROWN 0x282BAE
#define DARKGREEN 0x46AE41
#define GRAY 0x808080
#define LIGHTGRAY 0xC0C0C0
#define DARKGRAY 0x404040
#define PURPLE 0xD636D6
#define PINK 0xFBA0FF
#define LIGHTGREEN 0x80FF68

#define COLORMASK 0xFFFFFF  /* change to 0xFFFFFF for ibis */
#ifdef _4_1
#define PIXPG_COLOR PIXPG_24BIT_COLOR
#endif
