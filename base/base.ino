#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "secrets.h"
// Update these with values suitable for your network.

const char* ssid = WLAN_SSID;
const char* password = WLAN_PASS;
const char* mqtt_server = MQTT_HOST;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;
String hostname = "ESP-demo";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


int relay1 = D0;


int openState = LOW;
int closedState = LOW;

String currentState = "";
int lastPub = 0;
int stateChangeTimer = 0;
String isObstructed = "false";

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname.c_str());

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topicArr, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  String topic = topicArr;
  Serial.println(topic);
  if (topic == "devices/esp-garage/ping") {
    client.publish("devices/esp-garage/pong", "pong");
  } else {
    Serial.print("Message:");
    String message;
    for (int i = 0; i < length; i++) {
      message = message + (char) payload[i];  // convert *byte to string
    }
    Serial.print(message);
    char* msg;
    message.toCharArray(msg, sizeof(message));

    if (topic == "devices/esp-garage/gate/TargetState") {
      Serial.print("CONTROL");
      if (message == "OPEN") {
        //
      }
    }else {
      Serial.println("Other topic :-" + topic + "-");
    }

  }

  Serial.println();
  Serial.println("-----------------------");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("MQTT connected");

      //SUBS
      client.subscribe("devices/esp-garage/ping");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);

  setup_wifi();
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}


void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
