

```bash
# Build project
pio run

# Upload firmware to ESP32
pio run -t upload

# Monitor serial output at 115200 baud
pio device monitor -b 115200

# Combine upload and monitor
pio run -t upload && pio device monitor -b 115200
```

