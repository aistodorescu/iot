#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

/* Global features */
#define USE_SERIAL_COM              (0)

/* Libraries */
#include "tha.h"


#define USE_OTA                     (1)

#define LYT8266_POWER_PIN           15
#define LYT8266_RED_PIN             13
#define LYT8266_GREEN_PIN           12
#define LYT8266_BLUE_PIN            14
#define LYT8266_WHITE_PIN           2
#define LYT8266_TurnOn()            digitalWrite(LYT8266_POWER_PIN, 1);
#define LYT8266_TurnOff()           digitalWrite(LYT8266_POWER_PIN, 0);
#define LYT8266_SetBrightness(x)    analogWrite(LYT8266_WHITE_PIN, x);

#define LYT8266_ID                  "lyt8266_1" /* needs to be the same as programming port */


const char *pWifiSsId               = WIFI_SSID;
const char *pWifiPass               = WIFI_PSK;
const char *pMqttClientId           = LYT8266_ID;
const char *pMqttBrokerIp           = MQTT_BROKER_IP;
uint16_t mqttBrokerPort             = MQTT_BROKER_PORT;

const char *pMqttTopics[] =  {
        "/lyt8266/" LYT8266_ID "/set""power",
        "/lyt8266/" LYT8266_ID "/set""color",
        "/lyt8266/" LYT8266_ID "/set""brightness"
};
const char *pMqttAcks[] = {
        "/lyt8266/" LYT8266_ID "/set""powerack",
        "/lyt8266/" LYT8266_ID "/set""colorack",
        "/lyt8266/" LYT8266_ID "/set""brightnessack"
};

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


void LYT8266_SetColor(uint8_t red, uint8_t green, uint8_t blue){
    analogWrite(LYT8266_RED_PIN, red);
    analogWrite(LYT8266_GREEN_PIN, green);
    analogWrite(LYT8266_BLUE_PIN, blue);
}

void WIFI_ConnectingHandler()
{
    SERIAL_PRINT(".");
    delay(500);
    LYT8266_SetColor(255,0,0);
    delay(500);
    LYT8266_SetColor(0,255,0);
    delay(500);
    LYT8266_SetColor(0,0,255);
}

/**
 * MQTT callback to process messages
 */
void Mqtt_Handler(char *pTopic, byte *pPayload, unsigned int length)
{
    char aPayload[10] = {0};
    int i;
    String newStr;
    SERIAL_PRINT(pTopic);
    SERIAL_PRINT(": ");

    for (i=0;i<length;i++) {
        aPayload[i] = (uint8_t)pPayload[i];
        SERIAL_PRINT((char)pPayload[i]);
    }
    aPayload[i] = 0;
    SERIAL_PRINTLN();
    newStr = String(aPayload);

    if(String(pTopic) == "/lyt8266/" LYT8266_ID "/set""power")
    {
        if (newStr == "ON")
        {
            mqttClient.publish(pMqttAcks[0], "ON");
            LYT8266_TurnOn();
            SERIAL_PRINT("Bulb ON\n");
        }
        else if (newStr == "OFF")
        {
            mqttClient.publish(pMqttAcks[0], "OFF");
            LYT8266_TurnOff();
            SERIAL_PRINT("Bulb OFF\n");
        }
    }
    else if (String(pTopic) == "/lyt8266/" LYT8266_ID "/set""color")
    {
        int red = newStr.substring(0, newStr.indexOf(',')).toInt();
        int green = newStr.substring(newStr.indexOf(',') + 1, newStr.lastIndexOf(',')).toInt();
        int blue = newStr.substring(newStr.lastIndexOf(',') + 1).toInt();

        LYT8266_SetBrightness(0);

        mqttClient.publish(pMqttAcks[2], aPayload);
        LYT8266_SetColor(red, green, blue);
    }
    else if (String(pTopic) == "/lyt8266/" LYT8266_ID "/set""brightness")
    {
        int brightness = newStr.toInt();

        LYT8266_TurnOn();
        LYT8266_SetColor(0, 0, 0);
        LYT8266_SetBrightness(brightness);

        SERIAL_PRINT("Set brightness to: ");
        SERIAL_PRINT(brightness);
        SERIAL_PRINT("\n");
    }
}



void setup()
{
    /* Set up the outputs */
    pinMode(LYT8266_POWER_PIN, OUTPUT);
    pinMode(LYT8266_RED_PIN, OUTPUT);
    pinMode(LYT8266_GREEN_PIN, OUTPUT);
    pinMode(LYT8266_BLUE_PIN, OUTPUT);

    //LYT8266_TurnOn();

    LYT8266_SetColor(255,0,0);

    /* Setup Serial */
    SERIAL_INIT();

    THA_WifiManage(pWifiSsId, pWifiPass, WIFI_ConnectingHandler);

    SERIAL_PRINTLN("WiFi connected");
    SERIAL_PRINTLN("IP address: ");
    SERIAL_PRINTLN(WiFi.localIP());

#if USE_OTA
    //WiFi.hostname(pMqttClientId);
    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp8266.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (!MDNS.begin(pMqttClientId)) {
        SERIAL_PRINTLN("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    SERIAL_PRINTLN("mDNS responder started");

    /* Setup OTA */
    ArduinoOTA.setHostname(pMqttClientId);
    /*ArduinoOTA.onStart([]() {
        SERIAL_PRINTLN("Start");
    });
    ArduinoOTA.onEnd([]() {
        SERIAL_PRINTLN("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) SERIAL_PRINTLN("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) SERIAL_PRINTLN("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) SERIAL_PRINTLN("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) SERIAL_PRINTLN("Receive Failed");
        else if (error == OTA_END_ERROR) SERIAL_PRINTLN("End Failed");
    });*/
    ArduinoOTA.begin();
#endif
}

void loop()
{
    THA_WifiManage(pWifiSsId, pWifiPass, WIFI_ConnectingHandler);
    THA_MQTTManage(
            mqttClient,
            pMqttClientId,
            pMqttBrokerIp,
            mqttBrokerPort,
            NULL,
            NULL,
            Mqtt_Handler,
            pMqttTopics,
            3,
            NULL);
    ArduinoOTA.handle();
}
