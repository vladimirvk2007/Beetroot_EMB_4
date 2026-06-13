#include "flash_logger.h"

#include <string.h>

#include "esp_log.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char* TAG = "FLOG";

static constexpr size_t FLASH_SECTOR_SIZE = 4096;
static constexpr size_t RECORDS_PER_SECTOR = FLASH_SECTOR_SIZE / sizeof(FlashLogRecord);

static_assert(sizeof(FlashLogRecord) == 13, "FlashLogRecord size must stay packed");
static constexpr size_t LOG_RECORD_SIZE = sizeof(FlashLogRecord);

static const esp_partition_t* gPartition = nullptr;
static bool gLoggingStopped = false;
static bool gLoggerEnabled = false;
static uint8_t gRamBuffer[FLASH_SECTOR_SIZE] = {0};
static size_t gBufferUsed = 0;
static size_t gWriteOffset = 0;
static SemaphoreHandle_t gLogMutex = nullptr;

static bool lock_logger() {
    return (gLogMutex != nullptr) && (xSemaphoreTake(gLogMutex, pdMS_TO_TICKS(50)) == pdTRUE);
}

static void unlock_logger() {
    xSemaphoreGive(gLogMutex);
}

static void calculate_counters(size_t* flushed_sectors, size_t* flushed_records, size_t* buffered_records, size_t* total) {
    const size_t sectors = gWriteOffset / FLASH_SECTOR_SIZE;
    const size_t records_flushed = sectors * RECORDS_PER_SECTOR;
    const size_t records_buffered = gBufferUsed / LOG_RECORD_SIZE;
    if (flushed_sectors != nullptr) {
        *flushed_sectors = sectors;
    }
    if (flushed_records != nullptr) {
        *flushed_records = records_flushed;
    }
    if (buffered_records != nullptr) {
        *buffered_records = records_buffered;
    }
    if (total != nullptr) {
        *total = records_flushed + records_buffered;
    }
}

static void reset_runtime_state() {
    memset(gRamBuffer, 0xFF, sizeof(gRamBuffer));
    gBufferUsed = 0;
    gWriteOffset = 0;
    gLoggingStopped = false;
}

