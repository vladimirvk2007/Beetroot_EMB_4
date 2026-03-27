
# ESP32-S3 (YD-ESP-S3/ESP32-S3) ADC Example (ESP-IDF, PlatformIO)

This project demonstrates the use of the ADC (Analog-to-Digital Converter) on ESP32-S3 with calibration and output in millivolts using ESP-IDF (PlatformIO).

## Main Features
- Reads analog signal from ADC1, channel 3 (GPIO 4)
- Uses calibration (curve fitting) for accurate voltage measurement
- Logs raw ADC value and voltage in mV via ESP_LOGI
- 1 second delay between measurements

### Main Code
- File: `src/main.cpp`
- Channel: ADC1, channel 3 (GPIO 4 by default for ESP32-S3)
- Calibration: scheme_curve_fitting (ESP-IDF)

## PlatformIO
- Platform: `espressif32`
- Board: `esp32-s3-devkitc-1`
- Framework: `espidf`
- Monitor speed: 115200 baud

## How to Build and Flash
1. Connect your ESP32-S3 (YD-ESP-S3/ESP32-S3) board to your computer
2. Open a terminal in the project root
3. Run:
   ```
   pio run -t upload
   ```
4. To view logs, use:
   ```
   pio device monitor
   ```

## Code Highlights
- All critical ESP-IDF calls are wrapped with ESP_ERROR_CHECK for easy diagnostics
- Calibration (adc_cali*) is used for accurate voltage readings
- If calibration is not needed, you can use only the raw adc_raw value

## Configuration
- To change the ADC channel or parameters, edit the relevant structures in `main.cpp`
- To change the measurement frequency, adjust the delay in vTaskDelay

## More Info
- ESP-IDF ADC documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/adc_oneshot.html

