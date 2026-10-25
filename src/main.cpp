#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "wifi_setup.h"
#include "mqtt.h"
#include "pwm.h"
#include "servo.h"

static const char *TAG = "main";

#define LED_GPIO             GPIO_NUM_16
#define SERVO_GPIO           GPIO_NUM_17
#define PUBLISH_INTERVAL_MS  (10 * 1000)

static pwm_t s_servo_pwm = {};
static servo_t s_servo = {};
static bool s_led_on = false;

static bool parse_servo_angle(const char *text, uint16_t *angle)
{
    if (text == NULL || angle == NULL || *text == '\0') {
        return false;
    }

    unsigned int value = 0;
    for (const char *digit = text; *digit != '\0'; ++digit) {
        if (*digit < '0' || *digit > '9') {
            return false;
        }

        value = value * 10 + (unsigned int)(*digit - '0');
        if (value > SERVO_MAX_ANGLE) {
            return false;
        }
    }

    *angle = (uint16_t)value;
    return true;
}

static void handle_mqtt_message(const char *topic, const char *data)
{
    // Ігноруємо некоректні дані та повідомлення, що не адресовані командному топіку.
    if (topic == NULL || data == NULL) {
        return;
    }

    if (strcmp(topic, MQTT_COMMANDS) != 0) {
        return;
    }

    esp_mqtt_client_handle_t client = mqtt_get_client();

    if (strcmp(data, "ON") == 0) {
        ESP_LOGI(TAG, "Command: LED ON");
        esp_err_t err = gpio_set_level(LED_GPIO, 1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED ON");
        } else {
            s_led_on = true;
        }
    } else if (strcmp(data, "OFF") == 0) {
        ESP_LOGI(TAG, "Command: LED OFF");
        esp_err_t err = gpio_set_level(LED_GPIO, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED OFF");
        } else {
            s_led_on = false;
        }
    } else if (strcmp(data, "STATUS") == 0) {
        // Відповідь надсилається в окремий топік статусу.
        if (client) {
            char status[40];
            snprintf(status, sizeof(status), "LED=%s;SERVO=%u",
                     s_led_on ? "ON" : "OFF", (unsigned int)s_servo.angle);
            int msg_id = esp_mqtt_client_publish(client, MQTT_STATUS, status, 0, 0, 0);
            if (msg_id < 0) {
                ESP_LOGE(TAG, "Failed to publish status (error=%d)", msg_id);
            } else {
                ESP_LOGI(TAG, "Status sent: %s", status);
            }
        } else {
            ESP_LOGE(TAG, "Cannot send status: MQTT client is not initialized");
        }
    } else if (strncmp(data, "SERVO:", 6) == 0) {
        uint16_t angle = 0;
        if (!parse_servo_angle(data + 6, &angle)) {
            ESP_LOGW(TAG, "Invalid servo command: %s (expected SERVO:0..180)", data);
            return;
        }

        esp_err_t err = servo_set_angle(&s_servo, angle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set servo angle to %u: %s (0x%x)",
                     (unsigned int)angle, esp_err_to_name(err), (unsigned int)err);
        } else {
            ESP_LOGI(TAG, "Servo moved to %u degrees", (unsigned int)angle);
        }
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", data);
    }
}

extern "C" void app_main(void)
{
    uint32_t count = 0;
    char msg[64];

    // NVS потрібен для драйвера Wi-Fi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Сконфігурувати LED GPIO
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask  = (1ULL << LED_GPIO);
    io_conf.mode          = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en    = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type     = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, 0));
    s_led_on = false;

    // Серво використовує окремий LEDC-канал і таймер із частотою 50 Гц.
    pwm_config_t servo_pwm_config = {};
    servo_pwm_config.gpio = SERVO_GPIO;
    servo_pwm_config.channel = LEDC_CHANNEL_0;
    servo_pwm_config.timer = LEDC_TIMER_0;
    servo_pwm_config.frequency_hz = SERVO_FREQUENCY_HZ;
    servo_pwm_config.resolution = LEDC_TIMER_14_BIT;
    servo_pwm_config.duty = 0;
    servo_pwm_config.inverted = false;

    ret = pwm_init(&s_servo_pwm, &servo_pwm_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo PWM initialization failed: %s (0x%x)",
                 esp_err_to_name(ret), (unsigned int)ret);
        return;
    }

    ret = servo_init(&s_servo, &s_servo_pwm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo initialization failed: %s (0x%x)",
                 esp_err_to_name(ret), (unsigned int)ret);
        esp_err_t cleanup_err = pwm_deinit(&s_servo_pwm);
        if (cleanup_err != ESP_OK) {
            ESP_LOGE(TAG, "Servo PWM cleanup failed: %s (0x%x)",
                     esp_err_to_name(cleanup_err), (unsigned int)cleanup_err);
        }
        return;
    }

    ret = wifi_init_sta();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization/connection failed: %s (0x%x)",
                 esp_err_to_name(ret), (unsigned int)ret);
        return;
    }

    // Реєструємо callback до запуску клієнта, щоб не пропустити вхідні команди.
    mqtt_set_message_handler(handle_mqtt_message);
    ret = mqtt_app_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT initialization failed: %s (0x%x)",
                 esp_err_to_name(ret), (unsigned int)ret);
        return;
    }

    // Публікуємо періодичний heartbeat після успішного запуску мережевих модулів.
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));
        count++;
        snprintf(msg, sizeof(msg), "Hello from ESP32-S3! #%" PRIu32, count);

        esp_mqtt_client_handle_t client = mqtt_get_client();
        if (client == NULL || !mqtt_is_connected()) {
            ESP_LOGW(TAG, "MQTT is not connected, skipping publish: %s", msg);
        } else {
            int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, msg, 0, 0, 0);
            if (msg_id < 0) {
                ESP_LOGE(TAG, "Failed to publish message (error=%d)", msg_id);
            } else {
                ESP_LOGI(TAG, "Published: %s", msg);
            }
        }
    }
}
