#ifndef OLED_FONT_H
#define OLED_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 8x8 Latin font (U+0000..U+00FF); each byte is a column, bit 0 is the top pixel. */
extern const uint8_t OLED_FontLatin8x8[256][8];

/* BDF font 21x26 for OLED_PrintBdf. */
extern const uint8_t OLED_BdfFontNenr12_21x26[];

#ifdef __cplusplus
}
#endif

#endif /* OLED_FONT_H */
