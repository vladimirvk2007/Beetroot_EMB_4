#ifndef LIB_DRV8833_DRV8833_H
#define LIB_DRV8833_DRV8833_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "pwm.h"

// Конфігурація двох PWM-входів для одного двигуна.
typedef struct {
    pwm_config_t in1;
    pwm_config_t in2;
} drv8833_config_t;

// Стан драйвера одного двигуна DRV8833.
typedef struct {
    pwm_t in1_pwm;
    pwm_t in2_pwm;
    int16_t speed_percent;
    bool initialized;
} drv8833_t;

#ifdef __cplusplus
extern "C" {
#endif

// Ініціалізує драйвер і обидва PWM-канали.
esp_err_t drv8833_init(drv8833_t *motor, const drv8833_config_t *config);

// Зупиняє PWM-канали та звільняє ресурси драйвера.
esp_err_t drv8833_deinit(drv8833_t *motor);

// Встановлює швидкість від -100 до 100 відсотків:
// додатне значення — напрямок IN1, від'ємне — IN2, нуль — вибіг (IN1=IN2=0).
esp_err_t drv8833_set_speed(drv8833_t *motor, int16_t speed_percent);

#ifdef __cplusplus
}
#endif

#endif /* LIB_DRV8833_DRV8833_H */
