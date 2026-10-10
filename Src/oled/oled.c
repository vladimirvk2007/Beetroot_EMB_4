#include "oled/oled.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OLED_WIDTH 128U
#define OLED_DEFAULT_ADDRESS 0x3CU
#define OLED_I2C_TIMEOUT_MS 100U
#define OLED_PRINTF_MAX_LEN 32U

#define OLED_CONTROL_COMMAND 0x00U
#define OLED_CONTROL_DATA 0x40U
#define OLED_COMMAND_MAX_SIZE 32U

#define OLED_FONT_CELL 8U
#define OLED_BDF_HEADER_SIZE 9U
#define OLED_BDF_FIRST_RECORD 2U

struct OLED_HandleTypeDef
{
  I2C_HandleTypeDef *i2c;
  uint16_t address;
  uint8_t height;
  uint8_t pages;
  bool dirty;           /* є незбережені на дисплей зміни */
  uint8_t dirty_first;  /* діапазон змінених сторінок */
  uint8_t dirty_last;
  uint8_t framebuffer[]; /* буфер кадру: pages * 128 байт */
};

/* Надсилає командний пакет (до 32 байт). */
static HAL_StatusTypeDef OLED_WriteCommands(const OLED_HandleTypeDef *oled,
                                             const uint8_t *commands,
                                             uint8_t size)
{
  uint8_t packet[OLED_COMMAND_MAX_SIZE + 1U];

  if ((size == 0U) || (size > OLED_COMMAND_MAX_SIZE))
  {
    return HAL_ERROR;
  }

  packet[0] = OLED_CONTROL_COMMAND;
  memcpy(&packet[1], commands, size);
  return I2C_Transmit(oled->i2c, oled->address, packet, size + 1U,
                      OLED_I2C_TIMEOUT_MS);
}

/* Розширює діапазон змінених сторінок. */
static void OLED_MarkDirty(OLED_HandleTypeDef *oled,
                           uint8_t first_page,
                           uint8_t last_page)
{
  if (!oled->dirty)
  {
    oled->dirty = true;
    oled->dirty_first = first_page;
    oled->dirty_last = last_page;
    return;
  }

  if (first_page < oled->dirty_first)
  {
    oled->dirty_first = first_page;
  }
  if (last_page > oled->dirty_last)
  {
    oled->dirty_last = last_page;
  }
}

/* Передає на дисплей сторінки, змінені після попередньої передачі. */
static HAL_StatusTypeDef OLED_Flush(OLED_HandleTypeDef *oled)
{
  uint8_t packet[OLED_WIDTH + 1U];

  if (!oled->dirty)
  {
    return HAL_OK;
  }

  packet[0] = OLED_CONTROL_DATA;

  for (uint8_t page = oled->dirty_first; page <= oled->dirty_last; page++)
  {
    const uint8_t commands[] = {
        (uint8_t)(0xB0U | page), /* Адреса сторінки */
        0x00U,                   /* Молодша адреса колонки */
        0x10U                    /* Старша адреса колонки */
    };

    HAL_StatusTypeDef status =
        OLED_WriteCommands(oled, commands, sizeof(commands));
    if (status != HAL_OK)
    {
      return status;
    }

    memcpy(&packet[1], &oled->framebuffer[(uint16_t)page * OLED_WIDTH],
           OLED_WIDTH);
    status = I2C_Transmit(oled->i2c, oled->address, packet, sizeof(packet),
                          OLED_I2C_TIMEOUT_MS);
    if (status != HAL_OK)
    {
      return status;
    }
  }

  oled->dirty = false;
  return HAL_OK;
}

/* Змінює піксель у буфері; поза панеллю ігнорується. */
static void OLED_PutPixel(OLED_HandleTypeDef *oled, int x, int y, bool clear)
{
  if ((x < 0) || (x >= (int)OLED_WIDTH) || (y < 0) || (y >= (int)oled->height))
  {
    return;
  }

  const uint8_t page = (uint8_t)(y / 8);
  const uint8_t mask = (uint8_t)(1U << (y % 8));
  uint8_t *cell = &oled->framebuffer[((uint16_t)page * OLED_WIDTH) + (uint16_t)x];

  if (clear)
  {
    *cell &= (uint8_t)~mask;
  }
  else
  {
    *cell |= mask;
  }

  OLED_MarkDirty(oled, page, page);
}

