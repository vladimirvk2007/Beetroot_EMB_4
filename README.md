
# ESP32-S3 (YD-ESP-S3/ESP32-S3) ADC Example (ESP-IDF, PlatformIO)


This project demonstrates:
- Using the ADC (Analog-to-Digital Converter) on ESP32-S3 with calibration and output in millivolts
- Generating PWM (LEDC) on GPIO18 and controlling its duty cycle in real time by ADC value
- Modern ESP-IDF (PlatformIO) code style and compatibility


## Main Features
- Reads analog signal from ADC1, channel 3 (GPIO 4)
- Uses calibration (curve fitting) for accurate voltage measurement
- Logs raw ADC value and voltage in mV via ESP_LOGI
- Generates PWM on GPIO18 (5 kHz, 12-bit resolution)
- PWM duty cycle is set in real time according to ADC value (0...4095)
- 100 ms delay between measurements and PWM updates


### Main Code
- File: `src/main.cpp`
- ADC: ADC1, channel 3 (GPIO 4 by default for ESP32-S3)
- PWM: GPIO18, 12-bit, 5 kHz, LEDC (low speed mode)
- Calibration: scheme_curve_fitting (ESP-IDF)


## PlatformIO
- Platform: `espressif32`
- Board: `esp32-s3-devkitc-1` (or compatible ESP32-S3)
- Framework: `espidf`
- Monitor speed: 115200 baud


## How to Build and Flash
1. Connect your ESP32-S3 board to your computer
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
- PWM (LEDC) is configured for 12-bit resolution, 5 kHz, and output on GPIO18
- PWM duty cycle is updated every 100 ms according to ADC value
- If calibration is not needed, you can use only the raw adc_raw value


## Configuration
- To change the ADC channel or parameters, edit the relevant structures in `main.cpp`
- To change the PWM output pin, change `PWM_GPIO` in `main.cpp`
- To change the measurement or PWM update frequency, adjust the delay in vTaskDelay


## More Info
- ESP-IDF ADC documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/adc_oneshot.html
- ESP-IDF LEDC (PWM) documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/ledc.html

