#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <FS.h>

#include "wifi_config.h"

WebServer server(80);

// ================= UTILS =================

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

// ================= ROOT ====================

void handleRoot()
{
  if (LittleFS.exists("/index.html"))
  {
    File file = LittleFS.open("/index.html", "r");

    server.streamFile(file, "text/html");

    file.close();

    return;
  }

  String html;

  html += "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 File Server</title>";
  html += "</head><body>";

  html += "<h1>ESP32 File Server (LittleFS)</h1>";

  html += "<h3>System Info</h3>";

  html += "Chip: " + String(ESP.getChipModel()) + "<br>";
  html += "CPU: " + String(ESP.getCpuFreqMHz()) + " MHz<br>";
  html += "Flash: " + formatBytes(ESP.getFlashChipSize()) + "<br>";
  html += "Heap Free: " + formatBytes(ESP.getFreeHeap()) + "<br>";

  html += "LittleFS Used: " +
          formatBytes(LittleFS.usedBytes()) +
          " / " +
          formatBytes(LittleFS.totalBytes()) +
          "<br><br>";

  html += "WiFi SSID: " + WiFiConfig::getSSID() + "<br>";
  html += "IP: " + WiFiConfig::localIP().toString() + "<br><br>";

  html += "<a href='/files'>File Manager</a>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ================= FILE LIST =================

void handleFileList()
{
  String html;

  html += "<html><body>";
  html += "<h2>File Manager</h2>";

  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='upload'>";
  html += "<input type='submit' value='Upload'>";
  html += "</form>";

  html += "<br><a href='/format'>FORMAT FS</a><hr>";

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  bool found = false;

  while (file)
  {
    found = true;

    String path = file.path();
    size_t size = file.size();

    html += "<p>";

    html += path + " (" + formatBytes(size) + ") ";

    html += "<a href='/download?file=" + path + "'>DOWNLOAD</a> ";
    html += "<a href='/rename?file=" + path + "'>RENAME</a> ";
    html += "<a href='/delete?file=" + path + "'>DELETE</a>";

    html += "</p>";

    file = root.openNextFile();
  }

  if (!found)
    html += "<p>No files found</p>";

  html += "<br><a href='/'>Back</a>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ================= DOWNLOAD =================

void handleDownload()
{
  if (!server.hasArg("file"))
  {
    server.send(400, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("file");

  if (!path.startsWith("/"))
    path = "/" + path;

  if (!LittleFS.exists(path))
  {
    server.send(404, "text/plain", "NOT FOUND");
    return;
  }

  File file = LittleFS.open(path, "r");

  server.sendHeader(
    "Content-Disposition",
    "attachment; filename=" + path.substring(1)
  );

  server.streamFile(file, getContentType(path));

  file.close();
}

// ================= DELETE =================

void handleDelete()
{
  String path = server.arg("file");

  if (!path.startsWith("/"))
    path = "/" + path;

  if (LittleFS.exists(path))
    LittleFS.remove(path);

  server.sendHeader("Location", "/files");

  server.send(303);
}

// ================= FORMAT =================

void handleFormat()
{
  LittleFS.format();

  server.sendHeader("Location", "/files");

  server.send(303);
}

// ================= RENAME =================

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
    newF.write(oldF.read());

  oldF.close();
  newF.close();

  LittleFS.remove(file);

  server.sendHeader("Location", "/files");

  server.send(303);
}

// ================= UPLOAD =================

File uploadFile;

void handleUpload()
{
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;

    if (!filename.startsWith("/"))
      filename = "/" + filename;

    uploadFile = LittleFS.open(filename, FILE_WRITE);
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (uploadFile)
      uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (uploadFile)
      uploadFile.close();

    server.sendHeader("Location", "/upload_done");

    server.send(303);
  }
}

// ================= UPLOAD DONE =================

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

// ================= NOT FOUND =================

void handleNotFound()
{
  String path = server.uri();

  if (LittleFS.exists(path))
  {
    File file = LittleFS.open(path, "r");

    server.streamFile(file, getContentType(path));

    file.close();

    return;
  }

  server.send(404, "text/plain", "404 NOT FOUND");
}

// ================= FILE SERVER INIT =================

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

  server.on("/upload_done", handleUploadDone);

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("File Server iniciado");

  Serial.print("Acesse: http://");

  Serial.println(WiFi.localIP());
}

// ================= SETUP =================

void setup()
{
  Serial.begin(115200);

  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS mount failed");

    return;
  }

  // conecta STA ou entra em portal AP
  WiFiConfig::begin();

  // só chega aqui se STA conectou
  startFileServer();
}

// ================= LOOP =================

void loop()
{
  server.handleClient();
}