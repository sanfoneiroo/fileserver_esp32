#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiConfig
{
public:

    // inicia sistema WiFi
    static void begin();

    // status conexão
    static bool connected();

    // IP local STA
    static IPAddress localIP();

    // SSID conectado
    static String getSSID();
};

#endif