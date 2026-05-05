#include <Arduino.h>
#include <Wire.h>
#include <I2C_eeprom.h>

#define AT24C32_ADDR    0x50   // A0=A1=A2=GND
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

#define CYCLE_ADDR      0x0100  // address used for cyclic write/read
#define CYCLE_INTERVAL  2000    // ms between cycles

I2C_eeprom eeprom(AT24C32_ADDR, I2C_DEVICESIZE_24LC32);

static bool s_eeprom_ready = false;
static uint32_t s_cycle    = 0;

// --- I2C scan ---

static void i2c_scan()
{
    Serial.println("\n--- I2C Scan ---");
    uint8_t found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Device at 0x%02X", addr);
            if (addr == AT24C32_ADDR) Serial.print("  <-- AT24C32");
            Serial.println();
            found++;
        }
    }
    if (found == 0) {
        Serial.println("  No devices found");
    } else {
        Serial.printf("  Total: %u device(s)\n", found);
    }
    Serial.println("----------------");
}

// --- Cyclic write/read ---

static void cycle_run()
{
    s_cycle++;
    const uint32_t val = s_cycle;

    // Write 4-byte counter value
    uint8_t wbuf[4] = {
        (uint8_t)(val >> 24),
        (uint8_t)(val >> 16),
        (uint8_t)(val >> 8),
        (uint8_t)(val)
    };

    if (eeprom.writeBlock(CYCLE_ADDR, wbuf, sizeof(wbuf)) != 0) {
        Serial.printf("[Cycle %lu] FAIL: write error\n", s_cycle);
        return;
    }

    uint8_t rbuf[4] = {};
    if (eeprom.readBlock(CYCLE_ADDR, rbuf, sizeof(rbuf)) != sizeof(rbuf)) {
        Serial.printf("[Cycle %lu] FAIL: read error\n", s_cycle);
        return;
    }

    uint32_t readback = ((uint32_t)rbuf[0] << 24) | ((uint32_t)rbuf[1] << 16)
                      | ((uint32_t)rbuf[2] << 8)  |  (uint32_t)rbuf[3];

    if (readback != val) {
        Serial.printf("[Cycle %lu] FAIL: wrote %lu, read %lu\n", s_cycle, val, readback);
    } else {
        Serial.printf("[Cycle %lu] OK: counter=%lu at addr=0x%04X\n",
                      s_cycle, readback, CYCLE_ADDR);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== AT24C32 EEPROM Test ===");

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);

    i2c_scan();

    eeprom.begin();

    if (!eeprom.isConnected()) {
        Serial.printf("ERROR: AT24C32 not found at 0x%02X. Check wiring.\n", AT24C32_ADDR);
        return;
    }
    Serial.printf("AT24C32 ready. Size: %u bytes\n", eeprom.getDeviceSize());
    Serial.printf("Starting cyclic write/read at addr=0x%04X, interval=%u ms\n\n",
                  CYCLE_ADDR, CYCLE_INTERVAL);

    s_eeprom_ready = true;
}

void loop()
{
    if (!s_eeprom_ready) return;

    static uint32_t s_last_ms = 0;
    uint32_t now = millis();

    if (now - s_last_ms >= CYCLE_INTERVAL) {
        s_last_ms = now;
        cycle_run();
    }
}

