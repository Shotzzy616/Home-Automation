#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include <DFRobot_DF1201S.h>


#define WIFI_SSID "*****"
#define WIFI_PASSWORD "*****"

#define API_KEY "******"
#define DATABASE_URL "*****" 

#define LED D1
#define LED1 D2
#define LED2 D3
#define DHTPIN D0
#define DHTTYPE DHT11

#define DF1201SSerial Serial1

FirebaseData fbdo, fbdo_living, fbdo_kitchen, fbdo_setup, fbdo_volume, fbdo_elden;
FirebaseAuth auth;
FirebaseConfig config;

DHT dht(DHTPIN, DHTTYPE);

unsigned long sendDataPrevMillis = 0;
int countt = 0;
bool signupOK = false;
float temperature;
float humidity;
bool ledStatus;

int volume;
bool isPlaying = false;
DFRobot_DF1201S DF1201S;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.RTDB.beginStream(&fbdo_living, "Lighting/Living")) {
    Serial.printf("stream living begin error, %s\n\n", fbdo_living.errorReason().c_str());
  }
  if (!Firebase.RTDB.beginStream(&fbdo_kitchen, "Lighting/Kitchen")) {
    Serial.printf("stream kitchen begin error, %s\n\n", fbdo_kitchen.errorReason().c_str());
  }
  if (!Firebase.RTDB.beginStream(&fbdo_setup, "Lighting/Setup")) {
    Serial.printf("stream setup begin error, %s\n\n", fbdo_setup.errorReason().c_str());
  }
  if (!Firebase.RTDB.beginStream(&fbdo_elden, "Sound/Kitchen")) {
    Serial.printf("stream kitchen begin error, %s\n\n", fbdo_kitchen.errorReason().c_str());
  }
  if (!Firebase.RTDB.beginStream(&fbdo_setup, "Lighting/Setup")) {
    Serial.printf("stream setup begin error, %s\n\n", fbdo_setup.errorReason().c_str());
  }
  if (!Firebase.RTDB.beginStream(&fbdo_elden, "Sound/Elden")) {
    Serial.printf("stream elden begin error, %s\n\n", fbdo_elden.errorReason().c_str());
  }
  if (!Firebase.RTDB.beginStream(&fbdo_volume, "Sound/Volume")) {
    Serial.printf("stream volume begin error, %s\n\n", fbdo_volume.errorReason().c_str());
  }
#if (defined ESP32)
  DF1201SSerial.begin(115200, SERIAL_8N1, /*rx =*/D7, /*Tx =*/D6);
#else
  DF1201SSerial.begin(115200);
#endif
  while (!DF1201S.begin(DF1201SSerial)) {
    Serial.println("Init failed, please check the wire connection!");
    delay(1000);
  }
  DF1201S.switchFunction(DF1201S.MUSIC);
  delay(2000);
  DF1201S.setPlayMode(DF1201S.ALLCYCLE);
  Serial.print("PlayMode:");
  Serial.println(DF1201S.getPlayMode());
}
void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    sendData();
  }
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    Data();
  }
  if (Firebase.ready() && signupOK) {
    recieveData();
  }
}
void sendData() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (Firebase.RTDB.setFloat(&fbdo, "Sensor/Humidity", humidity)) {
    Serial.println(); Serial.print(humidity);
    Serial.print(" - successfuly saved to: " + fbdo.dataPath());
    Serial.println(" (" + fbdo.dataType() + ") ");
  }
  else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
  if (Firebase.RTDB.setFloat(&fbdo, "Sensor/Temperature", temperature)) {
    Serial.println(); Serial.print(temperature);
    Serial.print(" - successfuly saved to: " + fbdo.dataPath());
    Serial.println(" (" + fbdo.dataType() + ") ");
  }
  else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
}
void recieveData() {
  if (!Firebase.RTDB.readStream(&fbdo_living)) {
    Serial.printf("stream 1 read error, %s\n\n", fbdo_living.errorReason().c_str());
  }
  if (fbdo_living.streamAvailable()) {
    if (fbdo_living.dataType() == "int") {
      int Living = fbdo_living.intData();
      Serial.println("Successfuly read from Living: " + String(Living) + " (" + fbdo_living.dataType() + ")");
      digitalWrite(LED, Living == 1 ? HIGH : LOW);
    }
  }
  if (!Firebase.RTDB.readStream(&fbdo_kitchen)) {
    Serial.printf("stream kitchen read error, %s\n\n", fbdo_kitchen.errorReason().c_str());
  }
  if (fbdo_kitchen.streamAvailable()) {
    if (fbdo_kitchen.dataType() == "int") {
      int Kitchen = fbdo_kitchen.intData();
      Serial.println("Successfuly read from Kitchen: " + String(Kitchen) + " (" + fbdo_kitchen.dataType() + ")");
      digitalWrite(LED1, Kitchen == 1 ? HIGH : LOW);
    }
  }
  if (!Firebase.RTDB.readStream(&fbdo_setup)) {
    Serial.printf("stream setup read error, %s\n\n", fbdo_setup.errorReason().c_str());
  }
  if (fbdo_setup.streamAvailable()) {
    if (fbdo_setup.dataType() == "int") {
      int Setup = fbdo_setup.intData();
      Serial.println("Successfuly read from Setup: " + String(Setup) + " (" + fbdo_setup.dataType() + ")");
      digitalWrite(LED2, Setup == 1 ? HIGH : LOW);
    }
  }
  if (!Firebase.RTDB.readStream(&fbdo_elden)) {
    Serial.printf("stream Elden read error, %s\n\n", fbdo_elden.errorReason().c_str());
  }
  if (fbdo_elden.streamAvailable()) {
    if (fbdo_elden.dataType() == "int") {
      int Elden = fbdo_elden.intData();
      Serial.println("Successfuly read from Elden: " + String(Elden) + " (" + fbdo_elden.dataType() + ")");
      if (Elden == 0) {
        DF1201S.start();
      } else if (Elden == 1) {
        DF1201S.pause();
      } else if (Elden == 2) {
        DF1201S.next();
        DF1201S.start();
      }
      playingSong();
    }
  }
  if (!Firebase.RTDB.readStream(&fbdo_volume)) {
    Serial.printf("stream volume read error, %s\n\n", fbdo_volume.errorReason().c_str());
  }
  if (fbdo_volume.streamAvailable()) {
    if (fbdo_volume.dataType() == "int") {
      int Volume = fbdo_volume.intData();
      Serial.println("Successfuly read from Volume: " + String(Volume) + " (" + fbdo_volume.dataType() + ")");
      DF1201S.setVol(Volume);
    }
  }
}
void Data() {
  if (Firebase.RTDB.getFloat(&fbdo, "Sensor/Temperature")) {
    if (fbdo.dataType() == "float") {
      float floatValue = fbdo.floatData();
      Serial.println(floatValue);
    }
  }
  else {
    Serial.println(fbdo.errorReason());
  }
}
void playingSong() {
  if (Firebase.RTDB.setString(&fbdo, "Sound/Playing", DF1201S.getFileName())) {
    Serial.println(); Serial.print(DF1201S.getFileName());
    Serial.print(" - successfuly saved to: " + fbdo.dataPath());
    Serial.println(" (" + fbdo.dataType() + ") ");
  }
  else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
}
