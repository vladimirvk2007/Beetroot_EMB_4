#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "i2c/i2c.h"

#define I2C_TIMEOUT_MS 100
#define I2C_SPEED_HZ 100000U

#define DS1307_ADDRESS 0x68
#define DS1307_REG_START 0x00
#define DS1307_REG_COUNT 8

static I2C_HandleTypeDef hi2c1;

extern "C" void main_cpp()
{
    HAL_StatusTypeDef status = I2C_Init(&hi2c1, I2C1, I2C_SPEED_HZ);
    if (status != HAL_OK)
    {
        printf("[I2C] I2C1 initialization failed (status=%d)\r\n", (int)status);
        return;
    }

    HAL_Delay(500);

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

    status = I2C_Probe(&hi2c1, DS1307_ADDRESS, I2C_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        printf("[I2C] DS1307 not found (status=%d)\r\n", (int)status);
        return;
    }

    while (1)
    {
        uint8_t regs[DS1307_REG_COUNT];

        status = I2C_ReadRegister(&hi2c1, DS1307_ADDRESS, DS1307_REG_START,
                                  regs, sizeof(regs), I2C_TIMEOUT_MS);
        if (status == HAL_OK)
        {
            printf("[I2C] DS1307:");
            for (uint8_t i = 0; i < sizeof(regs); i++)
            {
                printf(" %02X", regs[i]);
            }
            printf("\r\n");
        }
        else
        {
            printf("[I2C] Register read failed (status=%d)\r\n", (int)status);
        }

        HAL_Delay(1000);
    }
}
