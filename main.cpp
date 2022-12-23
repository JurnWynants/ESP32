//includes
#include <Wire.h>
#include <BH1750.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_I2CDevice.h>
#include <PubSubClient.h>
#include "discord.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>


BH1750 lightMeter;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C
// Ensure that the credentials here allow you to publish and subscribe to the ThingSpeak channel.
#define channelID 1967205
const char mqttUserName[] = "MAIuLBgzMhwjOQsYMzYeFRM"; 
const char clientID[] = "MAIuLBgzMhwjOQsYMzYeFRM";
const char mqttPass[] = "ew7meQaM4EF9bW5+pNQh+sDX";
// It is strongly recommended to use secure connections. However, certain hardware does not work with the WiFiClientSecure library.

#ifdef ESP8266BOARD
  #include <ESP8266WiFi.h>
  const char* PROGMEM thingspeak_cert_thumbprint = "271892dda426c30709b97ae6c521b95b48f716e1";
#else
  #include <WiFi.h>
  const char * PROGMEM thingspeak_ca_cert = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
  "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
  "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
  "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
  "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
  "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
  "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
  "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
  "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
  "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
  "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
  "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
  "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
  "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
  "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
  "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
  "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
  "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
  "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
  "+OkuE6N36B9K\n" \
  "-----END CERTIFICATE-----\n";
#endif

#ifdef USESECUREMQTT
  #include <WiFiClientSecure.h>
  #define mqttPort 8883
  WiFiClientSecure client; 
#else
  #define mqttPort 1883
  WiFiClient client;
#endif

const char* server = "mqtt3.thingspeak.com";
int status = WL_IDLE_STATUS; 
long lastPublishMillis = 0;
int connectionDelay = 1;
int updateInterval = 15;
PubSubClient mqttClient( client );

// Function to handle messages from MQTT subscription.
void mqttSubscriptionCallback( char* topic, byte* payload, unsigned int length ) {
  // Print the details of the message that was received to the serial monitor.
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Subscribe to ThingSpeak channel for updates.
void mqttSubscribe( long subChannelID ){
  String myTopic = "channels/"+String( subChannelID )+"/subscribe";
  mqttClient.subscribe(myTopic.c_str());
}

// Publish messages to a ThingSpeak channel.
void mqttPublish(long pubChannelID, String message) {
  String topicString ="channels/" + String( pubChannelID ) + "/publish";
  mqttClient.publish( topicString.c_str(), message.c_str() );
}


// Connect to MQTT server.
void mqttConnect() {
  // Loop until connected.
  while ( !mqttClient.connected() )
  {
    // Connect to the MQTT broker.
    if ( mqttClient.connect( clientID, mqttUserName, mqttPass ) ) {
      Serial.print( "MQTT to " );
      Serial.print( server );
      Serial.print (" at port ");
      Serial.print( mqttPort );
      Serial.println( " successful." );
    } else {
      Serial.print( "MQTT connection failed, rc = " );
      // See https://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
      Serial.print( mqttClient.state() );
      Serial.println( " Will try again in a few seconds" );
      delay( connectionDelay*1000 );
    }
  }
    Serial.println("----------------------------------");
}

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1800        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}



void setup() {
  Serial.begin(9600);
  // Delay to allow serial monitor to come up.
    delay(1000);
  // connect to wifi
   connectWIFI();

  // track and print boot count
  ++bootCount;
  Serial.println();
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
  First we configure the wake up source
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.print("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
  Serial.println();
  Serial.println("----------------------------------");
  

  //if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
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
  // Configure the MQTT client
  mqttClient.setServer( server, mqttPort ); 
  // Set the MQTT message handler function.
  mqttClient.setCallback( mqttSubscriptionCallback );
  // Set the buffer to handle the returned JSON. NOTE: A buffer overflow of the message buffer will result in your callback not being invoked.
  mqttClient.setBufferSize( 2048 );
  // Use secure MQTT connections if defined.
  #ifdef USESECUREMQTT
    // Handle functionality differences of WiFiClientSecure library for different boards.
    #ifdef ESP8266BOARD
      client.setFingerprint(thingspeak_cert_thumbprint);
    #else
      client.setCACert(thingspeak_ca_cert);
    #endif
  #endif
  Wire.begin();
  lightMeter.begin();
}

void loop() {
  
   // Connect if MQTT client is not connected and resubscribe to channel updates.
   if (!mqttClient.connected()) {
      mqttConnect(); 
      mqttSubscribe( channelID );
   }
  
   // Call the loop to maintain connection to the server.
   mqttClient.loop(); 
  
   // Update ThingSpeak channel periodically. The update results in the message to the subscriber.
   if (bmp.takeForcedMeasurement()) {
    // can now print out the new measurements to thingspeak and serial monitor
     timeClient.update();
     timeClient.setTimeOffset(3600);
     String currentTime = timeClient.getFormattedTime();
     Serial.println(currentTime);
     Serial.print(F("Temperature = "));
     float temperature = bmp.readTemperature();
     Serial.print(temperature);
     Serial.println(" *C");
     Serial.print(F("Pressure = "));
     float Pressure = (bmp.readPressure()/100);
     Serial.print(Pressure);
     Serial.println(" hPa");
     Serial.print(F("Approx altitude = "));
     float altitude = (bmp.readAltitude(1000.66));
     Serial.print(altitude); /* Adjusted to local forecast! */
     Serial.println(" m");
     float lux = lightMeter.readLightLevel();
     Serial.print("Light: ");
     Serial.print(lux);
     Serial.println(" lx");
     Serial.println("----------------------------------");
     // send overview to discord channel
     sendDiscord("Time = " + String(currentTime) + "                                                                                                                                                                                                                                                                  " + "Temperature = " + String(temperature)+" °C" + "                                                                                                                                                                                                                                                    " + "Pressure = " + String(Pressure)+" hPa" + "                                                                                                                                                                                                                                                    " + "Altitude = " + String(altitude)+" m" + "                                                                                                                                                                                                                                                         " + "Light = " + String(lux)+" lx" + "                                                                                                                                                                                                                                                                   " + "-------------------------------------------------------");
     delay(50);
     // publish to thingspeak channel
     mqttPublish( channelID, (String("field1=")+String(lux)+"&"+String("field2=")+String(temperature)+"&"+String("field3=")+String(altitude)+"&"+String("field4=")+String(Pressure)));
     lastPublishMillis = millis();
     // Start deep sleep
     Serial.println("Going to sleep now");
     delay(1000);
     Serial.flush(); 
     esp_deep_sleep_start();
  }
 }


