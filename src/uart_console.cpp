#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds1307.h"
#include "esp_log.h"
#include "flash_logger.h"
#include "uart_console.h"

static const char* TAG = "UART";
static constexpr TickType_t STATE_MUTEX_TIMEOUT = pdMS_TO_TICKS(20);
static constexpr size_t MAX_LOG_LINES = 100;

static void print_uart_help() {
    ESP_LOGI(TAG, "UART commands:");
    ESP_LOGI(TAG, "  status");
    ESP_LOGI(TAG, "  set_time YYYY-MM-DD HH:MM:SS");
    ESP_LOGI(TAG, "  log show [N]");
    ESP_LOGI(TAG, "  log clear");
    ESP_LOGI(TAG, "  log on");
    ESP_LOGI(TAG, "  log off");
    ESP_LOGI(TAG, "  log status");
    ESP_LOGI(TAG, "  log flush");
}

static int abs_int(int value) {
    return (value >= 0) ? value : -value;
}

static void uart_log_show_command(const char* args) {
    size_t count = 10;
    if ((args != nullptr) && (*args != '\0')) {
        char* end = nullptr;
        const unsigned long parsed = strtoul(args, &end, 10);
        if ((end == args) || (parsed == 0UL)) {
            ESP_LOGW(TAG, "Usage: log show [N], N=1..100");
            return;
        }
        count = static_cast<size_t>(parsed);
    }

    if (count > MAX_LOG_LINES) {
        count = MAX_LOG_LINES;
    }

    const size_t total = flash_logger_get_record_count();
    if (total == 0) {
        ESP_LOGI(TAG, "LOGS: empty");
        return;
    }

    const size_t start = (total > count) ? (total - count) : 0;
    ESP_LOGI(TAG,
             "LOGS: showing %lu of %lu (full=%d)",
             static_cast<unsigned long>(total - start),
             static_cast<unsigned long>(total),
             flash_logger_is_full() ? 1 : 0);

    for (size_t i = start; i < total; ++i) {
        FlashLogRecord rec = {};
        const esp_err_t err = flash_logger_read_record(i, &rec);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "logs read failed at %lu: %s", static_cast<unsigned long>(i), esp_err_to_name(err));
            return;
        }

        const int t_int = rec.temperature_c_x100 / 100;
        const int t_frac = abs_int(rec.temperature_c_x100 % 100);
        ESP_LOGI(TAG,
                 "[%lu] %04u-%02u-%02u %02u:%02u:%02u t=%d.%02d h=%u.%02u p=%u",
                 static_cast<unsigned long>(i),
                 static_cast<unsigned>(rec.year),
                 static_cast<unsigned>(rec.month),
                 static_cast<unsigned>(rec.day),
                 static_cast<unsigned>(rec.hour),
                 static_cast<unsigned>(rec.minute),
                 static_cast<unsigned>(rec.second),
                 t_int,
                 t_frac,
                 static_cast<unsigned>(rec.humidity_pct_x100 / 100U),
                 static_cast<unsigned>(rec.humidity_pct_x100 % 100U),
                 static_cast<unsigned>(rec.pressure_mmhg));
    }
}

static void uart_log_clear_command() {
    const esp_err_t clear_err = flash_logger_clear();
    if (clear_err != ESP_OK) {
        ESP_LOGW(TAG, "log clear failed: %s", esp_err_to_name(clear_err));
        return;
    }
    ESP_LOGI(TAG, "LOGS: cleared");
}

static void uart_log_status_command() {
    FlashLoggerStatus status = {};
    const esp_err_t err = flash_logger_get_status(&status);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "log status failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG,
             "LOG STATUS: enabled=%d full=%d total=%lu buffered=%lu sectors=%lu offset=%lu size=%lu",
             status.enabled ? 1 : 0,
             status.full ? 1 : 0,
             static_cast<unsigned long>(status.total_records),
             static_cast<unsigned long>(status.buffered_records),
             static_cast<unsigned long>(status.flushed_sectors),
             static_cast<unsigned long>(status.write_offset),
             static_cast<unsigned long>(status.partition_size));
}

static void uart_log_enable_command(bool enabled) {
    const esp_err_t err = flash_logger_set_enabled(enabled);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "log %s failed: %s", enabled ? "on" : "off", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "LOGGING: %s", enabled ? "ON" : "OFF");
}

static void uart_log_flush_command() {
    const esp_err_t err = flash_logger_flush();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "log flush failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "LOG FLUSH: OK");
}

