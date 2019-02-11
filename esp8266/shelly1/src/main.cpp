#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <ESP8266mDNS.h>

/* Global features */
#define USE_SERIAL_COM          (1)
#define USE_JSON                (0)

#include "tha.h"



#define USE_OTA                 (1)

#define SHELLY1_RELAY           (4)
#define SHELLY1_PB              (0)
#define SHELLY1_On()            digitalWrite(SHELLY1_RELAY, HIGH);
#define SHELLY1_Off()           digitalWrite(SHELLY1_RELAY, LOW);
#define KEY_DEBOUNCE_TIMEOUT_S  (1)


const char *pWifiSsId       = WIFI_SSID;
const char *pWifiPass       = WIFI_PSK;
const char *pMqttClientId   = MQTT_CLIENT_ID;
const char *pMqttBrokerIp   = MQTT_BROKER_IP;
uint16_t mqttBrokerPort     = MQTT_BROKER_PORT;
const char *pMqttTopics[] =  {
    "/shelly1/relay",
};
const char *pMqttAcks[] =  {
    "/shelly1/relay/ack",
};

WiFiClient wificlient;
PubSubClient mqttClient(wificlient);
Ticker flipper;

uint8_t mRelayState = 0;
volatile bool mAllowButton = true;

void WIFI_ConnectingHandler()
{
    SERIAL_PRINT(".");
    delay(500);
    delay(500);
}

/**
 * MQTT callback to process messages
 */
void Mqtt_Handler(char *pTopic, byte *pPayload, unsigned int length) {
    char aPayload[10] = {0};
    unsigned int i;
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

    if (newStr == "ON")
    {
        SHELLY1_On();
        mRelayState = 1;
        mqttClient.publish(pMqttAcks[0], "ON");
    }
    else if(newStr == "OFF")
    {
        SHELLY1_Off();
        mRelayState = 0;
        mqttClient.publish(pMqttAcks[0], "OFF");
    }
}

void Key_Timeout()
{
    mAllowButton = true;
}

/**
 * Setup
 */
void setup() {
    /* Set up the outputs */
    pinMode(SHELLY1_RELAY, OUTPUT);
    pinMode(SHELLY1_PB, INPUT);
    SHELLY1_Off();

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
        Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });*/
    ArduinoOTA.begin();
#endif
}

/**
 * Main
 */
void loop() {
    uint8_t pinValue;

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
                1,
                NULL);
    ArduinoOTA.handle();

    /* Button logic */
#if 0
    if (mAllowButton)
    {
        pinValue = digitalRead(SONOFF_PB);
        if (pinValue == 0)
        {
            if(mRelayState == 0)
            {
                SONOFF_On();
                mRelayState = 1;
                mqttClient.publish(pMqttAcks[0], "ON");
            }
            else
            {
                SONOFF_Off();
                mRelayState = 0;
                mqttClient.publish(pMqttAcks[0], "OFF");
            }

            /* Start timer for debounce */
            flipper.once(KEY_DEBOUNCE_TIMEOUT_S, Key_Timeout);
            mAllowButton = false;
        }

    }
#endif
}
