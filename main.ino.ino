#include <PPP.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>


// ================= MODEM =================
#define MODEM_TX 17
#define MODEM_RX 16
#define APN "internet"
// ================= SENSOR PINS =================
#define SENSOR1_PIN 32   // 4–20mA
#define SENSOR2_PIN 33   // 4–13mA

#define NUM_SAMPLES 50
#define FILTER_SIZE 10

// ==== ADC calibration ====
#define ADC1_LOW   590    // 4mA
#define ADC1_HIGH  3440   // 20mA

#define ADC2_LOW   590    // 4mA
#define ADC2_HIGH  2272   // 13mA approx 

//buffers (global)
float filterBuf1[FILTER_SIZE];
float filterBuf2[FILTER_SIZE];
int idx1 = 0;
int idx2 = 0;



// ================= API =================
const char* tokenUrl =
"https://.......add your................";

const char* dataUrl =
"https:/........add your ..........................";

const char* basicAuth =
"..........add your .....................";

// ================= USER =================
const char* username = "add your ";
const char* userPassword = "add your ";

// ================= TOKEN =================
String accessToken = "";
unsigned long tokenExpiry = 0;

// =================================================
// PPP START
// =================================================
void startPPP()
{
  Serial.println("\nStarting PPP...");

  PPP.setApn(APN);
  PPP.setPins(MODEM_TX, MODEM_RX, -1, -1, ESP_MODEM_FLOW_CONTROL_NONE);

  //  EC200U must use GENERIC
  PPP.begin(PPP_MODEM_GENERIC);

  Serial.print("Waiting for attach");

  for (int i = 0; i < 40; i++)
  {
    Serial.print(".");
    if (PPP.attached()) break;
    delay(500);
  }

  Serial.println();

  if (!PPP.attached())
  {
    Serial.println("PPP attach failed!");
    while (1);
  }

  PPP.mode(ESP_MODEM_MODE_DATA);

  if (!PPP.waitStatusBits(ESP_NETIF_CONNECTED_BIT, 20000))
  {
    Serial.println("PPP connect failed!");
    while (1);
  }

  Serial.println("PPP CONNECTED!");
  Serial.println(PPP.localIP());
}

// =================================================
// TOKEN REQUEST
// =================================================
void getAccessToken()
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  if (!https.begin(client, tokenUrl))
    return;

  https.useHTTP10(true);

  https.addHeader("Authorization", basicAuth);
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData =
    "grant_type=password&username=" +
    String(username) +
    "&password=" +
    String(userPassword);

  Serial.println("\nRequesting token...");

  int httpCode = https.POST(postData);

  if (httpCode == HTTP_CODE_OK)
  {
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, https.getString());

    accessToken = doc["access_token"].as<String>();
    int expiresIn = doc["expires_in"];

    tokenExpiry = millis() + (expiresIn * 1000UL);

    Serial.println("Token received");
  }

  https.end();
}

// =================================================
// TOKEN CHECK
// =================================================
bool isTokenValid()
{
  return accessToken.length() && millis() < tokenExpiry;
}

