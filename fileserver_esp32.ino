#include <WiFi.h>
#include <WebServer.h>

#include <LittleFS.h>
#include <FS.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "wifi_config.h"
#include "config.h"

// ======================================================
// SERVER
// ======================================================

WebServer server(80);

// ======================================================
// OLED
// ======================================================

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// ======================================================
// OLED STATUS
// ======================================================

String statusLine1 = "";
String statusLine2 = "";

String logLine1 = "";
String logLine2 = "";

// ======================================================
// OLED RENDER
// ======================================================

void renderOLED()
{
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ------------------------------
  // STATUS FIXO
  // ------------------------------

  display.setCursor(0, 0);
  display.println(statusLine1);

  display.setCursor(0, 16);
  display.println(statusLine2);

  // ------------------------------
  // LOG DINAMICO
  // ------------------------------

  display.setCursor(0, 32);
  display.println(logLine1);

  display.setCursor(0, 48);
  display.println(logLine2);

  display.display();
}

// ======================================================
// STATUS
// ======================================================

void setStatus(
  String line1,
  String line2
)
{
  statusLine1 = line1;
  statusLine2 = line2;

  renderOLED();
}

// ======================================================
// LOG
// ======================================================

void oledLog(String msg)
{
  Serial.println(msg);

  logLine1 = logLine2;
  logLine2 = msg;

  renderOLED();
}

// ======================================================
// UTILS
// ======================================================

String formatBytes(size_t bytes)
{
  if (bytes < 1024)
    return String(bytes) + " B";

  else if (bytes < (1024 * 1024))
    return String(bytes / 1024.0) + " KB";

  else if (bytes < (1024 * 1024 * 1024))
    return String(bytes / 1024.0 / 1024.0) + " MB";

  return String(bytes);
}

String getContentType(String filename)
{
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg")) return "image/jpeg";
  if (filename.endsWith(".gif")) return "image/gif";
  if (filename.endsWith(".ico")) return "image/x-icon";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".txt")) return "text/plain";

  return "application/octet-stream";
}

// ======================================================
// ROOT
// ======================================================

void handleRoot()
{
  handleFileList();
}

// ======================================================
// FILE LIST
// ======================================================

void handleFileList()
{
  String html;

  html += "<html><body>";

  html += "<h2>ESP32 File Server</h2>";

  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='upload'>";
  html += "<input type='submit' value='Upload'>";
  html += "</form>";

  html += "<br>";
  html += "<a href='/format'>FORMAT FS</a>";
  html += "<hr>";

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  bool found = false;

  while (file)
  {
    found = true;

    String path = file.path();
    size_t size = file.size();

    html += "<p>";

    html += path;
    html += " (";
    html += formatBytes(size);
    html += ") ";

    html += "<a href='/download?file=" + path + "'>DOWNLOAD</a> ";

    html += "<a href='/rename?file=" + path + "'>RENAME</a> ";

    html += "<a href='/delete?file=" + path + "'>DELETE</a>";

    html += "</p>";

    file = root.openNextFile();
  }

  if (!found)
  {
    html += "<p>No files found</p>";
  }

  // ==================================================
  // FOOTER
  // ==================================================

  html += "<hr>";

  html += "<small>";

  html += "IP: ";
  html += WiFi.localIP().toString();

  html += "<br>";

  html += "LittleFS Used: ";
  html += formatBytes(LittleFS.usedBytes());

  html += " / ";

  html += formatBytes(LittleFS.totalBytes());

  html += "</small>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ======================================================
// DOWNLOAD
// ======================================================

void handleDownload()
{
  if (!server.hasArg("file"))
  {
    server.send(400, "text/plain", "BAD ARGS");

    oledLog("DOWNLOAD FAIL");

    return;
  }

  String path = server.arg("file");

  if (!path.startsWith("/"))
    path = "/" + path;

  if (!LittleFS.exists(path))
  {
    server.send(404, "text/plain", "NOT FOUND");

    oledLog("FILE NOT FOUND");

    return;
  }

  oledLog("DOWNLOAD:");
  oledLog(path);

  File file = LittleFS.open(path, "r");

  server.sendHeader(
    "Content-Disposition",
    "attachment; filename=" + path.substring(1)
  );

  server.streamFile(
    file,
    getContentType(path)
  );

  file.close();
}

// ======================================================
// DELETE
// ======================================================

void handleDelete()
{
  String path = server.arg("file");

  if (!path.startsWith("/"))
    path = "/" + path;

  if (LittleFS.exists(path))
  {
    LittleFS.remove(path);

    oledLog("DELETE:");
    oledLog(path);
  }

  server.sendHeader("Location", "/files");

  server.send(303);
}

// ======================================================
// FORMAT
// ======================================================

void handleFormat()
{
  oledLog("FORMAT FS");

  LittleFS.format();

  server.sendHeader("Location", "/files");

  server.send(303);
}

// ======================================================
// RENAME
// ======================================================

void handleRename()
{
  String file = server.arg("file");

  if (!file.startsWith("/"))
    file = "/" + file;

  if (!server.hasArg("new"))
  {
    String html;

    html += "<form action='/rename'>";

    html += "<input type='hidden' name='file' value='" + file + "'>";

    html += "<input name='new' placeholder='new name'>";

    html += "<input type='submit'>";

    html += "</form>";

    server.send(200, "text/html", html);

    return;
  }

  String newName = server.arg("new");

  if (!newName.startsWith("/"))
    newName = "/" + newName;

  File oldF = LittleFS.open(file, "r");
  File newF = LittleFS.open(newName, "w");

  while (oldF.available())
  {
    newF.write(oldF.read());
  }

  oldF.close();
  newF.close();

  LittleFS.remove(file);

  oledLog("RENAME:");
  oledLog(file);
  oledLog(newName);

  server.sendHeader("Location", "/files");

  server.send(303);
}

// ======================================================
// UPLOAD
// ======================================================

File uploadFile;

void handleUpload()
{
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;

    if (!filename.startsWith("/"))
      filename = "/" + filename;

    uploadFile = LittleFS.open(
      filename,
      FILE_WRITE
    );

    oledLog("UPLOAD:");
    oledLog(filename);
  }

  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (uploadFile)
    {
      uploadFile.write(
        upload.buf,
        upload.currentSize
      );
    }
  }

  else if (upload.status == UPLOAD_FILE_END)
  {
    if (uploadFile)
    {
      uploadFile.close();
    }

    oledLog("UPLOAD OK");

    server.sendHeader(
      "Location",
      "/upload_done"
    );

    server.send(303);
  }
}