/* Лінія за алгоритмом Брезенхема. */
static void OLED_PutLine(OLED_HandleTypeDef *oled,
                         int x0, int y0, int x1, int y1, bool clear)
{
  const int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  const int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
  const int sx = (x0 < x1) ? 1 : -1;
  const int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;

  while (1)
  {
    OLED_PutPixel(oled, x0, y0, clear);
    if ((x0 == x1) && (y0 == y1))
    {
      break;
    }

    const int e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

/* Коло (середньоточковий алгоритм); fill = залити. */
static void OLED_PutCircle(OLED_HandleTypeDef *oled,
                           int cx, int cy, int radius, bool fill, bool clear)
{
  int f = 1 - radius;
  int ddf_x = 1;
  int ddf_y = -2 * radius;
  int x = 0;
  int y = radius;

  if (fill)
  {
    OLED_PutLine(oled, cx - radius, cy, cx + radius, cy, clear);
  }
  else
  {
    OLED_PutPixel(oled, cx, cy + radius, clear);
    OLED_PutPixel(oled, cx, cy - radius, clear);
    OLED_PutPixel(oled, cx + radius, cy, clear);
    OLED_PutPixel(oled, cx - radius, cy, clear);
  }

  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddf_y += 2;
      f += ddf_y;
    }
    x++;
    ddf_x += 2;
    f += ddf_x;

    if (fill)
    {
      OLED_PutLine(oled, cx - x, cy + y, cx + x, cy + y, clear);
      OLED_PutLine(oled, cx - x, cy - y, cx + x, cy - y, clear);
      OLED_PutLine(oled, cx - y, cy + x, cx + y, cy + x, clear);
      OLED_PutLine(oled, cx - y, cy - x, cx + y, cy - x, clear);
    }
    else
    {
      OLED_PutPixel(oled, cx + x, cy + y, clear);
      OLED_PutPixel(oled, cx - x, cy + y, clear);
      OLED_PutPixel(oled, cx + x, cy - y, clear);
      OLED_PutPixel(oled, cx - x, cy - y, clear);
      OLED_PutPixel(oled, cx + y, cy + x, clear);
      OLED_PutPixel(oled, cx - y, cy + x, clear);
      OLED_PutPixel(oled, cx + y, cy - x, clear);
      OLED_PutPixel(oled, cx - y, cy - x, clear);
    }
  }
}

/* Прямокутник; fill = залити. */
static void OLED_PutRectangle(OLED_HandleTypeDef *oled,
                              int x, int y, int width, int height,
                              bool fill, bool clear)
{
  const int right = x + width - 1;
  const int bottom = y + height - 1;

  if (fill)
  {
    for (int row = y; row <= bottom; row++)
    {
      OLED_PutLine(oled, x, row, right, row, clear);
    }
    return;
  }

  OLED_PutLine(oled, x, y, right, y, clear);
  OLED_PutLine(oled, x, bottom, right, bottom, clear);
  OLED_PutLine(oled, x, y, x, bottom, clear);
  OLED_PutLine(oled, right, y, right, bottom, clear);
}

