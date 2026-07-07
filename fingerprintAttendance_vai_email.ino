
#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <ESP_Mail_Client.h>   // Install: ESP Mail Client by Mobizt

// ============================================================
// WIFI
// ============================================================
const char* ssid     = "manisha's A17";
const char* password = "1234567890";

// ============================================================
// EMAIL  –– fill these in before uploading
// ============================================================
#define SMTP_HOST       "smtp.gmail.com"
#define SMTP_PORT       465

#define SENDER_EMAIL    "anusanu2926@gmail.com"    // <-- change
#define SENDER_PASSWORD "iwix rqvt qahz gvho"     // <-- Gmail App Password
#define RECIPIENT_EMAIL "anujajadhav2946@gmail.com"      // <-- change

// ============================================================
// WEB SERVER
// ============================================================
WebServer server(80);

// ============================================================
// LCD
// ============================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================================================
// FINGERPRINT
// ============================================================
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ============================================================
// DATA
// ============================================================
String currentUser   = "No Attendance";
String attendanceLog = "";
String emailStatus   = "";

// ============================================================
// COOLDOWN
// ============================================================
unsigned long lastScan = 0;
const unsigned long scanCooldown = 5000;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
void   getFingerprintID();
String getDateTime();
void   sendAttendanceEmail();

// ============================================================
// SMTP
// ============================================================
SMTPSession smtp;

void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
}

// ============================================================
// SETUP
// ============================================================
void setup() {

  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 25) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi Failed");
  }

  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  int ntpRetry = 0;
  while (!getLocalTime(&timeinfo) && ntpRetry < 10) {
    Serial.println("Waiting NTP...");
    delay(1000);
    ntpRetry++;
  }
  Serial.println("Time Ready");

  // --- Main page ---
  server.on("/", []() {

    String html;
    html += "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='3'>";
    html += "<meta charset='UTF-8'>";
    html += "<title>Attendance System</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f4; }";
    html += "h1 { color: #333; }";
    html += "table { border-collapse: collapse; margin: auto; background: white; }";
    html += "th,td { border: 1px solid #ccc; padding: 8px 16px; }";
    html += "th { background: #4CAF50; color: white; }";
    html += ".btn { padding: 12px 28px; font-size: 16px; background: #2196F3; color: white; border: none; border-radius: 6px; cursor: pointer; margin-top: 20px; }";
    html += ".btn:hover { background: #1565C0; }";
    html += ".status { margin-top: 12px; font-weight: bold; color: #e65100; }";
    html += "</style></head><body>";

    html += "<h1>ESP32 Fingerprint Attendance</h1>";
    html += "<h3>Current Time: " + getDateTime() + "</h3>";
    html += "<h2>Latest: " + currentUser + "</h2>";

    html += "<table>";
    html += "<tr><th>#</th><th>Name</th><th>Date & Time</th></tr>";
    if (attendanceLog == "") {
      html += "<tr><td colspan='3'>No records yet</td></tr>";
    } else {
      html += attendanceLog;
    }
    html += "</table>";

    html += "<br>";
    html += "<form action='/send_email' method='GET'>";
    html += "<button class='btn' type='submit'>Send Attendance Email</button>";
    html += "</form>";

    if (emailStatus != "") {
      html += "<p class='status'>" + emailStatus + "</p>";
    }

    html += "</body></html>";

    server.send(200, "text/html", html);
  });

  // --- Send email route ---
  server.on("/send_email", []() {
    sendAttendanceEmail();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();

  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint OK");
    lcd.clear();
    lcd.print("Sensor Ready");
    delay(2000);
  } else {
    Serial.println("Sensor Error");
    lcd.clear();
    lcd.print("Sensor Error");
    while (1);
  }

  lcd.clear();
  lcd.print("Place Finger");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  server.handleClient();
  getFingerprintID();
}

// ============================================================
// GET DATE & TIME
// ============================================================
String getDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "No Time";
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// ============================================================
// FINGERPRINT SCAN
// ============================================================
String attendanceText = "";
int    recordCount    = 0;

void getFingerprintID() {

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return;

  if (millis() - lastScan < scanCooldown) return;
  lastScan = millis();

  String name;
  if      (finger.fingerID == 1) name = "ANUJA";
  else if (finger.fingerID == 2) name = "ANIL";
  else if (finger.fingerID == 3) name = "PRANALI";
  else if (finger.fingerID == 4) name = "SARIKA";
  else if (finger.fingerID == 6) name = "PRAJAKTA";
  else if (finger.fingerID == 7) name = "GITA";
  else if (finger.fingerID == 8) name = "ANURADHA";
  else                            name = "ID " + String(finger.fingerID);

  String dt = getDateTime();
  recordCount++;

  currentUser = name + " - " + dt;

  attendanceLog  += "<tr><td>" + String(recordCount) + "</td><td>" + name + "</td><td>" + dt + "</td></tr>";
  attendanceText += String(recordCount) + ". " + name + "  |  " + dt + "\n";

  Serial.println(name + " | " + dt);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(name);
  lcd.setCursor(0, 1);
  lcd.print(dt.substring(0, 16));
  delay(2000);

  lcd.clear();
  lcd.print("Place Finger");
}

// ============================================================
// SEND EMAIL
// ============================================================
void sendAttendanceEmail() {

  if (attendanceText == "") {
    emailStatus = "No attendance records to send!";
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sending Email..");

  smtp.debug(1);
  smtp.callback(smtpCallback);

  ESP_Mail_Session session;
  session.server.host_name  = SMTP_HOST;
  session.server.port       = SMTP_PORT;
  session.login.email       = SENDER_EMAIL;
  session.login.password    = SENDER_PASSWORD;
  session.login.user_domain = "";

  SMTP_Message message;
  message.sender.name  = "ESP32 Attendance";
  message.sender.email = SENDER_EMAIL;
  message.subject      = "Attendance Report - " + getDateTime();
  message.addRecipient("Receiver", RECIPIENT_EMAIL);

  String body  = "Attendance Report\n";
         body += "Generated: " + getDateTime() + "\n";
         body += "Total Records: " + String(recordCount) + "\n\n";
         body += "-----------------------------------\n";
         body += attendanceText;
         body += "-----------------------------------\n";
         body += "\nSent from ESP32 Fingerprint Attendance System";

  message.text.content = body;
  message.text.charSet = "utf-8";

  if (!smtp.connect(&session)) {
    Serial.println("SMTP connect failed: " + smtp.errorReason());
    emailStatus = "Email Failed: " + smtp.errorReason();
    lcd.clear();
    lcd.print("Email Failed!");
    delay(2000);
    lcd.clear();
    lcd.print("Place Finger");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Send failed: " + smtp.errorReason());
    emailStatus = "Send Failed: " + smtp.errorReason();
    lcd.clear();
    lcd.print("Send Failed!");
  } else {
    Serial.println("Email Sent OK");
    emailStatus = "Email sent successfully at " + getDateTime();
    lcd.clear();
    lcd.print("Email Sent!");
  }

  smtp.closeSession();
  delay(2000);
  lcd.clear();
  lcd.print("Place Finger");
}