/*
 * STC15W408AS - DIY LED Clock
 */
#include <stdint.h>
#include "stc15.h"

#define DIGITS_MASK     (0x0FU << 2)  /* P3.2..P3.5 */
#define DIGIT_DS1_MASK  (1U << 2)     /* P3.2 */
#define SEGMENT_A       (1U << 0)     /* P2.0 */
#define BLINK_DELAY_MS  500U

/* Програмна затримка, ~1 мс при 11.0592 МГц */
static void delay_ms(uint16_t ms)
{
    uint16_t i;
    while (ms--)
    {
        for (i = 0; i < 1100U; i++)
        {
            __asm__("nop");
        }
    }
}

void main(void)
{
    P3 |= DIGITS_MASK;
    P3 &= (uint8_t)~DIGIT_DS1_MASK;

    while (1)
    {
        P2 = (uint8_t)~SEGMENT_A;
        delay_ms(BLINK_DELAY_MS);

        P2 = 0xFF;
        delay_ms(BLINK_DELAY_MS);
    }
}


