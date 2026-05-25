#include <WiFi.h>
#include <WebServer.h>

#include <LittleFS.h>
#include <FS.h>

#include <SPI.h>
#include <SD.h>

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
// SD STATUS
// ======================================================

bool sdMounted = false;

// ======================================================
// OLED LOG
// ======================================================

String oledLogLine = "";

// ======================================================
// UTILS
// ======================================================

String formatBytes(uint64_t bytes)
{
  if (bytes < 1024)
  {
    return String((uint32_t)bytes) + " B";
  }

  double kb = bytes / 1024.0;

  if (kb < 1024)
  {
    return String(kb, 2) + " KB";
  }

  double mb = kb / 1024.0;

  if (mb < 1024)
  {
    return String(mb, 2) + " MB";
  }

  double gb = mb / 1024.0;

  if (gb < 1024)
  {
    return String(gb, 2) + " GB";
  }

  double tb = gb / 1024.0;

  return String(tb, 2) + " TB";
}

String shortText(
  String text,
  int maxLen
)
{
  if (text.length() <= maxLen)
  {
    return text;
  }

  return text.substring(0, maxLen - 3) + "...";
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
// OLED
// ======================================================

void renderOLED()
{
  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(
    SSD1306_WHITE
  );

  // ==================================================
  // IP
  // ==================================================

  display.setCursor(0, 0);

  if (WiFi.status() == WL_CONNECTED)
  {
    display.println(
      WiFi.localIP().toString()
    );
  }
  else
  {
    display.println("NO WIFI");
  }

  // ==================================================
  // LITTLEFS
  // ==================================================

  display.setCursor(0, 16);

  display.print("FS ");

  display.print(
    formatBytes(
      LittleFS.usedBytes()
    )
  );

  display.print("/");

  display.println(
    formatBytes(
      LittleFS.totalBytes()
    )
  );

  // ==================================================
  // SD
  // ==================================================

  display.setCursor(0, 32);

  if (sdMounted)
  {
    uint64_t total =
      SD.cardSize();

    uint64_t used =
      SD.usedBytes();

    display.print("SD ");

    display.print(
      formatBytes(used)
    );

    display.print("/");

    display.println(
      formatBytes(total)
    );
  }
  else
  {
    display.println("SD FAIL");
  }

  // ==================================================
  // LOG
  // ==================================================

  display.setCursor(0, 48);

  display.println(oledLogLine);

  display.display();
}

void oledLog(String msg)
{
  Serial.println(msg);

  oledLogLine = shortText(
    msg,
    20
  );

  renderOLED();
}

// ======================================================
// FS HELPERS
// ======================================================

FS* getFS()
{
  if (
    server.hasArg("fs") &&
    server.arg("fs") == "sd"
  )
  {
    return &SD;
  }

  return &LittleFS;
}

String getFSId(FS* fs)
{
  if (fs == &SD)
  {
    return "sd";
  }

  return "lfs";
}

// ======================================================
// ROOT
// ======================================================

void handleRoot()
{
  String html;

  html += "<html><body>";

  html += "<h1>ESP32 FILE SERVER</h1>";

  html += "<p>";
  html += WiFi.localIP().toString();
  html += "</p>";

  // ==================================================
  // LITTLEFS
  // ==================================================

  renderFileSection(
    html,
    LittleFS,
    "LittleFS",
    "lfs"
  );

  // ==================================================
  // SD
  // ==================================================

  if (sdMounted)
  {
    renderFileSection(
      html,
      SD,
      "SD CARD",
      "sd"
    );
  }
  else
  {
    html += "<hr>";
    html += "<h2>SD CARD</h2>";
    html += "<p>SD NOT MOUNTED</p>";
  }

  html += "</body></html>";

  server.send(
    200,
    "text/html",
    html
  );
}

// ======================================================
// FILE SECTION
// ======================================================

void renderFileSection(
  String& html,
  FS& fs,
  String fsName,
  String fsId
)
{
  html += "<hr>";

  // ==================================================
  // TITLE
  // ==================================================

  html += "<h2>";
  html += fsName;
  html += "</h2>";

  // ==================================================
  // UPLOAD
  // ==================================================

  html += "<form method='POST' action='/upload?fs=";

  html += fsId;

  html += "' enctype='multipart/form-data'>";

  html += "<input type='file' name='upload'> ";

  html += "<input type='submit' value='Upload'>";

  html += "</form>";

  html += "<br>";

  // ==================================================
  // FILE LIST
  // ==================================================

  File root = fs.open("/");

  File file = root.openNextFile();

  bool found = false;

  while (file)
  {
    found = true;

    String path = file.path();

    // ==============================================
    // CARD
    // ==============================================

    html += "<div style='";

    html += "border:1px solid #ccc;";
    html += "padding:12px;";
    html += "margin-bottom:12px;";
    html += "border-radius:8px;";
    html += "background:#f8f8f8;";

    html += "'>";

    // ==============================================
    // FILE NAME
    // ==============================================

    html += "<div style='";

    html += "font-weight:bold;";
    html += "margin-bottom:6px;";
    html += "word-break:break-all;";

    html += "'>";

    html += path;

    html += "</div>";

    // ==============================================
    // FILE SIZE
    // ==============================================

    html += "<div style='";

    html += "font-size:14px;";
    html += "color:#666;";
    html += "margin-bottom:10px;";

    html += "'>";

    html += formatBytes(
      file.size()
    );

    html += "</div>";

    // ==============================================
    // OPEN
    // ==============================================

    html += "<a target='_blank' ";

    html += "style='";

    html += "display:inline-block;";
    html += "padding:6px 10px;";
    html += "margin-right:6px;";
    html += "margin-bottom:6px;";
    html += "background:#2196F3;";
    html += "color:white;";
    html += "text-decoration:none;";
    html += "border-radius:4px;";

    html += "' href='/open?fs=";

    html += fsId;

    html += "&file=";

    html += path;

    html += "'>";

    html += "OPEN";

    html += "</a>";

    // ==============================================
    // DOWNLOAD
    // ==============================================

    html += "<a ";

    html += "style='";

    html += "display:inline-block;";
    html += "padding:6px 10px;";
    html += "margin-right:6px;";
    html += "margin-bottom:6px;";
    html += "background:#4CAF50;";
    html += "color:white;";
    html += "text-decoration:none;";
    html += "border-radius:4px;";

    html += "' href='/download?fs=";

    html += fsId;

    html += "&file=";

    html += path;

    html += "'>";

    html += "DOWNLOAD";

    html += "</a>";

    // ==============================================
    // RENAME
    // ==============================================

    html += "<a ";

    html += "style='";

    html += "display:inline-block;";
    html += "padding:6px 10px;";
    html += "margin-right:6px;";
    html += "margin-bottom:6px;";
    html += "background:#FF9800;";
    html += "color:white;";
    html += "text-decoration:none;";
    html += "border-radius:4px;";

    html += "' href='/rename?fs=";

    html += fsId;

    html += "&file=";

    html += path;

    html += "'>";

    html += "RENAME";

    html += "</a>";

    // ==============================================
    // DELETE
    // ==============================================

    html += "<a ";

    html += "onclick='return confirm(\"Delete file?\")' ";

    html += "style='";

    html += "display:inline-block;";
    html += "padding:6px 10px;";
    html += "margin-bottom:6px;";
    html += "background:#f44336;";
    html += "color:white;";
    html += "text-decoration:none;";
    html += "border-radius:4px;";

    html += "' href='/delete?fs=";

    html += fsId;

    html += "&file=";

    html += path;

    html += "'>";

    html += "DELETE";

    html += "</a>";

    // ==============================================
    // END CARD
    // ==============================================

    html += "</div>";

    file = root.openNextFile();
  }

  // ==================================================
  // EMPTY
  // ==================================================

  if (!found)
  {
    html += "<p>No files found</p>";
  }

  // ==================================================
  // STORAGE INFO
  // ==================================================

  html += "<hr>";

  html += "<div style='";
html += "display:flex;";
html += "align-items:center;";
html += "gap:8px;";
html += "'>";

// STORAGE TEXT

html += "<small>";

if (fsId == "lfs")
{
  html += "Used: ";

  html += formatBytes(
    LittleFS.usedBytes()
  );

  html += " / ";

  html += formatBytes(
    LittleFS.totalBytes()
  );
}
else
{
  uint64_t total =
    SD.cardSize();

  uint64_t used =
    SD.usedBytes();

  html += "Used: ";

  html += formatBytes(used);

  html += " / ";

  html += formatBytes(total);
}

html += "</small>";

// FORMAT BUTTON

if (fsId == "lfs")
{
  html += "<a ";

  html += "onclick='return confirm(\"Format filesystem?\")' ";

  html += "style='";
  html += "font-size:10px;";
  html += "padding:2px 6px;";
  html += "background:#f44336;";
  html += "color:white;";
  html += "text-decoration:none;";
  html += "border-radius:4px;";
  html += "' ";

  html += "href='/format?fs=lfs'>";

  html += "FORMAT";

  html += "</a>";
}
else
{
  html += "<small style='font-size:10px;color:#888;'>";
  html += "FORMAT DISABLED";
  html += "</small>";
}

html += "</div>";

}

// ======================================================
// OPEN
// ======================================================

void handleOpen()
{
  FS* fs = getFS();

  if (!server.hasArg("file"))
  {
    server.send(
      400,
      "text/plain",
      "BAD ARGS"
    );

    return;
  }

  String path = server.arg("file");

  if (!path.startsWith("/"))
  {
    path = "/" + path;
  }

  if (!fs->exists(path))
  {
    server.send(
      404,
      "text/plain",
      "NOT FOUND"
    );

    return;
  }

  oledLog(
    "OPEN " + path
  );

  File file = fs->open(
    path,
    "r"
  );

  server.streamFile(
    file,
    getContentType(path)
  );

  file.close();
}


// ======================================================
// DOWNLOAD
// ======================================================

void handleDownload()
{
  FS* fs = getFS();

  if (!server.hasArg("file"))
  {
    server.send(
      400,
      "text/plain",
      "BAD ARGS"
    );

    oledLog("D FAIL");

    return;
  }

  String path = server.arg("file");

  if (!path.startsWith("/"))
  {
    path = "/" + path;
  }

  if (!fs->exists(path))
  {
    server.send(
      404,
      "text/plain",
      "NOT FOUND"
    );

    oledLog("NOT FOUND");

    return;
  }

  oledLog(
    "D " + path
  );

  File file = fs->open(
    path,
    "r"
  );

  server.sendHeader(
    "Content-Disposition",
    "attachment; filename=" +
    path.substring(1)
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
  FS* fs = getFS();

  String path = server.arg("file");

  if (!path.startsWith("/"))
  {
    path = "/" + path;
  }

  if (fs->exists(path))
  {
    fs->remove(path);

    oledLog(
      "DEL " + path
    );
  }

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);
}

// ======================================================
// FORMAT
// ======================================================

void handleFormat()
{
  if (
    server.hasArg("fs") &&
    server.arg("fs") == "sd"
  )
  {
    server.send(
      200,
      "text/plain",
      "SD FORMAT DISABLED"
    );

    return;
  }

  LittleFS.format();

  oledLog("FMT FS");

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);
}

