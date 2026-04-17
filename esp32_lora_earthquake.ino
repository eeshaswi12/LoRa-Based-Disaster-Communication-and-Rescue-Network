#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <LoRa.h>

#define BUZZER 25

// LoRa Pins
#define NSS 5
#define RST 14
#define DIO0 2

const char* ssid = "Onepluss";
const char* password = "12345678";

// Telegram credentials
String BOT_TOKEN = "8702807558:AAEiNNXHVDdo2ua3ndRqIrLhFgUXLnk4oMc";
String CHAT_ID = "5669264555";

String zone = "SAFE";
String lastZone = "";

unsigned long previousBuzzer = 0;
bool buzzerState = false;

void setup()
{
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);

  // WiFi connection
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while(WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  // LoRa setup
  LoRa.setPins(NSS, RST, DIO0);

  if (!LoRa.begin(433E6))
  {
    Serial.println("LoRa Failed");
    while (1);
  }

  Serial.println("LoRa Receiver Ready");
}

void loop()
{
  int packetSize = LoRa.parsePacket();

  if(packetSize)
  {
    String received="";

    while(LoRa.available())
    {
      received += (char)LoRa.read();
    }

    zone = received;

    Serial.print("Zone Received: ");
    Serial.println(zone);

    // Send telegram only if zone changed
    if(zone != lastZone)
    {
      sendTelegram(zone);
      lastZone = zone;
    }
  }

  // -------- BUZZER CONTROL --------

  if(zone == "SAFE")
  {
    digitalWrite(BUZZER, LOW);
  }

  else if(zone == "ALERT")
  {
    unsigned long currentTime = millis();

    if(currentTime - previousBuzzer >= 1000)
    {
      previousBuzzer = currentTime;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER, buzzerState);
    }
  }

  else if(zone == "DANGER")
  {
    digitalWrite(BUZZER, HIGH);
  }
}

void sendTelegram(String status)
{
  if(WiFi.status()==WL_CONNECTED)
  {
    HTTPClient http;

    String message;

    if(status=="ALERT")
    {
      message="⚠ ALERT: Disaster risk detected";
    }
    else if(status=="DANGER")
    {
      message="🚨 DANGER: Immediate disaster detected";
    }
    else
    {
      message="✅ SAFE: System normal";
    }

    String url="https://api.telegram.org/bot"+BOT_TOKEN+
    "/sendMessage?chat_id="+CHAT_ID+
    "&text="+message;

    http.begin(url);

    int httpCode=http.GET();

    Serial.print("Telegram Response: ");
    Serial.println(httpCode);

    http.end();
  }
}