HAL_StatusTypeDef OLED_Init(I2C_HandleTypeDef *hi2c,
                            const OLED_SettingsTypeDef *settings,
                            OLED_HandleTypeDef **oled)
{
  if ((hi2c == NULL) || (hi2c->Instance == NULL) || (settings == NULL) ||
      (oled == NULL))
  {
    return HAL_ERROR;
  }

  uint8_t height;
  switch (settings->panel)
  {
  case OLED_PANEL_128X32:
    height = 32U;
    break;
  case OLED_PANEL_128X64:
    height = 64U;
    break;
  case OLED_PANEL_128X128:
    height = 128U;
    break;
  default:
    return HAL_ERROR;
  }

  const uint16_t address =
      (settings->address == 0U) ? OLED_DEFAULT_ADDRESS : settings->address;
  if (address > 0x7FU)
  {
    return HAL_ERROR;
  }

  if ((settings->scl_speed_hz != 0U) &&
      (settings->scl_speed_hz != hi2c->Init.ClockSpeed))
  {
    HAL_StatusTypeDef status =
        I2C_Init(hi2c, hi2c->Instance, settings->scl_speed_hz);
    if (status != HAL_OK)
    {
      return status;
    }
  }

  HAL_StatusTypeDef status = I2C_Probe(hi2c, address, OLED_I2C_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  const uint8_t pages = height / 8U;
  OLED_HandleTypeDef *dev =
      calloc(1U, sizeof(*dev) + ((size_t)pages * OLED_WIDTH));
  if (dev == NULL)
  {
    return HAL_ERROR;
  }

  dev->i2c = hi2c;
  dev->address = address;
  dev->height = height;
  dev->pages = pages;

  const uint8_t commands[] = {
      0xAEU,                          /* Дисплей вимкнено */
      0xA8U, (uint8_t)(height - 1U),  /* Коефіцієнт мультиплексування */
      0xD3U, 0x00U,                   /* Зміщення дисплея */
      0x40U,                          /* Початковий рядок */
      settings->flip ? 0xA0U : 0xA1U, /* Відображення сегментів */
      settings->flip ? 0xC0U : 0xC8U, /* Напрямок сканування COM */
      0xD5U, 0x80U,                   /* Дільник частоти */
      0xDAU, (height == 32U) ? 0x02U : 0x12U, /* Конфігурація виводів COM */
      0x81U, 0xFFU,                   /* Контраст */
      0xD9U, 0xF1U,                   /* Період попереднього заряду */
      0xDBU, 0x40U,                   /* Рівень VCOMH */
      0x20U, 0x02U,                   /* Сторінкова адресація */
      0x8DU, 0x14U,                   /* Помповий перетворювач */
      0x2EU,                          /* Прокрутку вимкнено */
      0xA4U,                          /* Показ вмісту RAM */
      0xA6U,                          /* Нормальний режим */
      0xAFU                           /* Дисплей увімкнено */
  };

  status = OLED_WriteCommands(dev, commands, sizeof(commands));
  if (status == HAL_OK)
  {
    OLED_MarkDirty(dev, 0U, (uint8_t)(pages - 1U));
    status = OLED_Flush(dev);
  }

  if (status != HAL_OK)
  {
    free(dev);
    return status;
  }

  *oled = dev;
  return HAL_OK;
}

HAL_StatusTypeDef OLED_Deinit(OLED_HandleTypeDef *oled)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  free(oled);
  return HAL_OK;
}

HAL_StatusTypeDef OLED_Clear(OLED_HandleTypeDef *oled)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  memset(oled->framebuffer, 0, (size_t)oled->pages * OLED_WIDTH);
  OLED_MarkDirty(oled, 0U, (uint8_t)(oled->pages - 1U));
  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_DrawPixel(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, bool invert)
{
  if ((oled == NULL) || (x >= OLED_WIDTH) || (y >= oled->height))
  {
    return HAL_ERROR;
  }

  OLED_PutPixel(oled, x, y, invert);
  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_DrawLine(OLED_HandleTypeDef *oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool invert)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  OLED_PutLine(oled, x0, y0, x1, y1, invert);
  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_DrawCircle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t radius, bool invert)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  OLED_PutCircle(oled, x, y, radius, false, invert);
  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_FillCircle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t radius, bool invert)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  OLED_PutCircle(oled, x, y, radius, true, invert);
  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_DrawRectangle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  if ((width != 0U) && (height != 0U))
  {
    OLED_PutRectangle(oled, x, y, width, height, false, invert);
  }
  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_FillRectangle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  if ((width != 0U) && (height != 0U))
  {
    OLED_PutRectangle(oled, x, y, width, height, true, invert);
  }
  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_SetContrast(OLED_HandleTypeDef *oled, uint8_t contrast)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  const uint8_t commands[] = {0x81U, contrast};
  return OLED_WriteCommands(oled, commands, sizeof(commands));
}

