#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#if USE_JSON
#include <ArduinoJson.h>
#endif

#include "tha.h"

char aOutData[200] = {0};

/*
 * Handle WIFI connection
 *  */
void THA_WifiManage(const char *pSsid, const char *pPass, thaWifiConnectingCb_t pConnectingCb)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(pSsid, pPass);

        while (WiFi.status() != WL_CONNECTED) {
            if (pConnectingCb)
            {
                pConnectingCb();
            }
            else
            {
                delay(500);
            }
        }
    }
}

/*
 * Handle MQTT connection
 *  */
void THA_MQTTManage
(
    PubSubClient &mqttClient,
    const char *pMqttClientId,
    const char *pMqttBrokerIp,
    uint16_t mqttBrokerPort,
    const char *pMqttUser,
    const char *pMqttPass,
    thaMqttCb_t callback,
    const char *pMqttStateTopics[],
    uint8_t cTopics,
    thaMqttConnStatusCb_t pConnStatus
)
{
    // Loop until we're reconnected
    if (!mqttClient.connected()) {

        mqttClient.setServer(pMqttBrokerIp, mqttBrokerPort);
        mqttClient.setCallback(callback);

        // Attempt to connect
        if (mqttClient.connect(pMqttClientId)) {
            if (pConnStatus)
            {
                pConnStatus(connStatusConnected_c);
            }
            for(uint8_t idx=0;idx<cTopics;idx++)
            {
                mqttClient.subscribe(pMqttStateTopics[idx]);
            }
        } else {
            if (pConnStatus)
            {
                pConnStatus(connStatusFailed_c);
            }
        }
    }
    if (mqttClient.connected())
    {
        mqttClient.loop();
    }
}

void THA_PublishSensorData(
    PubSubClient &mqttClient,
    const char *pTopic,
    sensorData_t *pSensorData
)
{
#if USE_JSON
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    // INFO: the data must be converted into a string; a problem occurs when using floats...

    root["t"] = (String)pSensorData->temperature;
    root["h"] = (String)pSensorData->humidity;
    root["p"] = (String)pSensorData->pressure;
    root["hi"] = (String)pSensorData->heatIndex;

    //root.prettyPrintTo(SERIAL);
    SERIAL_PRINTLN("");

    root.printTo(aOutData, root.measureLength() + 1);
    mqttClient.publish(pTopic, aOutData, true);
#endif
}
