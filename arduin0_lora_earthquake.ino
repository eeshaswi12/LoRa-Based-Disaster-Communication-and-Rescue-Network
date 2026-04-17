#include <SPI.h>
#include <LoRa.h>

// -------- ADXL --------
#define X_PIN A0
#define Y_PIN A1

// -------- Ultrasonic --------
#define TRIG 3
#define ECHO 4

// -------- LoRa --------
#define NSS 10
#define RST 9
#define DIO0 2

String zone = "SAFE";

// Calibration values
int xRest = 0;
int yRest = 0;

void setup() {

  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  LoRa.setPins(NSS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("Calibrating ADXL... Keep steady");
  delay(3000);

  // -------- Auto Calibration --------
  long xSum = 0;
  long ySum = 0;

  for (int i = 0; i < 200; i++) {
    xSum += analogRead(X_PIN);
    ySum += analogRead(Y_PIN);
    delay(5);
  }

  xRest = xSum / 200;
  yRest = ySum / 200;

  Serial.println("System Ready");
}

void loop() {

  // -------- EARTHQUAKE (Vibration) --------
  long xSum = 0;
  long ySum = 0;

  for (int i = 0; i < 10; i++) {
    xSum += analogRead(X_PIN);
    ySum += analogRead(Y_PIN);
    delay(2);
  }

  int xVal = xSum / 10;
  int yVal = ySum / 10;

  int vibration = abs(xVal - xRest) + abs(yVal - yRest);

  if (vibration < 50) vibration = 0;

  // -------- FLOOD (Ultrasonic) --------
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);

  float distance;

  if (duration == 0) {
    distance = 999;
  } else {
    distance = duration * 0.034 / 2;
  }

  // -------- ZONE DECISION --------

  bool eqDanger = vibration > 300;
  bool eqAlert  = vibration > 150;

  bool floodDanger = distance < 10;
  bool floodAlert  = distance < 25;

  if (eqDanger || floodDanger) {
    zone = "DANGER";
  }
  else if (eqAlert || floodAlert) {
    zone = "ALERT";
  }
  else {
    zone = "SAFE";
  }

  // -------- Serial Output --------
  Serial.print("Vibration: ");
  Serial.print(vibration);
  Serial.print(" | Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Zone: ");
  Serial.println(zone);

  // -------- Send via LoRa --------
  LoRa.beginPacket();
  LoRa.print(zone);
  LoRa.endPacket();

  delay(2000);
}