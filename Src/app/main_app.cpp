#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "i2c/i2c.h"
#include "oled/oled.h"
#include "ds1307/ds1307.h"

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

    /* Перевірка DS1307 */
    DS1307_HandleTypeDef *rtc = NULL;
    status = DS1307_Init(&hi2c1, &rtc);
    if (status != HAL_OK)
    {
        printf("[RTC] Init failed (status=%d)\r\n", (int)status);
        return;
    }

    /* Встановлюємо тестову дату й час для перевірки RTC */
    const DS1307_DateTimeTypeDef initial_datetime = {
        2026, /* year */
        10,   /* month */
        10,   /* day */
        7,    /* weekday */
        18,   /* hour */
        48,   /* minute */
        20,   /* second */
    };
    status = DS1307_SetDateTime(rtc, &initial_datetime);
    if (status != HAL_OK)
    {
        printf("[RTC] SetDateTime failed (status=%d)\r\n", (int)status);
        return;
    }

    OLED_Clear(oled);
    OLED_Print(oled, 0, 1, false, "DS1307 RTC");

    while (1)
    {
        DS1307_DateTimeTypeDef datetime;
        status = DS1307_GetDateTime(rtc, &datetime);
        if (status == HAL_OK)
        {
            OLED_Printf(oled, 2, 1, "%04u-%02u-%02u",
                        (unsigned)datetime.year,
                        (unsigned)datetime.month,
                        (unsigned)datetime.day);
            OLED_Printf(oled, 4, 2, "%02u:%02u:%02u",
                        (unsigned)datetime.hour,
                        (unsigned)datetime.minute,
                        (unsigned)datetime.second);
        }
        else
        {
            printf("[RTC] GetDateTime failed (status=%d)\r\n", (int)status);
        }
        HAL_Delay(1000);
    }
}
