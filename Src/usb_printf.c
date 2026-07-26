#include "main.h"
#include "usbd_cdc_if.h"

int _write(int file, char *ptr, int len)
{
  (void)file;

  int sent = 0;
  while (sent < len)
  {
    uint16_t chunk = (uint16_t)((len - sent) > APP_TX_DATA_SIZE ? APP_TX_DATA_SIZE : (len - sent));
    while (CDC_Transmit_FS((uint8_t *)&ptr[sent], chunk) == USBD_BUSY)
    {
      HAL_Delay(1);
    }
    sent += chunk;
  }

  return len;
}

int __io_putchar(int ch)
{
  char c = (char)ch;
  _write(1, &c, 1);
  return ch;
}