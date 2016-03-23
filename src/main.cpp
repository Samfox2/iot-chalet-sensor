#include <Arduino.h>

#include <SPI.h>
#include <RF24.h>
#include <NewPing.h>
#include <LowPower.h>

#define STEP_UP_PIN   5
#define TRIGGER_PIN  3
#define ECHO_PIN     4
#define MAX_DISTANCE 250

#define NB_MEASURES 5
#define NB_MEASURES_SKIP 1

#define SLEEP_STEP_DURATION SLEEP_8S
#define SLEEP_STEP_NB 2

uint8_t gatewayAddress[] = { 0x00, 0xA1, 0xB2, 0xC3, 0xD4 };
uint8_t nodeAddress[] = { 0x01, 0xA1, 0xB2, 0xC3, 0xD4 };

RF24 radio(7,8);
NewPing sonar(TRIGGER_PIN, ECHO_PIN);

void setup() {
  Serial.begin(115200);
  pinMode(STEP_UP_PIN, OUTPUT);
  digitalWrite(STEP_UP_PIN, LOW);

  // Setup and configure radio
  radio.begin();
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);

  radio.openWritingPipe(gatewayAddress);
  radio.openReadingPipe(1, nodeAddress);

  radio.powerDown();
}

void loop() {
  int i = 0;
  for(i = 0; i < SLEEP_STEP_NB; i++) {
    LowPower.powerDown(SLEEP_STEP_DURATION, ADC_OFF, BOD_OFF);
  }

  Serial.println(F("Awake"));

  // - Démarrage du power up
  digitalWrite(STEP_UP_PIN, HIGH);
  delay(100);

  // - Mesure
  int valuesInCm[NB_MEASURES];
  double avgMeasureCm = 0.0d;
  for(i = 0; i < NB_MEASURES; i++) {
    valuesInCm[i] = sonar.ping_cm();
    Serial.print(F("Distance: "));
    Serial.print(valuesInCm[i]);
    Serial.println(F("cm"));

    if(i >= NB_MEASURES_SKIP) {
      avgMeasureCm += valuesInCm[i];
    }

    // - Delay for remaining echos
    delay(70);
  }

  // Arret du step-up
  digitalWrite(STEP_UP_PIN, LOW);

  // - Time to start the radio
  radio.powerUp();

  byte valueToSend = round(avgMeasureCm / (double) (NB_MEASURES - NB_MEASURES_SKIP));

  Serial.print(F("Now sending: "));
  Serial.println(valueToSend);

  if ( radio.write(&valueToSend, 1) ) {
      if(!radio.available()) {
          Serial.println(F("Got blank ack"));
      }
  } else {
    Serial.println(F("Sending failed"));
  }

  // - Time to stop the radio
  radio.powerDown();

  Serial.println(F("----------"));
  delay(50);
}