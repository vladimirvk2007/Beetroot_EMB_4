#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "esp_err.h"

// Ініціалізує Wi-Fi в режимі клієнта (STA)
esp_err_t wifi_init_sta(void);

#endif // WIFI_SETUP_H
