#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

bool wifiConnection_init(const char *ssid, const char *password);
bool wifiConnection_run();

#endif // WIFI_CONNECTION_H
