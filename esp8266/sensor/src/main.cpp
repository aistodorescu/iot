#include <Arduino.h>
#include <Wire.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>

#include <ArduinoJson.h>

/* Global features */
#define USE_SERIAL_COM          (0)

/* Libraries */
#include "tha.h"
#include "utils.h"


#define USE_OTA                 (1)
#define DHT22_PRESENT           (0)
#define DS18B20_PRESENT         (0)
#define BME280_PRESENT          (1)
#define PIR_PRESENT             (0)

#define DHTPIN                  (2)
#define DHTTYPE                 DHT22

#define DS18B20PIN              (0)

#define PIR_PIN                 (ESP01_RX_PIN)

#define LED_BLUE_PIN            (ESP01_TX_PIN)

#define BME280_SDA              (0)
#define BME280_SCL              (2)

#define SENSOR_DATA_TIMEOUT_S   (5 * 60)
//#define SENSOR_DATA_TIMEOUT_S   (10)
#define NUM_SAMPLES             (10)


const char *pWifiSsId       = WIFI_SSID;
const char *pWifiPass       = WIFI_PSK;
const char *pMqttClientId   = MQTT_CLIENT_ID;
const char *pMqttBrokerIp   = MQTT_BROKER_IP;
uint16_t mqttBrokerPort     = MQTT_BROKER_PORT;
const char *pMqttTopics[] =  {
    "...",
};
const char *pMqttAcks[] =  {
    "/esp01/monitor1",
};


WiFiClient wificlient;
PubSubClient mqttClient(wificlient);

#if DHT22_PRESENT
DHT dht(DHTPIN, DHTTYPE);
#endif /* DHT22_PRESENT */
#if DS18B20_PRESENT
OneWire oneWire(DS18B20PIN);
DallasTemperature ds18b20(&oneWire);
#endif /* DS18B20_PRESENT */
#if BME280_PRESENT
Adafruit_BME280 bme;
#endif /* BME280_PRESENT */

Ticker ticker;
volatile int mThaNeedsProcess = 0;
int lastState = 0;
int curentState = 0;
volatile bool mTickOccured = true;

float aRawTempData[NUM_SAMPLES] = {NAN};
float aRawHumData[NUM_SAMPLES] = {NAN};
float aRawPressureData[NUM_SAMPLES] = {NAN};

void WIFI_ConnectingHandler()
{
    SERIAL_PRINT(".");
    //digitalWrite(LED_BLUE_PIN, 0);
    delay(500);
    //digitalWrite(LED_BLUE_PIN, 1);
    delay(500);
}

void THA_Process(PubSubClient &mqttClient)
{
    float tempValue = UNUSED_SENSOR_READING;
    float humValue = UNUSED_SENSOR_READING;
    float pressureValue = UNUSED_SENSOR_READING;
    sensorData_t sensorData;
    bool valid = true;
    uint32_t idxData = 0;

    /* Get multiple samples samples */
    for (idxData=0;idxData<NUM_SAMPLES;idxData++)
    {
        do
        {
#if DHT22_PRESENT
            tempValue = dht.readTemperature();
            humValue = dht.readHumidity();
#endif /* DHT22_PRESENT */
#if DS18B20_PRESENT
            ds18b20.requestTemperatures();
            tempValue = ds18b20.getTempCByIndex(0);
#endif /* DS18B20_PRESENT */
#if BME280_PRESENT
            tempValue = bme.readTemperature();
            humValue = bme.readHumidity();
            pressureValue = bme.readPressure() * 0.00750; /* P[mmHg] = P[Pa] x 0.0075 */
#endif /* BME280_PRESENT */

            /* Check if values are valid */
            if (tempValue == NAN) {
                valid = false;
            }
            if (humValue == NAN) {
                valid = false;
            }
            if (pressureValue == NAN) {
                valid = false;
            }
        } while (valid == false);
        aRawTempData[idxData] = tempValue;
        aRawHumData[idxData] = humValue;
        aRawPressureData[idxData] = pressureValue;
#if DHT22_PRESENT
        delay(2000);
#endif
    }

    if (tempValue != UNUSED_SENSOR_READING)
    {
        sensorData.temperature = UTILS_GetFilteredValueInArray(aRawTempData, NUM_SAMPLES);
    }

    if (humValue != UNUSED_SENSOR_READING)
    {
        sensorData.humidity = UTILS_GetFilteredValueInArray(aRawHumData, NUM_SAMPLES);
    }

    if (pressureValue != UNUSED_SENSOR_READING)
    {
        sensorData.pressure = UTILS_GetFilteredValueInArray(aRawPressureData, NUM_SAMPLES);
    }

    sensorData.heatIndex = UTILS_ComputeHeatIndex(sensorData.temperature,
        sensorData.humidity, false);

    THA_PublishSensorData(mqttClient, pMqttAcks[0], &sensorData);
}

void Tick_Timeout()
{
    mTickOccured = true;
}

void setup() {
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
    ArduinoOTA.onStart([]() {
        SERIAL_PRINTLN("Start OTA");
    });
    ArduinoOTA.onEnd([]() {
        SERIAL_PRINTLN("End OTA");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        SERIAL_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
        mqttClient.publish("/tha/log", pMqttClientId);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        SERIAL_PRINTF("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) SERIAL_PRINTLN("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) SERIAL_PRINTLN("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) SERIAL_PRINTLN("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) SERIAL_PRINTLN("Receive Failed");
        else if (error == OTA_END_ERROR) SERIAL_PRINTLN("End Failed");
    });

    ArduinoOTA.begin();
#endif

    /* Setup sensors */
#if DHT22_PRESENT
    dht.begin();
#endif
#if DS18B20_PRESENT
    ds18b20.begin();
#endif
#if BME280_PRESENT
    Wire.begin(BME280_SDA, BME280_SCL);
    Wire.setClock(100000);
    if (!bme.begin(0x76)) {
        SERIAL_PRINTLN("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
#endif
#if PIR_PRESENT
    pinMode(PIR_PIN, INPUT_PULLUP);
    pinMode(LED_BLUE_PIN, OUTPUT);
#endif
}

void loop() {

    THA_WifiManage(pWifiSsId, pWifiPass, WIFI_ConnectingHandler);
    THA_MQTTManage(
        mqttClient,
        pMqttClientId,
        pMqttBrokerIp,
        mqttBrokerPort,
        NULL,
        NULL,
        NULL,
        pMqttTopics,
        0,
        NULL);
    ArduinoOTA.handle();

#if PIR_PRESENT
/* PIR */
    curentState = digitalRead(PIR_PIN);
    if((curentState == 1) && (lastState == 0))
    {
        SERIAL_PRINTLN("Motion detected");
        mqttClient.publish(APP_PIR_TOPIC, "ON");
        //delay(1*1000);
        digitalWrite(LED_BLUE_PIN, 0);
    }
    else if((curentState == 0) && (lastState == 1))
    {
        SERIAL_PRINTLN("Stop motion");
        mqttClient.publish(APP_PIR_TOPIC, "OFF");
        digitalWrite(LED_BLUE_PIN, 1);
        //delay(1*1000);
    }
    lastState = curentState;
#endif

    if (mTickOccured)
    {
        mTickOccured = false;
        THA_Process(mqttClient);
        ticker.once(SENSOR_DATA_TIMEOUT_S, Tick_Timeout);
    }
}
