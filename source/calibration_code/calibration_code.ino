#include <DHT.h>

#define TRIG_PIN 5
#define ECHO_PIN 18

#define DHTPIN 4
#define DHTTYPE DHT11

#define MIC_PIN 13   // microphone A0 -> GPIO34

#define BUZZER_PIN 23

DHT dht(DHTPIN, DHTTYPE);

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1.0;
  }

  return duration * 0.0343 / 2.0;
}

void readMicWindow(int &micRaw, int &micMin, int &micMax, int &micP2P) {
  micMin = 4095;
  micMax = 0;

  for (int i = 0; i < 200; i++) {
    int v = analogRead(MIC_PIN);

    if (v < micMin) micMin = v;
    if (v > micMax) micMax = v;

    delayMicroseconds(500);
  }

  micRaw = analogRead(MIC_PIN);
  micP2P = micMax - micMin;
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  analogReadResolution(12);   // ESP32 ADC: 0 to 4095

  dht.begin();

  Serial.println("ESP32 Ultrasonic + DHT + Microphone Test Start");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    if (command == 'A') {
      digitalWrite(BUZZER_PIN, HIGH); // Sound the alarm
    } else if (command == 'O') {
      digitalWrite(BUZZER_PIN, LOW);  // Turn it off
    }
  }

  float distance = readDistanceCm();
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  int micRaw, micMin, micMax, micP2P;
  readMicWindow(micRaw, micMin, micMax, micP2P);

  // Print all the data out to the MCXC444 just like before
  Serial.print("DIST_CM=");
  Serial.print(distance >= 0 ? distance : -1.0, 2);
  Serial.print(" | HUMIDITY=");
  Serial.print(isnan(humidity) ? -1.0 : humidity, 1);
  Serial.print(" | TEMP_C=");
  Serial.print(isnan(temperature) ? -1.0 : temperature, 1);
  Serial.print(" | MIC_P2P=");
  Serial.println(micP2P);

  // Reduce delay slightly so the ESP32 is more responsive to the buzzer command
  delay(500); 
}