#ifndef FLASH_LOGGER_H
#define FLASH_LOGGER_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#include "app_types.h"

struct __attribute__((packed)) FlashLogRecord {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	int16_t temperature_c_x100;
	uint16_t humidity_pct_x100;
	uint16_t pressure_mmhg;
};

struct FlashLoggerStatus {
	bool enabled;
	bool full;
	size_t total_records;
	size_t buffered_records;
	size_t flushed_sectors;
	size_t write_offset;
	size_t partition_size;
};

esp_err_t flash_logger_init(void);
esp_err_t flash_logger_clear(void);
esp_err_t flash_logger_flush(void);
esp_err_t flash_logger_set_enabled(bool enabled);
bool flash_logger_is_enabled(void);
esp_err_t flash_logger_append(const SensorData* sample);
bool flash_logger_is_full(void);
size_t flash_logger_get_record_count(void);
esp_err_t flash_logger_read_record(size_t index, FlashLogRecord* out);
esp_err_t flash_logger_get_status(FlashLoggerStatus* out);

#endif // FLASH_LOGGER_H
