#include "common.h"
#include "arm/cpu.h"

static const u8 bug_font[];

static int
bug_draw_str(u8 *fb, int x, int y, const char *str)
{
    u8 *px;
    int mask, row, xpos = x;

    while(1) {
        int c = *str++;

        if (c == 0)
            break;

        // lowercase -> uppercase conv
        if (c >= 'a' && c <= 'z')
            c &= ~0x20;;

        // unknown char, print a blob instead
        if (c < 0x20 || c > 0x5F)
            c = 0x61;

        if (xpos >= 400) {
            xpos = x;
            y += 8;
        }

        c -= 0x20;

        for (int y_ = 0; y_ < 8; y_++) {
            px = fb + (((xpos) * 240 + (239 - (y + y_))) * 3);
            row = bug_font[y_ + (c * 8)];
            mask = 0x80;

            for (int x_ = 0; x_ < 8; x_++, mask >>= 1) {
                *px = (mask & row) ? 0xFF : 0;
                *(px + 1) = (mask & row) ? 0xFF : 0;
                *(px + 2) = (mask & row) ? 0xFF : 0;
                px += 240*3;
            }
        }
        xpos += 8;
    }

    return y;
}


static const char bug_draw_int_lut[] = "0123456789ABCDEF";
static void
bug_draw_int(u8 *fb, int x, int y, u32 h)
{
    char hexstr[9];
    hexstr[8] = 0;
    int i = 8;
    do {
        hexstr[--i] = bug_draw_int_lut[h & 15];
        h >>= 4;
    } while(i != 0);
    bug_draw_str(fb, x, y, hexstr);
}

void
handle_fatal_error(int src, u32 *regs)
{
    u8 *fb = (u8*)0x18000000;
    int y;

    while(1) {
        for (int i = 0; i < 400*240*3; i++)
            fb[i] = 0;

        bug_draw_str(fb, 8, 8, "BUG!");
        y = 16;

        bug_draw_int(fb, 8, y, src);
        y += 8;

        for (int i = 0; i < 17; i++) {
            bug_draw_int(fb, 8, y, regs[i]);
            y += 8;
        }
    }
}

static const u8 bug_font[] = {
/* 20 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*   */
/* 21 */  0x18, 0x3c, 0x3c, 0x18, 0x18, 0x00, 0x18, 0x00, /* ! */
/* 22 */  0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, /* " */
/* 23 */  0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00, /* # */
/* 24 */  0x0C, 0x3F, 0x68, 0x3E, 0x0B, 0x7E, 0x18, 0x00, /* $ */
/* 25 */  0x60, 0x66, 0x0C, 0x18, 0x30, 0x66, 0x06, 0x00, /* % */
/* 26 */  0x38, 0x6C, 0x6C, 0x38, 0x6D, 0x66, 0x3B, 0x00, /* & */
/* 27 */  0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' */
/* 28 */  0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00, /* ( */
/* 29 */  0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00, /* ) */
/* 2A */  0x00, 0x18, 0x7E, 0x3C, 0x7E, 0x18, 0x00, 0x00, /* * */
/* 2B */  0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, /* + */
/* 2C */  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, /* , */
/* 2D */  0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, /* - */
/* 2E */  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, /* . */
/* 2F */  0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00, 0x00, /* / */
/* 30 */  0x3C, 0x66, 0x6E, 0x7E, 0x76, 0x66, 0x3C, 0x00, /* 0 */
/* 31 */  0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00, /* 1 */
/* 32 */  0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00, /* 2 */
/* 33 */  0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00, /* 3 */
/* 34 */  0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00, /* 4 */
/* 35 */  0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00, /* 5 */
/* 36 */  0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00, /* 6 */
/* 37 */  0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00, /* 7 */
/* 38 */  0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00, /* 8 */
/* 39 */  0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00, /* 9 */
/* 3A */  0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, /* : */
/* 3B */  0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30, /* ; */
/* 3C */  0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00, /* < */
/* 3D */  0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00, /* = */ 
/* 3E */  0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00, /* > */
/* 3F */  0x3C, 0x66, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x00, /* ? */
/* 40 */  0x3C, 0x66, 0x6E, 0x6A, 0x6E, 0x60, 0x3C, 0x00, /* @ */
/* 41 */  0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, /* A */
/* 42 */  0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00, /* B */
/* 43 */  0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00, /* C */
/* 44 */  0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00, /* D */
/* 45 */  0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00, /* E */
/* 46 */  0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00, /* F */
/* 47 */  0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00, /* G */
/* 48 */  0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, /* H */
/* 49 */  0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00, /* I */
/* 4A */  0x3E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00, /* J */
/* 4B */  0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00, /* K */
/* 4C */  0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00, /* L */
/* 4D */  0x63, 0x77, 0x7F, 0x6B, 0x6B, 0x63, 0x63, 0x00, /* M */
/* 4E */  0x66, 0x66, 0x76, 0x7E, 0x6E, 0x66, 0x66, 0x00, /* N */
/* 4F */  0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, /* O */
/* 50 */  0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00, /* P */
/* 51 */  0x3C, 0x66, 0x66, 0x66, 0x6A, 0x6C, 0x36, 0x00, /* Q */
/* 52 */  0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00, /* R */
/* 53 */  0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00, /* S */
/* 54 */  0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, /* T */
/* 55 */  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, /* U */
/* 56 */  0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00, /* V */
/* 57 */  0x63, 0x63, 0x6B, 0x6B, 0x7F, 0x77, 0x63, 0x00, /* W */
/* 58 */  0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00, /* X */
/* 59 */  0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00, /* Y */
/* 5A */  0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00, /* Z */
/* 5B */  0x7C, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7C, 0x00, /* [ */
/* 5C */  0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00, 0x00, /* \ */
/* 5D */  0x3E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x3E, 0x00, /* ] */
/* 5E */  0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ^ */
/* 5F */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, /* _ */
/* 60 */  0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ` */
/* 61 */  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* idk */
};