static esp_err_t flush_full_buffer() {
    if (gBufferUsed != FLASH_SECTOR_SIZE) {
        return ESP_ERR_INVALID_STATE;
    }
    if (gPartition == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((gWriteOffset + FLASH_SECTOR_SIZE) > gPartition->size) {
        gLoggingStopped = true;
        return ESP_ERR_NO_MEM;
    }

    const esp_err_t err = esp_partition_write(gPartition, gWriteOffset, gRamBuffer, FLASH_SECTOR_SIZE);
    if (err != ESP_OK) {
        return err;
    }

    gWriteOffset += FLASH_SECTOR_SIZE;
    gBufferUsed = 0;
    return ESP_OK;
}

static esp_err_t flush_when_no_more_record_space() {
    if ((FLASH_SECTOR_SIZE - gBufferUsed) >= LOG_RECORD_SIZE) {
        return ESP_OK;
    }

    memset(&gRamBuffer[gBufferUsed], 0xFF, FLASH_SECTOR_SIZE - gBufferUsed);
    gBufferUsed = FLASH_SECTOR_SIZE;
    return flush_full_buffer();
}

static esp_err_t flush_partial_buffer_locked() {
    if (gBufferUsed == 0) {
        return ESP_OK;
    }
    if (gPartition == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((gWriteOffset + FLASH_SECTOR_SIZE) > gPartition->size) {
        gLoggingStopped = true;
        return ESP_ERR_NO_MEM;
    }

    memset(&gRamBuffer[gBufferUsed], 0xFF, FLASH_SECTOR_SIZE - gBufferUsed);
    gBufferUsed = FLASH_SECTOR_SIZE;
    return flush_full_buffer();
}

esp_err_t flash_logger_init(void) {
    if (gLogMutex == nullptr) {
        gLogMutex = xSemaphoreCreateMutex();
        if (gLogMutex == nullptr) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (!lock_logger()) {
        return ESP_ERR_TIMEOUT;
    }

    gPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "logdata");
    if (gPartition == nullptr) {
        ESP_LOGE(TAG, "Partition 'logdata' not found");
        unlock_logger();
        return ESP_ERR_NOT_FOUND;
    }

    if ((gPartition->size == 0U) || ((gPartition->size % FLASH_SECTOR_SIZE) != 0U)) {
        ESP_LOGE(TAG, "Partition 'logdata' size must be multiple of 4096");
        unlock_logger();
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG,
             "logdata partition: offset=0x%08lx size=%lu",
             static_cast<unsigned long>(gPartition->address),
             static_cast<unsigned long>(gPartition->size));

    const esp_err_t erase_err = esp_partition_erase_range(gPartition, 0, gPartition->size);
    if (erase_err != ESP_OK) {
        ESP_LOGE(TAG, "Erase logdata failed: %s", esp_err_to_name(erase_err));
        unlock_logger();
        return erase_err;
    }

    memset(gRamBuffer, 0xFF, sizeof(gRamBuffer));
    gBufferUsed = 0;
    gWriteOffset = 0;
    gLoggingStopped = false;
    gLoggerEnabled = true; // Initialize logger enabled state
    unlock_logger();
    return ESP_OK;
}

esp_err_t flash_logger_clear(void) {
    if (!lock_logger()) {
        return ESP_ERR_TIMEOUT;
    }
    if (gPartition == nullptr) {
        unlock_logger();
        return ESP_ERR_INVALID_STATE;
    }

    const esp_err_t erase_err = esp_partition_erase_range(gPartition, 0, gPartition->size);
    if (erase_err != ESP_OK) {
        unlock_logger();
        return erase_err;
    }

    reset_runtime_state();
    unlock_logger();
    return ESP_OK;
}

esp_err_t flash_logger_flush(void) {
    if (!lock_logger()) {
        return ESP_ERR_TIMEOUT;
    }
    if (gPartition == nullptr) {
        unlock_logger();
        return ESP_ERR_INVALID_STATE;
    }

    const esp_err_t err = flush_partial_buffer_locked();
    unlock_logger();
    return err;
}

esp_err_t flash_logger_set_enabled(bool enabled) {
    if (!lock_logger()) {
        return ESP_ERR_TIMEOUT;
    }
    if (gPartition == nullptr) {
        unlock_logger();
        return ESP_ERR_INVALID_STATE;
    }

    gLoggerEnabled = enabled;
    unlock_logger();
    return ESP_OK;
}

bool flash_logger_is_enabled(void) {
    if (!lock_logger()) {
        return false;
    }
    const bool enabled = gLoggerEnabled;
    unlock_logger();
    return enabled;
}

esp_err_t flash_logger_append(const SensorData* sample) {
    if (sample == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_logger()) {
        return ESP_ERR_TIMEOUT;
    }

    if (gPartition == nullptr) {
        unlock_logger();
        return ESP_ERR_INVALID_STATE;
    }
    if (!gLoggerEnabled) {
        unlock_logger();
        return ESP_ERR_INVALID_STATE;
    }
    if (gLoggingStopped) {
        unlock_logger();
        return ESP_ERR_NO_MEM;
    }

    FlashLogRecord rec = {
        .year = sample->rtc.year,
        .month = sample->rtc.month,
        .day = sample->rtc.day,
        .hour = sample->rtc.hour,
        .minute = sample->rtc.minute,
        .second = sample->rtc.second,
        .temperature_c_x100 = sample->temperature_c_x100,
        .humidity_pct_x100 = sample->humidity_pct_x100,
        .pressure_mmhg = sample->pressure_mmhg,
    };

    esp_err_t err = flush_when_no_more_record_space();
    if (err != ESP_OK) {
        unlock_logger();
        return err;
    }

    memcpy(&gRamBuffer[gBufferUsed], &rec, LOG_RECORD_SIZE);
    gBufferUsed += LOG_RECORD_SIZE;

    err = flush_when_no_more_record_space();
    unlock_logger();
    return err;
}

bool flash_logger_is_full(void) {
    if (!lock_logger()) {
        return false;
    }
    const bool full = gLoggingStopped;
    unlock_logger();
    return full;
}

size_t flash_logger_get_record_count(void) {
    if (!lock_logger()) {
        return 0;
    }

    size_t total = 0;
    calculate_counters(nullptr, nullptr, nullptr, &total);

    unlock_logger();
    return total;
}

esp_err_t flash_logger_read_record(size_t index, FlashLogRecord* out) {
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!lock_logger()) {
        return ESP_ERR_TIMEOUT;
    }
    if (gPartition == nullptr) {
        unlock_logger();
        return ESP_ERR_INVALID_STATE;
    }

    size_t flushed_records = 0;
    size_t total = 0;
    calculate_counters(nullptr, &flushed_records, nullptr, &total);

    if (index >= total) {
        unlock_logger();
        return ESP_ERR_NOT_FOUND;
    }

    if (index < flushed_records) {
        const size_t sector = index / RECORDS_PER_SECTOR;
        const size_t in_sector = index % RECORDS_PER_SECTOR;
        const size_t offset = (sector * FLASH_SECTOR_SIZE) + (in_sector * LOG_RECORD_SIZE);
        const esp_err_t err = esp_partition_read(gPartition, offset, out, sizeof(*out));
        unlock_logger();
        return err;
    }

    const size_t ram_index = index - flushed_records;
    const size_t ram_offset = ram_index * LOG_RECORD_SIZE;
    memcpy(out, &gRamBuffer[ram_offset], sizeof(*out));
    unlock_logger();
    return ESP_OK;
}

esp_err_t flash_logger_get_status(FlashLoggerStatus* out) {
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!lock_logger()) {
        return ESP_ERR_TIMEOUT;
    }
    if (gPartition == nullptr) {
        unlock_logger();
        return ESP_ERR_INVALID_STATE;
    }

    size_t sectors = 0;
    size_t buffered = 0;
    size_t total = 0;
    calculate_counters(&sectors, nullptr, &buffered, &total);

    out->enabled = gLoggerEnabled;
    out->full = gLoggingStopped;
    out->total_records = total;
    out->buffered_records = buffered;
    out->flushed_sectors = sectors;
    out->write_offset = gWriteOffset;
    out->partition_size = gPartition->size;

    unlock_logger();
    return ESP_OK;
}
