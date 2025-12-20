#include <pinicore.hpp>
#include <Arduino.h>
#include <ArduinoHttpClient.h>

#include "secrets.hpp"

#include <PubSubClient.h>


#define TAG_MAIN    "main"

#define FIRMWARE_VERSION    666

#define USE_WIFI

#ifdef USE_WIFI
    WiFiComm wifi;
    INetwork* network = (INetwork*)&wifi;
#else
    MobileComm mobile;
    INetwork* network = (INetwork*)&mobile;
#endif

MQTT mqtt;
String topic_lwt;

//#define READ_SENSORS

#ifdef READ_SENSORS
String topic_temperatureDHT;
String topic_humidityDHT;
String topic_temperatureLM;
#endif // READ_SENSORS


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;      // adjust if needed
const int   daylightOffset_sec = 0; // adjust if needed

#ifdef READ_SENSORS
LM35 lm35;
DHT dht;
#endif // READ_SENSORS

#define UPDATE_VALUE    (15*1000)
#define UPLOAD_VALUE    (5*60*1000)
uint64_t lastUpdate = 0;
uint64_t lastUpload = 0;
bool connectUpload = false;

//#define TEST_RELAYS_TS

//RelaysVirtual rVirtual;
#ifdef TEST_RELAYS_TS
RelaysTS rTS;
#endif // TEST_RELAYS_TS
//RelaysX16Blue rBlue;

#define STORAGE_ID "PINI_TEST_CONTROLLER"
Storage storage;



#define LORA_TEST
#ifdef LORA_TEST
    LoRaTxRx lora;
    LoRaComm loraComm;
#endif



void MQTT_callback(const char *topic, const byte *payload, const unsigned int length) {
    LOG_I(TAG_MAIN, "Received: [topic: %s] [payload: %s] [length: %d]", topic, payload, length);
}

