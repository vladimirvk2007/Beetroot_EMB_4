#include <Arduino.h>
#include <Wire.h>
#include <I2C_eeprom.h>

#define AT24C32_ADDR    0x50   // A0=A1=A2=GND
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

#define PAGE_SIZE       32     // AT24C32 page size in bytes
#define CYCLE_INTERVAL  500    // ms between pages

I2C_eeprom eeprom(AT24C32_ADDR, I2C_DEVICESIZE_24LC32);

static bool     s_eeprom_ready = false;
static uint16_t s_total_pages  = 0;
static uint16_t s_current_page = 0;
static uint16_t s_pass_count   = 0;
static uint16_t s_fail_count   = 0;

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

// --- Page write/read/verify ---

static void page_run()
{
    const uint16_t addr = s_current_page * PAGE_SIZE;

    // Build label: "#<n> Page <n>", padded with 0x00 to fill PAGE_SIZE
    uint8_t wbuf[PAGE_SIZE] = {};
    snprintf((char *)wbuf, PAGE_SIZE, "#%u Page %u", s_current_page, s_current_page);

    if (eeprom.writeBlock(addr, wbuf, PAGE_SIZE) != 0) {
        Serial.printf("[Page %3u/%u | addr=0x%04X] FAIL: write error\n",
                      s_current_page, s_total_pages - 1, addr);
        s_fail_count++;
    } else {
        uint8_t rbuf[PAGE_SIZE] = {};
        if (eeprom.readBlock(addr, rbuf, PAGE_SIZE) != PAGE_SIZE) {
            Serial.printf("[Page %3u/%u | addr=0x%04X] FAIL: read error\n",
                          s_current_page, s_total_pages - 1, addr);
            s_fail_count++;
        } else {
            bool ok = true;
            for (uint8_t i = 0; i < PAGE_SIZE; i++) {
                if (rbuf[i] != wbuf[i]) {
                    Serial.printf("[Page %3u/%u | addr=0x%04X] FAIL: byte[%u] wrote 0x%02X read 0x%02X\n",
                                  s_current_page, s_total_pages - 1, addr,
                                  i, wbuf[i], rbuf[i]);
                    ok = false;
                    break;
                }
            }
            if (ok) {
                Serial.printf("[Page %3u/%u | addr=0x%04X] OK: \"%s\"\n",
                              s_current_page, s_total_pages - 1, addr, (char *)rbuf);
                s_pass_count++;
            } else {
                Serial.printf("[Page %3u/%u | addr=0x%04X] FAIL: expected \"%s\", got \"%s\"\n",
                              s_current_page, s_total_pages - 1, addr,
                              (char *)wbuf, (char *)rbuf);
                s_fail_count++;
            }
        }
    }

    s_current_page++;

    if (s_current_page >= s_total_pages) {
        Serial.printf("\n=== Full scan complete: %u/%u pages passed ===\n\n",
                      s_pass_count, s_total_pages);
        // Reset for next round
        s_current_page = 0;
        s_pass_count   = 0;
        s_fail_count   = 0;
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

    s_total_pages = eeprom.getDeviceSize() / PAGE_SIZE;
    Serial.printf("AT24C32 ready. Size: %u bytes, Pages: %u x %u bytes\n",
                  eeprom.getDeviceSize(), s_total_pages, PAGE_SIZE);
    Serial.printf("Writing all pages, one per %u ms...\n\n", CYCLE_INTERVAL);

    s_eeprom_ready = true;
}

void loop()
{
    if (!s_eeprom_ready) return;

    static uint32_t s_last_ms = 0;
    uint32_t now = millis();

    if (now - s_last_ms >= CYCLE_INTERVAL) {
        s_last_ms = now;
        page_run();
    }
}

