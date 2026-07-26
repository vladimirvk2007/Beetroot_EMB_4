#include "printf/usb_printf.h"
#include "main.h"

/* Retarget the low-level write syscall to USB CDC so printf() goes over Type-C. */
int _write(int file, char *ptr, int len)
{
  (void)file;

  int sent = 0;
  uint16_t chunk = 0;
  uint32_t start_tick = 0;

  while (sent < len)
  {
    if (len - sent > APP_TX_DATA_SIZE)
    {
      chunk = APP_TX_DATA_SIZE;
    }
    else
    {
      chunk = (uint16_t)(len - sent);
    }

    start_tick = HAL_GetTick();
    while (CDC_Transmit_FS((uint8_t *)&ptr[sent], chunk) == USBD_BUSY)
    {
      if ((HAL_GetTick() - start_tick) > USB_PRINTF_TX_TIMEOUT_MS)
      {
        return len;
      }

      HAL_Delay(1);
    }

    sent += chunk;
  }

  return len;
}

/* Fallback character output used by some libc configurations for printf(). */
int __io_putchar(int ch)
{
    char c = (char)ch;
    _write(1, &c, 1);

    return ch;
}
