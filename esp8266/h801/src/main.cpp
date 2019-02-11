#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#define USE_SERIAL_COM1     0

#include "tha.h"
#include "utils.h"

#define USE_OTA             1

// RGB FET
#define H801_RED_PIN        15 //12
#define H801_GREEN_PIN      13 //15
#define H801_BLUE_PIN       12 //13

// W FET
#define H801_W1_PIN         14
#define H801_W2_PIN         4

#define H801_GREEN_D1_PIN   1 // ON = 0
#define H801_RED_D2_PIN     5 // ON = 0


const char *pWifiSsId       = WIFI_SSID;
const char *pWifiPass       = WIFI_PSK;
const char *pMqttClientId   = MQTT_CLIENT_ID;
const char *pMqttBrokerIp   = MQTT_BROKER_IP;
uint16_t mqttBrokerPort     = MQTT_BROKER_PORT;

const char *pMqttTopics[] =  {
    "/h801/set/power",
    "/h801/set/color",
    "/h801/set/brightness"
};
const char *pMqttAcks[] =  {
    "/h801/set/powerack",
    "/h801/set/colorack",
    "/h801/set/brightnessack"
};

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


void H801_SetColor(int red, int green, int blue)
{
    analogWrite(H801_RED_PIN, red * 4);
    analogWrite(H801_GREEN_PIN, green * 4);
    analogWrite(H801_BLUE_PIN, blue * 4);
}

void H801_SetBrightness(int brightness)
{
    analogWrite(H801_W1_PIN, brightness * 4);
    analogWrite(H801_W2_PIN, brightness * 4);
}

/**
 * MQTT callback to process messages
 */
void Mqtt_Handler(char *pTopic, byte *pPayload, unsigned int length) {
    char aPayload[10] = {0};
    unsigned int i;
    String newStr;
    Serial1.print(pTopic);
    Serial1.print(": ");

    for (i=0;i<length;i++) {
        aPayload[i] = (uint8_t)pPayload[i];
        Serial1.print((char)pPayload[i]);
    }
    aPayload[i] = 0;
    Serial1.println();
    newStr = String(aPayload);

    if(!strcmp(pTopic, pMqttTopics[0]))
    {
        if (newStr == "ON") {
            mqttClient.publish(pMqttAcks[0], "ON");
            Serial1.println("ON");
        }
        else if (newStr == "OFF") {
            mqttClient.publish(pMqttAcks[0], "OFF");

            H801_SetBrightness(0);
            H801_SetColor(0, 0, 0);
            Serial1.println("OFF");
        }
    }
    else if (!strcmp(pTopic, pMqttTopics[1])) {
        int red = newStr.substring(0, newStr.indexOf(',')).toInt();
        int green = newStr.substring(newStr.indexOf(',') + 1, newStr.lastIndexOf(',')).toInt();
        int blue = newStr.substring(newStr.lastIndexOf(',') + 1).toInt();

        H801_SetBrightness(0);
        H801_SetColor(red, green, blue);

        //mqttClient.publish(pMqttAcks[1], aPayload);
    }
    else if (!strcmp(pTopic, pMqttTopics[2])) {
        int brightness = newStr.toInt();

        //client.publish(pMqttTopics[0], "ON");

        H801_SetBrightness(brightness);
        H801_SetColor(0, 0, 0);

        Serial1.print("Set brightness to: ");
        Serial1.println(brightness);
    }
}

void WIFI_ConnectingHandler()
{
    Serial1.print(".");
    digitalWrite(H801_RED_D2_PIN, 0);
    delay(500);
    digitalWrite(H801_RED_D2_PIN, 1);
    delay(500);
}

void MQTT_ConnectionHandler(connStatus_t status)
{
    if (status == connStatusConnected_c)
    {
        Serial1.println("connected");
        digitalWrite(H801_GREEN_D1_PIN, 0);
    }
    else if (status == connStatusFailed_c)
    {
        digitalWrite(H801_GREEN_D1_PIN, 0);
        Serial1.print("failed, rc=");
        delay(500);
        digitalWrite(H801_GREEN_D1_PIN, 1);
        Serial1.print(mqttClient.state());
        Serial1.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
    }
}

void setup()
{
    pinMode(H801_GREEN_D1_PIN, OUTPUT);
    digitalWrite(H801_GREEN_D1_PIN, 1);
    pinMode(H801_RED_D2_PIN, OUTPUT);
    digitalWrite(H801_RED_D2_PIN, 1);

    pinMode(H801_RED_PIN, OUTPUT);
    pinMode(H801_GREEN_PIN, OUTPUT);
    pinMode(H801_BLUE_PIN, OUTPUT);
    pinMode(H801_W1_PIN, OUTPUT);
    pinMode(H801_W2_PIN, OUTPUT);

    // Setup console
    Serial1.begin(115200);
    delay(10);
    Serial1.println();
    Serial1.println();

    THA_WifiManage(pWifiSsId, pWifiPass, WIFI_ConnectingHandler);

    Serial1.println("WiFi connected");
    Serial1.println("IP address: ");
    Serial1.println(WiFi.localIP());

#if USE_OTA
    //WiFi.hostname(pMqttClientId);
    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp8266.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (!MDNS.begin(pMqttClientId)) {
        Serial1.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial1.println("mDNS responder started");

    /* Setup OTA */
    ArduinoOTA.setHostname(pMqttClientId);
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
        MQTT_ConnectionHandler);
#if USE_OTA
    ArduinoOTA.handle();
#endif
}