// ======================================================
// RENAME
// ======================================================

void handleRename()
{
  FS* fs = getFS();

  String file = server.arg("file");

  if (!file.startsWith("/"))
  {
    file = "/" + file;
  }

  if (!server.hasArg("new"))
  {
    String html;

    html += "<form action='/rename'>";

    html += "<input type='hidden' name='fs' value='";
    html += getFSId(fs);
    html += "'>";

    html += "<input type='hidden' name='file' value='";
    html += file;
    html += "'>";

    html += "<input name='new' placeholder='new name'>";

    html += "<input type='submit'>";

    html += "</form>";

    server.send(
      200,
      "text/html",
      html
    );

    return;
  }

  String newName =
    server.arg("new");

  if (!newName.startsWith("/"))
  {
    newName = "/" + newName;
  }

  File oldF =
    fs->open(file, "r");

  File newF =
    fs->open(newName, "w");

  while (oldF.available())
  {
    newF.write(
      oldF.read()
    );
  }

  oldF.close();

  newF.close();

  fs->remove(file);

  oledLog(
    "REN " + newName
  );

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);
}

// ======================================================
// UPLOAD
// ======================================================

File uploadFile;

void handleUpload()
{
  FS* fs = getFS();

  HTTPUpload& upload =
    server.upload();

  if (
    upload.status ==
    UPLOAD_FILE_START
  )
  {
    String filename =
      upload.filename;

    if (!filename.startsWith("/"))
    {
      filename =
        "/" + filename;
    }

    uploadFile = fs->open(
      filename,
      FILE_WRITE
    );

    oledLog(
      "U " + filename
    );
  }

  else if (
    upload.status ==
    UPLOAD_FILE_WRITE
  )
  {
    if (uploadFile)
    {
      uploadFile.write(
        upload.buf,
        upload.currentSize
      );
    }
  }

  else if (
    upload.status ==
    UPLOAD_FILE_END
  )
  {
    if (uploadFile)
    {
      uploadFile.close();
    }

    oledLog("UPLOAD OK");

  }
}

