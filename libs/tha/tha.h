#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#if USE_SERIAL_COM
#   define Serial               Serial
#   define SERIAL_INIT()        Serial.begin(115200);\
                                delay(10);
#   define SERIAL_PRINT(x)      Serial.print(x)
#   define SERIAL_PRINTLN(...)  Serial.println(__VA_ARGS__)
#   define SERIAL_PRINTF(...)   Serial.printf(__VA_ARGS__)
#elif USE_SERIAL_COM1
#   define Serial               Serial1
#   define SERIAL_INIT()        Serial1.begin(115200);\
                                delay(10);
#   define SERIAL_PRINT(x)      Serial1.print(x)
#   define SERIAL_PRINTLN(...)  Serial1.println(__VA_ARGS__)
#   define SERIAL_PRINTF(...)   Serial1.printf(__VA_ARGS__)
#else
#   define Serial
#   define SERIAL_INIT()
#   define SERIAL_PRINT(x)
#   define SERIAL_PRINTLN(...)
#   define SERIAL_PRINTF(...)
#endif

#define UNUSED_SENSOR_READING   (-10000.0f)
#define NumberOfElements(arr)   (sizeof(arr)/sizeof(arr[0]))


typedef enum connectionStatus_tag
{
    connStatusConnecting_c,
    connStatusConnected_c,
    connStatusFailed_c,
} connStatus_t;

typedef struct sensorData_tag
{
    float temperature;
    float humidity;
    float pressure;
    float heatIndex;
} sensorData_t;

typedef void(thaMqttCb_t)(char *pTopic, uint8_t *pPayload, unsigned int length);
typedef void(thaWifiConnectingCb_t)();
typedef void(thaMqttConnStatusCb_t)(connStatus_t status);


void THA_WifiManage(const char *pSsid, const char *pPass, thaWifiConnectingCb_t pConnectingCb);
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
);

void THA_PublishSensorData(
    PubSubClient &mqttClient,
    const char *pTopic,
    sensorData_t *pSensorData
);