// ======================================================
// UPLOAD DONE
// ======================================================

void handleUploadDone()
{
  String html;

  html += "<html><body>";

  html += "<h2>Upload concluido</h2>";

  html += "<a href='/files'>File Manager</a><br>";

  html += "<a href='/'>Home</a>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ======================================================
// NOT FOUND
// ======================================================

void handleNotFound()
{
  String path = server.uri();

  if (LittleFS.exists(path))
  {
    File file = LittleFS.open(path, "r");

    server.streamFile(
      file,
      getContentType(path)
    );

    file.close();

    return;
  }

  Serial.println("404: " + path);

  server.send(
    404,
    "text/plain",
    "404 NOT FOUND"
  );
}

// ======================================================
// SERVER INIT
// ======================================================

void startFileServer()
{
  server.on("/", handleRoot);

  server.on("/files", handleFileList);

  server.on("/download", handleDownload);

  server.on("/delete", handleDelete);

  server.on("/rename", handleRename);

  server.on("/format", handleFormat);

  server.on(
    "/upload",
    HTTP_POST,
    []() {},
    handleUpload
  );

  server.on(
    "/upload_done",
    handleUploadDone
  );

  server.onNotFound(handleNotFound);

  server.begin();

  setStatus(
  WiFi.localIP().toString(),
  "FS: " +
  formatBytes(LittleFS.usedBytes()) +
  "/" +
  formatBytes(LittleFS.totalBytes())
);

  oledLog("SERVER START");
  oledLog("");
}

// ======================================================
// OLED INIT
// ======================================================

void initOLED()
{
  Wire.begin(
    OLED_SDA_PIN,
    OLED_SCL_PIN
  );

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      ))
  {
    Serial.println("OLED FAIL");

    return;
  }

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setCursor(0, 0);

  display.println("ESP32 FILE SERVER");

  display.display();

  delay(1000);

  oledLog("OLED READY");
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  initOLED();

  oledLog("MOUNT FS");

  if (!LittleFS.begin(true))
  {
    oledLog("FS FAIL");

    return;
  }

  oledLog("FS OK");

  oledLog("WIFI START");

  WiFiConfig::begin();

  oledLog("WIFI OK");

  startFileServer();
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
  server.handleClient();
}