
# ESP32-S3 OTA Update Example

This project demonstrates how to implement Over-the-Air (OTA) firmware updates for the ESP32-S3 using PlatformIO and the Arduino framework. The onboard RGB LED (WS2812) is used as a simple indicator to show how OTA can be integrated into a real project.

## Features
- **OTA Update**: Update firmware wirelessly using PlatformIO and ArduinoOTA.
- **Wi-Fi Connection**: ESP32-S3 connects to your Wi-Fi network for OTA.
- **RGB LED Demo**: The onboard WS2812 LED blinks with different colors to show the device is running and responsive.

## How to Use

### 1. Configure Wi-Fi
Edit `lib/ota/credentials.h` and set your Wi-Fi credentials:
WIFI_SSID and WIFI_PASSWORD

### 2. Build and Upload (First Time)
Connect your ESP32-S3 via USB and upload the firmware:
```
pio run -t upload
```

### 3. Find Device IP
Open the Serial Monitor:
```
pio device monitor -b 115200
```
After boot, the ESP32-S3 will print its IP address.

### 4. OTA Update
Add your device's IP to `platformio.ini`:
```ini
upload_protocol = espota
upload_port = 192.168.x.x  ; Replace with your ESP32-S3 IP
```
Now you can upload new firmware wirelessly:
```
pio run -t upload
```

## Notes
- The RGB LED (WS2812, pin 48) is used for demonstration only. You can remove or replace this part in your own project.
- OTA will not work if the device is not connected to the same network as your PC.
- Do not use long delays in `loop()` to keep OTA responsive.

## Dependencies
- [Adafruit NeoPixel](https://platformio.org/lib/show/28/Adafruit%20NeoPixel)
- ArduinoOTA (included in ESP32 Arduino core)

## License
Educational use only. Not for commercial applications.

