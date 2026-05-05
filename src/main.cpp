#include <Arduino.h>
#include <Wire.h>
#include <I2C_eeprom.h>

#define AT24C32_ADDR        0x50   // A0=A1=A2=GND
#define I2C_SDA_PIN         8
#define I2C_SCL_PIN         9

#define PAGE_SIZE           32
#define TOTAL_PAGES         (I2C_DEVICESIZE_24LC32 / PAGE_SIZE)
#define CYCLE_DELAY         500
#define WRITE_READ_DELAY    5

#define I2C_SCAN_ADDR_MIN   0x08   // first valid 7-bit address
#define I2C_SCAN_ADDR_MAX   0x77   // last valid 7-bit address

I2C_eeprom eeprom(AT24C32_ADDR, I2C_DEVICESIZE_24LC32);

static void i2c_scan()
{
    Serial.println("\n--- I2C Scan ---");
    uint8_t found = 0;
    for (uint8_t addr = I2C_SCAN_ADDR_MIN; addr <= I2C_SCAN_ADDR_MAX; addr++) {
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

static bool page_write(uint16_t page, const uint8_t *buf)
{
    if (buf == nullptr) {
        Serial.printf("[Page %3u/%u] FAIL: null buffer\n", page, TOTAL_PAGES - 1);
        return false;
    }

    const uint16_t addr = page * PAGE_SIZE;
    if (eeprom.writeBlock(addr, buf, PAGE_SIZE) != 0) {
        Serial.printf("[Page %3u/%u | addr=0x%04X] FAIL: write error\n",
                      page, TOTAL_PAGES - 1, addr);
        return false;
    }
    return true;
}

static bool page_read(uint16_t page, uint8_t *buf)
{
    if (buf == nullptr) {
        Serial.printf("[Page %3u/%u] FAIL: null buffer\n", page, TOTAL_PAGES - 1);
        return false;
    }

    const uint16_t addr = page * PAGE_SIZE;
    if (eeprom.readBlock(addr, buf, PAGE_SIZE) != PAGE_SIZE) {
        Serial.printf("[Page %3u/%u | addr=0x%04X] FAIL: read error\n",
                      page, TOTAL_PAGES - 1, addr);
        return false;
    }
    return true;
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

    Serial.printf("AT24C32 ready. Size: %u bytes, Pages: %u x %u bytes\n",
                  eeprom.getDeviceSize(), TOTAL_PAGES, PAGE_SIZE);
    Serial.printf("Interval: %u ms/page. Starting Write phase...\n\n", CYCLE_DELAY);
}

void loop()
{
    static uint16_t current_page = 0;
    const uint16_t addr = current_page * PAGE_SIZE;
    uint8_t buf[PAGE_SIZE] = {};

    snprintf((char *)buf, PAGE_SIZE, "#%u Page %u", current_page, current_page);

    if (page_write(current_page, buf)) {
        Serial.printf("[Write | Page %3u/%u | addr=0x%04X] \"%s\"\n",
                          current_page, TOTAL_PAGES - 1, addr, (char *)buf);
    }

    delay(WRITE_READ_DELAY);

    if (page_read(current_page, buf)) {
        Serial.printf("[Read  | Page %3u/%u | addr=0x%04X] \"%s\"\n",
                          current_page, TOTAL_PAGES - 1, addr, (char *)buf);
    }

    current_page++;

    if (current_page >= TOTAL_PAGES) {
        current_page = 0;
    }

    delay(CYCLE_DELAY);
}

