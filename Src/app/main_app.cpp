#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "i2c/i2c.h"
#include "at24c32/at24c32.h"

#define I2C_TIMEOUT_MS 100
#define I2C_SPEED_HZ 100000U

#include <stdlib.h>
#include <string.h>

/* Сторінка EEPROM (32 байти), у якій зберігається рядок з номером */
#define EEPROM_START_ADDRESS 0x0000U
#define EEPROM_PAGE_SIZE 32U
#define EEPROM_PAGE_COUNT 128U /* 4096 / 32 */
#define EEPROM_TEXT_PREFIX "Counter: "
#define LOOP_DELAY_MS 500U

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

    /* EEPROM AT24C32 з A2..A0 = 0 (адреса 0x50) */
    AT24C32_HandleTypeDef eeprom;
    status = AT24C32_Init(&eeprom, &hi2c1, 0);
    if (status != HAL_OK)
    {
        printf("[EEPROM] AT24C32 not found (status=%d)\r\n", (int)status);
        return;
    }
    printf("[EEPROM] AT24C32 connected\r\n");

    uint32_t page_index = 0;

    while (1)
    {
        /* Адреса сторінки змінюється в кожному циклі по колу */
        uint32_t address = EEPROM_START_ADDRESS + (page_index % EEPROM_PAGE_COUNT) * EEPROM_PAGE_SIZE;
        page_index++;

        /* Читання сторінки */
        char page[EEPROM_PAGE_SIZE + 1] = {0};
        status = AT24C32_Read(&eeprom, address, (uint8_t *)page, EEPROM_PAGE_SIZE);
        if (status != HAL_OK)
        {
            printf("[EEPROM] Read 0x%04lX failed (status=%d)\r\n", (unsigned long)address, (int)status);
            HAL_Delay(LOOP_DELAY_MS);
            continue;
        }

        /* Витягуємо номер з рядка; порожня/чужа сторінка - починаємо з 0 */
        uint32_t number = 0;
        const char *text = strstr(page, EEPROM_TEXT_PREFIX);
        if (text != NULL)
        {
            number = strtoul(text + strlen(EEPROM_TEXT_PREFIX), NULL, 10);
            printf("[EEPROM] Read 0x%04lX: \"%s\"\r\n", (unsigned long)address, page);
        }
        else
        {
            printf("[EEPROM] Page 0x%04lX has no valid text, starting from 0\r\n", (unsigned long)address);
        }

        /* Інкремент і запис назад з адресою в рядку (решта сторінки - нулі) */
        number++;
        uint8_t out[EEPROM_PAGE_SIZE] = {0};
        snprintf((char *)out, sizeof(out), "Addr 0x%04lX " EEPROM_TEXT_PREFIX "%lu",
                 (unsigned long)address, (unsigned long)number);

        status = AT24C32_Write(&eeprom, address, out, sizeof(out));
        if (status == HAL_OK)
        {
            printf("[EEPROM] Written: \"%s\"\r\n", (const char *)out);
        }
        else
        {
            printf("[EEPROM] Write failed (status=%d)\r\n", (int)status);
        }

        HAL_Delay(LOOP_DELAY_MS);
    }
}