HAL_StatusTypeDef OLED_Power(OLED_HandleTypeDef *oled, bool on)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  const uint8_t command = on ? 0xAFU : 0xAEU;
  return OLED_WriteCommands(oled, &command, 1U);
}

HAL_StatusTypeDef OLED_Print(OLED_HandleTypeDef *oled, uint8_t row, uint8_t scale, bool invert, const char *text)
{
  if ((oled == NULL) || (text == NULL) || (scale < 1U) || (scale > 3U) ||
      (((uint16_t)row + scale) > oled->pages))
  {
    return HAL_ERROR;
  }

  const size_t max_len = OLED_WIDTH / (OLED_FONT_CELL * scale);
  const size_t len = strnlen(text, max_len + 1U);
  if (len > max_len)
  {
    return HAL_ERROR;
  }

  const int origin_y = (int)row * 8;

  for (size_t i = 0U; i < len; i++)
  {
    const uint8_t *columns = OLED_FontLatin8x8[(uint8_t)text[i]];
    const int origin_x = (int)(i * OLED_FONT_CELL * scale);

    for (uint8_t col = 0U; col < OLED_FONT_CELL; col++)
    {
      for (uint8_t bit = 0U; bit < OLED_FONT_CELL; bit++)
      {
        const bool lit = (((columns[col] >> bit) & 1U) != 0U) != invert;

        for (uint8_t dx = 0U; dx < scale; dx++)
        {
          for (uint8_t dy = 0U; dy < scale; dy++)
          {
            OLED_PutPixel(oled,
                          origin_x + (col * scale) + dx,
                          origin_y + (bit * scale) + dy,
                          !lit);
          }
        }
      }
    }
  }

  return OLED_Flush(oled);
}

HAL_StatusTypeDef OLED_Printf(OLED_HandleTypeDef *oled, uint8_t row, uint8_t scale, const char *fmt, ...)
{
  char buffer[OLED_PRINTF_MAX_LEN + 1U];

  if (fmt == NULL)
  {
    return HAL_ERROR;
  }

  va_list args;
  va_start(args, fmt);
  const int written = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  if (written < 0)
  {
    return HAL_ERROR;
  }

  return OLED_Print(oled, row, scale, false, buffer);
}

/*
 * Формат BDF-шрифту: [ширина, висота] і записи символів
 * [код, крок, bbw, bbh, bbx, bby, розмір даних, y початок, y кінець, дані...];
 * кінець - запис із розміром даних 0. Рядки даних: старший біт зліва.
 */
HAL_StatusTypeDef OLED_PrintBdf(OLED_HandleTypeDef *oled, const uint8_t *font, int x, int y, const char *text)
{
  if ((oled == NULL) || (font == NULL) || (text == NULL))
  {
    return HAL_ERROR;
  }

  for (; *text != '\0'; text++)
  {
    const uint8_t *record = &font[OLED_BDF_FIRST_RECORD];

    while ((record[6] != 0U) && (record[0] != (uint8_t)*text))
    {
      record += OLED_BDF_HEADER_SIZE + record[6];
    }

    if (record[6] == 0U)
    {
      continue;
    }

    const int rows = (int)record[8] - (int)record[7] + 1;
    if (rows > 0)
    {
      const int row_bytes = record[6] / rows;
      const uint8_t *data = &record[OLED_BDF_HEADER_SIZE];

      for (int row = 0; row < rows; row++)
      {
        for (int col = 0; col < (row_bytes * 8); col++)
        {
          if ((data[(row * row_bytes) + (col / 8)] & (0x80U >> (col % 8))) != 0U)
          {
            OLED_PutPixel(oled, x + col, y + record[7] + row, false);
          }
        }
      }
    }

    x += record[1];
  }

  return OLED_Flush(oled);
}
