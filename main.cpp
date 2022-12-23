
// includes
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>



/*WiFi Credentials */
const char* ssid = "telenet-7643A"; // SSID
const char* password = "VQc2mMK0YTGQ"; // Password

/* Start Webserver */
AsyncWebServer server(80);

/* Attach ESP-DASH to AsyncWebServer */
ESPDash dashboard(&server); 

/* 
  Dashboard Cards 
  Format - (Dashboard Instance, Card Type, Card Name, Card Symbol(optional) )
*/
Card temperature(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
Card pressure(&dashboard, GENERIC_CARD, "Pressure", "hPa");
Card light(&dashboard, GENERIC_CARD, "Light", "lx");
Card altitude(&dashboard, GENERIC_CARD, "Altitude", "m");

// Define BMP
#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

// sensors
Adafruit_BMP280 bmp;
BH1750 lightMeter;

void setup() {
  // start serial monitor
  Serial.begin(9600);

  /* Connect WiFi */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      return;
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  /* Start AsyncWebServer and sensors */
  server.begin();
  Wire.begin();
  lightMeter.begin();

  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

void loop() {
  /* Update DASH Card Values */
  float Temperature = bmp.readTemperature();
  float Pressure = bmp.readPressure()/100;
  float Altitude = bmp.readAltitude();
  float lux = lightMeter.readLightLevel();
  temperature.update(Temperature);
  pressure.update(Pressure);
  altitude.update(Altitude);
  light.update(lux);
  /* Send Updates to DASH Dashboard (realtime) */
  dashboard.sendUpdates();

  /* Delay voor testdoeleinden */
  delay(3000);
}