void setup() {
    Serial.begin(115200);
    Serial.println();   // Just to start on a new clean line

    LOG_I(TAG_MAIN, "Setup started");
    LOG_I(TAG_MAIN, "Firmware: [%d] | Build: [%s, %s]", FIRMWARE_VERSION, __DATE__, __TIME__);

    LOG_F(TAG_MAIN, "Fatal example");
    LOG_E(TAG_MAIN, "Error example");
    LOG_W(TAG_MAIN, "Warning example");
    LOG_I(TAG_MAIN, "Information example");
    LOG_D(TAG_MAIN, "Debug example");
    LOG_T(TAG_MAIN, "Trace example");

    storage.init(STORAGE_ID, sizeof(STORAGE_ID));

#ifdef USE_WIFI
    wifi.init();
    wifi.config(WIFI_SSID, WIFI_PASS);
#else
    mobile.init(23, 4, 5, 27, 26);
    mobile.config("", "");
#endif

    network->enable();
    network->connect();
    while (!network->isConnected()) {
        delay(1000);
    }
    LOG_I(TAG_MAIN, "connected = %d", network->isConnected());

    Client* client = network->getClient();

    /*
    HttpClient http(*client, "example.com");
    int error = http.get("/");
    int status  = http.responseStatusCode();
    int bodyLen = http.contentLength();
    String body = http.responseBody();
    LOG_D(TAG_MAIN, "Received: [status: %d] [body (%d): %s]", status, bodyLen, body.c_str());
    */

   topic_lwt = "irrigation/sectors/v0/"+String(getUniqueId())+"/up/lwt";
#ifdef READ_SENSORS
    topic_temperatureDHT = "irrigation/sectors/v0/"+String(getUniqueId())+"/up/sensors/temperature/0";
    topic_temperatureLM = "irrigation/sectors/v0/"+String(getUniqueId())+"/up/sensors/temperature/1";
    topic_humidityDHT = "irrigation/sectors/v0/"+String(getUniqueId())+"/up/sensors/humidity/0";
#endif // READ_SENSORS

    /*
    OTATS ota(client, FIRMWARE_VERSION, "s_");
    ota.setProgressCallback(
        /* onProgress * [](uint32_t downloadedBytes, uint32_t totalBytes) {
            LOG_T(TAG_MAIN, "DL: %6.02f %%", (100.0 * downloadedBytes) / totalBytes);
        }
    );
    ota.setCredentials(OTA_TS_USER, OTA_TS_PASS);
    ota.setCertificate(OTA_TS_SSL_CERTIFICATE);
    ota.checkUpdate();
    
    EOTAUpdateStatus statusOTA = ota.update();
    if (statusOTA == EOTAUpdateStatus::OTA_INSTALLED) {
        ESP.restart();
    }
    */

    mqtt.setClient(client, getUniqueId());
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCredentials(MQTT_USER, MQTT_PASS);
    mqtt.setWill(topic_lwt.c_str(), "0", 2, true);

    mqtt.onTopic(
        "pinicore/down/test",
        [](const char* payload, uint32_t length) {
            LOG_I(TAG_MAIN, "TEST topic: [payload: '%s'] [length: %d]", payload, length);
        }
    );

    mqtt.onConnect(
        []() {
            mqtt.publish(topic_lwt.c_str(), "1", true);
            connectUpload = true;
        }
    );
    mqtt.connect();

#ifdef READ_SENSORS
    lm35.init(34, 2.0f);    // The one I have seems to be reading -2ºC below the real expected value
    dht.init(22, EDHT::DHT_11);
#endif // READ_SENSORS

    /*
    rVirtual.setModules(2, 4);
    rVirtual.init();
    LOG_D(TAG_MAIN, "Virtual relays start");
    rVirtual.getActiveCount();
    LOG_D(TAG_MAIN, "Virtual [0:1] true");
    rVirtual.set(0, 1, true);
    rVirtual.getActiveCount();
    LOG_D(TAG_MAIN, "Virtual [0:3] true");
    rVirtual.set(0, 3, true);
    rVirtual.getActiveCount();

    LOG_D(TAG_MAIN, "Virtual [0:5] true");
    rVirtual.set(0, 5, true);
    rVirtual.getActiveCount();

    LOG_D(TAG_MAIN, "Virtual [1:0] true");
    rVirtual.set(1, 0, true);
    rVirtual.getActiveCount();
    */
#ifdef TEST_RELAYS_TS
    rTS.init();
    for (int i=0; i<rTS.getRelaysPerModule(); ++i) {
        rTS.set(0, i, true);
        delay(500);
    }
#endif // TEST_RELAYS_TS

#ifdef LORA_TEST
    lora.init(27,19,5,18,23,26, 864);
    lora.setSpreadingFactor(7);
    lora.setTxPower(20);
    lora.setBandwidth(ELoRaBandwidth::LR_BW_125_KHZ);
    lora.onReceive(
        [](const uint8_t* payload, size_t size, int rssi, float snr) {
            LOG_D(TAG_MAIN, "size=%d rssi=%d snr=%f", size, rssi, snr);
            Serial.print("Payload: ");
            for (size_t i = 0; i < size; ++i) {
                Serial.print(payload[i], HEX);
                Serial.print(' ');
            }
            Serial.println();
        }
    );
    //
    uint8_t payload[4] = { 0xFF, 0XAB, 0X12, 0X34 };
    lora.send(payload, 4);
#endif

    LOG_I(TAG_MAIN, "Setup completed");
}

uint64_t loraSend = 0;
bool set = false;
#ifdef READ_SENSORS
int temp0Value;
int hum0Value;
int temp1Value;
#endif // READ_SENSORS
void loop() {
    network->maintain();
    mqtt.maintain();

#ifdef LORA_TEST
    lora.maintain();
    /*
    if (loraSend + 10000 < getMillis()) {
        uint8_t payload[4] = { 0xFF, 0XAB, 0X12, 0X34 };
        lora.send(payload, 4);
        loraSend = getMillis();
    }
    */
#endif

#ifdef TEST_RELAYS_TS
    if (rTS.isModuleConnected(1)) {
        if (!set) {
            for (int i=0; i<rTS.getRelaysPerModule(); ++i) {
                rTS.set(1, i, true);
                delay(500);
            }
            set = true;
        }
    }
    else {
        set = false;
    }
#endif // TEST_RELAYS_TS

#ifdef READ_SENSORS
    temp0Value = dht.readTemperature();
    hum0Value  = dht.readHumidity();
    temp1Value = lm35.readTemperature();
    
    if (mqtt.isConnected()) {
        if (lastUpload + UPLOAD_VALUE < getMillis() || connectUpload) {
            mqtt.publish(topic_temperatureDHT.c_str(), String(temp0Value).c_str(), false);
            mqtt.publish(topic_humidityDHT.c_str(), String(hum0Value).c_str(), false);
            mqtt.publish(topic_temperatureLM.c_str(), String(temp1Value).c_str(), false);
            lastUpload = getMillis();
            connectUpload = false;
        }
    }
#endif // READ_SENSORS
}