// =================================================
// SENSOR DATA SEND
// =================================================
void sendSensorData()
{
  if (!isTokenValid())
  {
    Serial.println("Refreshing token...");
    getAccessToken();
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  if (!https.begin(client, dataUrl))
    return;

  https.useHTTP10(true);

  https.addHeader("Authorization", "Bearer " + accessToken);
  https.addHeader("Content-Type", "application/json");

  //  FULL mandatory payload (per official API doc)
  String dateTime = getDateTimeString();
  int percent1 = readChannel1Percent();
  int percent2 = readChannel2Percent();

  Serial.print("CH1: ");
  Serial.print(percent1);
  Serial.print("%  |  CH2: ");
  Serial.print(percent2);
  Serial.println("%");


  String payload = "{";
    payload += "\"coachNo\":\"233443\",";
    payload += "\"waterTankPercentage\":" + String(percent1) + ",";
    payload += "\"readingDateTime\":\"" + dateTime + "\",";
    payload += "\"sensorLocation\":\"C\"";
  payload += "}";

  Serial.println("\nSending Sensor Data...");
  Serial.println(payload);

  int httpCode = https.POST(payload);

  Serial.print("HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode > 0)
  {
    Serial.println(https.getString());
  }

  https.end();
}

void stopPPP()
{
  Serial.println("Closing PPP...");

  PPP.mode(ESP_MODEM_MODE_COMMAND);
  delay(300);

  PPP.end();

  Serial.println("PPP closed");
}



void syncTime()
{
  Serial.println("Syncing time...");

  // IST (GMT+5:30)
  configTime(19800, 0, "pool.ntp.org", "time.google.com");

  struct tm timeinfo;

  for (int i = 0; i < 20; i++)
  {
    if (getLocalTime(&timeinfo))
    {
      Serial.println("Time synced");
      return;
    }
    delay(500);
  }

  Serial.println("Time sync failed");
}




String getDateTimeString()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
    return "01/01/2000 00:00";

  char buf[20];

  strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", &timeinfo);

  return String(buf);
}

//   helper functions

float mapFloat(float x, float in_min, float in_max,
               float out_min, float out_max)
{
  return (x-in_min)*(out_max-out_min)/(in_max-in_min)+out_min;
}

//---Averaged ADC

int readADC(int pin)
{
  long sum = 0;

  for(int i=0;i<NUM_SAMPLES;i++)
  {
    sum += analogRead(pin);
    delayMicroseconds(200);
  }

  return sum / NUM_SAMPLES;
}


//-------Moving average

float movingAverage(float val, float *buf, int &idx)
{
  buf[idx] = val;
  idx = (idx + 1) % FILTER_SIZE;

  float sum = 0;
  for(int i=0;i<FILTER_SIZE;i++)
    sum += buf[i];

  return sum / FILTER_SIZE;
}


int readChannel1Percent()
{
  int adc = readADC(SENSOR1_PIN);

  float mA = mapFloat(adc, ADC1_LOW, ADC1_HIGH, 4.0, 20.0);
  mA = constrain(mA, 4.0, 20.0);

  // mA = movingAverage(mA, filterBuf1, idx1);

  float percent = mapFloat(mA, 4.0, 20.0, 0.0, 100.0);

  return constrain(percent, 0, 100);
}


//-------Channel-2 (4–13mA → %)

int readChannel2Percent()
{
  int adc = readADC(SENSOR2_PIN);

  float mA = mapFloat(adc, ADC2_LOW, ADC2_HIGH, 4.0, 13.0);
  mA = constrain(mA, 4.0, 13.0);

  // mA = movingAverage(mA, filterBuf2, idx2);

  float percent = mapFloat(mA, 4.0, 13.0, 0.0, 100.0);

  return constrain(percent, 0, 100);
}





// =================================================
// SETUP
// =================================================
// 




void setup()
{
  Serial.begin(115200);
  delay(2000);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSOR1_PIN, ADC_11db);
  analogSetPinAttenuation(SENSOR2_PIN, ADC_11db);

  Serial.println("\n===== BOOT SEND =====");

  //  SEND ONCE IMMEDIATELY AFTER BOOT
  startPPP();
  syncTime();
  getAccessToken();
  sendSensorData();
  stopPPP();

  Serial.println("Boot send done. Switching to 15-minute cycle...");
}












// =================================================
// LOOP (optional periodic send)
// =================================================




#define SEND_INTERVAL 900000UL   // 15 minutes
// #define SEND_INTERVAL 180000UL   // 3 minutes

void loop()
{
  static unsigned long lastSend = millis(); 
  // ↑ start counting AFTER boot send

  if (millis() - lastSend >= SEND_INTERVAL)
  {
    Serial.println("\n===== 15 MIN SEND =====");

    startPPP();
    syncTime();
    getAccessToken();
    sendSensorData();
    stopPPP();

    lastSend = millis();
  }
}


// void loop()
// {
  
  
//  // Example: send every 60s
//   static unsigned long lastSend = 0;

//   if (millis() - lastSend > 60000)
//   {
//     sendSensorData();
//     lastSend = millis();
//   }
// }
