#include "bme280_stub.h"

esp_err_t bme280_read_stub(int16_t* temp_x100, uint16_t* hum_x100, uint16_t* pressure_mmhg) {
    if ((temp_x100 == nullptr) || (hum_x100 == nullptr) || (pressure_mmhg == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Stub values until BME280 full driver integration.
    *temp_x100 = 2534;
    *hum_x100 = 4821;
    *pressure_mmhg = 748;
    return ESP_OK;
}
