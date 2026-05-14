// wifi_config.cpp

#include "wifi_config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// =====================================================
// CONFIG
// =====================================================

static const char* AP_SSID = "ESP32_SETUP";
static const char* AP_PASS = "12345678";

// =====================================================
// GLOBAIS
// =====================================================

static Preferences prefs;
static WebServer server(80);

static String sta_ssid;
static String sta_pass;

static bool wifi_connected = false;

// =====================================================
// HTML
// =====================================================

static String htmlPage()
{
    return R"rawliteral(
<!DOCTYPE html>
<html>

<head>

<meta charset="utf-8">

<title>ESP32 WiFi Manager</title>

<style>

body{
    font-family:Arial;
    max-width:400px;
    margin:40px auto;
    padding:20px;
}

input{
    width:100%;
    padding:12px;
    margin-bottom:12px;
    box-sizing:border-box;
}

button{
    width:100%;
    padding:12px;
    font-size:16px;
}

</style>

</head>

<body>

<h2>Configurar WiFi</h2>

<form action="/save" method="POST">

<label>SSID</label>
<input type="text" name="ssid">

<label>Senha</label>
<input type="password" name="pass">

<button type="submit">
Salvar
</button>

</form>

</body>
</html>
)rawliteral";
}

// =====================================================
// STORAGE
// =====================================================

static bool loadCredentials()
{
    prefs.begin("wifi", true);

    sta_ssid = prefs.getString("ssid", "");
    sta_pass = prefs.getString("pass", "");

    prefs.end();

    return sta_ssid.length() > 0;
}

static void saveCredentials(String ssid, String pass)
{
    prefs.begin("wifi", false);

    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);

    prefs.end();
}

// =====================================================
// STA
// =====================================================

static bool connectSTA()
{
    Serial.println();
    Serial.println("Conectando STA...");

    Serial.print("SSID: ");
    Serial.println(sta_ssid);

    WiFi.mode(WIFI_STA);

    WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());

    int retries = 0;

    while (WiFi.status() != WL_CONNECTED && retries < 20)
    {
        delay(500);

        Serial.print(".");

        retries++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        wifi_connected = true;

        Serial.println("WiFi conectado!");

        Serial.print("IP DHCP: ");
        Serial.println(WiFi.localIP());

        return true;
    }

    wifi_connected = false;

    Serial.println("Falha na conexão STA");

    return false;
}

// =====================================================
// AP
// =====================================================

static void startAP()
{
    Serial.println();
    Serial.println("Iniciando AP...");

    WiFi.mode(WIFI_AP);

    WiFi.softAP(AP_SSID, AP_PASS);

    Serial.print("AP SSID: ");
    Serial.println(AP_SSID);

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

// =====================================================
// WEB ROUTES
// =====================================================

static void handleRoot()
{
    server.send(200, "text/html", htmlPage());
}

static void handleSave()
{
    String new_ssid = server.arg("ssid");
    String new_pass = server.arg("pass");

    Serial.println();
    Serial.println("Novas credenciais recebidas");

    Serial.print("SSID: ");
    Serial.println(new_ssid);

    saveCredentials(new_ssid, new_pass);

    server.send(
        200,
        "text/html",
        "<h2>Credenciais salvas!</h2>"
        "<p>Reiniciando ESP32...</p>"
    );

    delay(2000);

    ESP.restart();
}

// =====================================================
// PORTAL
// =====================================================

static void startPortal()
{
    startAP();

    server.on("/", HTTP_GET, handleRoot);

    server.on("/save", HTTP_POST, handleSave);

    server.begin();

    Serial.println("Portal web iniciado");

    // =================================================
    // LOOP BLOQUEANTE
    // =================================================

    while (true)
    {
        server.handleClient();

        delay(1);
    }
}

// =====================================================
// PUBLIC API
// =====================================================

void WiFiConfig::begin()
{
    Serial.println();
    Serial.println("==== WiFiConfig ====");

    bool hasCredentials = loadCredentials();

    // tenta conectar caso exista config salva
    if (hasCredentials)
    {
        if (connectSTA())
        {
            Serial.println("WiFi pronto");

            return;
        }
    }

    // caso falhe -> entra em portal
    Serial.println("Entrando em modo configuração");

    startPortal();
}

bool WiFiConfig::connected()
{
    return wifi_connected;
}

IPAddress WiFiConfig::localIP()
{
    return WiFi.localIP();
}

String WiFiConfig::getSSID()
{
    return sta_ssid;
}