#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "i2c/i2c.h"
#include "oled/oled.h"

#define I2C_TIMEOUT_MS 100
#define I2C_SPEED_HZ 100000U

static I2C_HandleTypeDef hi2c1;

extern "C" void main_cpp()
{
    HAL_StatusTypeDef status = I2C_Init(&hi2c1, I2C1, I2C_SPEED_HZ);
    if (status != HAL_OK)
    {
        printf("[I2C] I2C1 initialization failed (status=%d)\r\n", (int)status);
        return;
    }

    /* Затримка, щоб встигнути підключити термінал до USB CDC */
    HAL_Delay(5000);

    /* Сканування шини I2C */
    uint8_t found[16];
    uint8_t found_count = 0;

    status = I2C_Scan(&hi2c1, found, sizeof(found), &found_count, 10);
    if (status != HAL_OK)
    {
        printf("[I2C] Scan failed (status=%d)\r\n", (int)status);
        return;
    }

    printf("[I2C] Scan: %u device(s) found\r\n", (unsigned)found_count);
    for (uint8_t i = 0; (i < found_count) && (i < sizeof(found)); i++)
    {
        printf("[I2C] Found device at 0x%02X\r\n", found[i]);
    }

    /* Ініціалізація дисплея 128x64 з адресою 0x3C */
    OLED_SettingsTypeDef oled_settings = OLED_SETTINGS_128X64_DEFAULT();
    OLED_HandleTypeDef *oled = NULL;

    status = OLED_Init(&hi2c1, &oled_settings, &oled);
    if (status != HAL_OK)
    {
        printf("[OLED] Initialization failed (status=%d)\r\n", (int)status);
        return;
    }

    /* Заголовок та прості фігури */
    OLED_Print(oled, 0, 1, true, "SSD1306 OLED");
    OLED_DrawRectangle(oled, 0, 16, 40, 24, false);
    OLED_FillCircle(oled, 64, 28, 10, false);
    OLED_DrawLine(oled, 88, 16, 127, 39, false);

    printf("[OLED] SSD1306 ready\r\n");

    /* Лічильник секунд в 6-му рядку сторінок */
    for (uint32_t tick = 0;; tick++)
    {
        OLED_Printf(oled, 6, 1, "Tick: %lu", (unsigned long)tick);
        HAL_Delay(1000);
    }
}