// ======================================================
// NOT FOUND
// ======================================================

void handleNotFound()
{
  String path = server.uri();

  // LITTLEFS

  if (LittleFS.exists(path))
  {
    File file =
      LittleFS.open(
        path,
        "r"
      );

    server.streamFile(
      file,
      getContentType(path)
    );

    file.close();

    return;
  }

  // SD

  if (
    sdMounted &&
    SD.exists(path)
  )
  {
    File file =
      SD.open(
        path,
        "r"
      );

    server.streamFile(
      file,
      getContentType(path)
    );

    file.close();

    return;
  }

   server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);
}

// ======================================================
// SERVER INIT
// ======================================================

void startFileServer()
{
  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/download",
    handleDownload
  );

  server.on(
    "/delete",
    handleDelete
  );

  server.on(
    "/rename",
    handleRename
  );

  server.on(
    "/format",
    handleFormat
  );

  server.on(
  "/open",
  handleOpen
);

  server.on(
  "/upload",
  HTTP_POST,

  []()
  {
    server.sendHeader(
      "Location",
      "/"
    );

    server.send(303);
  },

  handleUpload
);

  server.onNotFound(
    handleNotFound
  );

  server.begin();

  oledLog("SERVER OK");
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

  display.println(
    "ESP32 FILE SERVER"
  );

  display.display();

  delay(1000);

  oledLog("OLED OK");
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  initOLED();

  // ==================================================
  // LITTLEFS
  // ==================================================

  oledLog("MOUNT LFS");

  if (!LittleFS.begin(true))
  {
    oledLog("LFS FAIL");

    return;
  }

  oledLog("LFS OK");

  // ==================================================
  // SD
  // ==================================================

  SPI.begin(
    SD_SCK,
    SD_MISO,
    SD_MOSI,
    SD_CS
  );

  oledLog("MOUNT SD");

  sdMounted = SD.begin(
    SD_CS
  );

  if (sdMounted)
  {
    oledLog("SD OK");
  }
  else
  {
    oledLog("SD FAIL");
  }

  // ==================================================
  // WIFI
  // ==================================================

  oledLog("WIFI");

  WiFiConfig::begin();

  oledLog("WIFI OK");

  // ==================================================
  // SERVER
  // ==================================================

  startFileServer();
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
  server.handleClient();
}