static void uart_status_command(UartConsoleContext* ctx) {
    SensorData sample = {};
    bool has_sample = false;
    bool display_ready = false;
    bool bme_ready = false;
    esp_err_t rtc_err = ESP_OK;

    if (xSemaphoreTake(ctx->state_mutex, STATE_MUTEX_TIMEOUT) != pdTRUE) {
        ESP_LOGW(TAG, "STATUS failed: state mutex timeout");
        return;
    }
    sample = *ctx->last_sample;
    has_sample = *ctx->has_sample;
    display_ready = *ctx->display_ready;
    bme_ready = *ctx->bme_ready;
    rtc_err = *ctx->last_rtc_err;
    xSemaphoreGive(ctx->state_mutex);

    ESP_LOGI(TAG,
             "STATUS: display=%d bme=%d last_rtc=%s has_sample=%d",
             display_ready ? 1 : 0,
             bme_ready ? 1 : 0,
             esp_err_to_name(rtc_err),
             has_sample ? 1 : 0);

    if (has_sample) {
        const int temp_c_x10 = (sample.temperature_c_x100 >= 0)
                                   ? ((sample.temperature_c_x100 + 5) / 10)
                                   : ((sample.temperature_c_x100 - 5) / 10);
        const int temp_frac = (temp_c_x10 % 10 >= 0) ? (temp_c_x10 % 10) : -(temp_c_x10 % 10);
        const unsigned hum_pct_x10 = static_cast<unsigned>((sample.humidity_pct_x100 + 5U) / 10U);

        ESP_LOGI(TAG,
                 "STATUS SAMPLE: seq=%lu rtc=%d bme=%d time=%04u-%02u-%02u %02u:%02u:%02u t=%d.%d h=%u.%u p=%u",
                 static_cast<unsigned long>(sample.seq),
                 sample.rtc_ok ? 1 : 0,
                 sample.bme_ok ? 1 : 0,
                 static_cast<unsigned>(sample.rtc.year),
                 static_cast<unsigned>(sample.rtc.month),
                 static_cast<unsigned>(sample.rtc.day),
                 static_cast<unsigned>(sample.rtc.hour),
                 static_cast<unsigned>(sample.rtc.minute),
                 static_cast<unsigned>(sample.rtc.second),
                 temp_c_x10 / 10,
                 temp_frac,
                 hum_pct_x10 / 10,
                 hum_pct_x10 % 10,
                 static_cast<unsigned>(sample.pressure_mmhg));
    }
}

static void uart_set_time_command(UartConsoleContext* ctx, const char* args) {
    if (args == nullptr) {
        ESP_LOGW(TAG, "Usage: set_time YYYY-MM-DD HH:MM:SS");
        return;
    }

    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;

    if (sscanf(args, "%u-%u-%u %u:%u:%u", &year, &month, &day, &hour, &minute, &second) != 6) {
        ESP_LOGW(TAG, "Usage: set_time YYYY-MM-DD HH:MM:SS");
        return;
    }

    const RtcDateTime dt = {
        .year = static_cast<uint16_t>(year),
        .month = static_cast<uint8_t>(month),
        .day = static_cast<uint8_t>(day),
        .hour = static_cast<uint8_t>(hour),
        .minute = static_cast<uint8_t>(minute),
        .second = static_cast<uint8_t>(second),
    };

    const esp_err_t err = ds1307_set_time(ctx->i2c_port, ctx->ds1307_addr, ctx->i2c_mutex, &dt);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "set_time failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG,
             "Time updated: %04u-%02u-%02u %02u:%02u:%02u",
             year,
             month,
             day,
             hour,
             minute,
             second);
}

void uart_console_task(void* pvParameters) {
    auto* ctx = static_cast<UartConsoleContext*>(pvParameters);
    if ((ctx == nullptr) ||
        (ctx->i2c_mutex == nullptr) ||
        (ctx->last_sample == nullptr) ||
        (ctx->has_sample == nullptr) ||
        (ctx->display_ready == nullptr) ||
        (ctx->bme_ready == nullptr) ||
        (ctx->last_rtc_err == nullptr) ||
        (ctx->state_mutex == nullptr)) {
        ESP_LOGE(TAG, "UART context is invalid");
        vTaskDelete(nullptr);
        return;
    }

    char line[96] = {0};
    size_t len = 0;

    print_uart_help();

    while (true) {
        const int ch = getchar();
        if (ch < 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if ((ch == '\n') || (ch == '\r')) {
            line[len] = '\0';

            char* cmd = line;
            while ((*cmd == ' ') || (*cmd == '\t')) {
                ++cmd;
            }

            if (*cmd != '\0') {
                if (strcmp(cmd, "status") == 0) {
                    uart_status_command(ctx);
                } else if (strcmp(cmd, "log on") == 0) {
                    uart_log_enable_command(true);
                } else if (strcmp(cmd, "log off") == 0) {
                    uart_log_enable_command(false);
                } else if (strcmp(cmd, "log status") == 0) {
                    uart_log_status_command();
                } else if (strcmp(cmd, "log flush") == 0) {
                    uart_log_flush_command();
                } else if (strcmp(cmd, "log clear") == 0) {
                    uart_log_clear_command();
                } else if (strncmp(cmd, "set_time ", 9) == 0) {
                    uart_set_time_command(ctx, cmd + 9);
                } else if ((strncmp(cmd, "log show", 8) == 0) &&
                           ((cmd[8] == '\0') || (cmd[8] == ' ') || (cmd[8] == '\t'))) {
                    const char* args = cmd + 8;
                    while ((*args == ' ') || (*args == '\t')) {
                        ++args;
                    }
                    uart_log_show_command(args);
                } else {
                    ESP_LOGW(TAG, "Unknown command: %s", cmd);
                    print_uart_help();
                }
            }

            len = 0;
            continue;
        }

        if (len + 1 < sizeof(line)) {
            line[len++] = static_cast<char>(ch);
        }
    }
}
