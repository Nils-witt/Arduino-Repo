#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "secrets.h"

const char* ssid = WLAN_SSID;
const char* password = WLAN_PASS;
const char* mqtt_server = MQTT_HOST;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;
String hostname = "ESP-garage";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

int openReed = D5;
int closeReed = D6;

int relay1 = D0;
int relay2 = D1;
int relay3 = D2;
int relay4 = D4;

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
        openGate();
      } else if (message == "CLOSED") {
        closeGate();
      }
    } else if (topic == "devices/esp-garage/valve1/TargetState") {
      Serial.print("CONTROL");
      if (message == "true") {
        digitalWrite(relay1 , LOW);
        client.publish("devices/esp-garage/valve1/CurrentState", "OPEN");
      } else if (message == "false") {
        digitalWrite(relay1 , HIGH);
        client.publish("devices/esp-garage/valve1/CurrentState", "CLOSED");
      }
    } else if (topic == "devices/esp-garage/valve2/TargetState") {
      Serial.print("CONTROL");
      if (message == "true") {
        digitalWrite(relay1 , LOW);
        client.publish("devices/esp-garage/valve2/CurrentState", "OPEN");
      } else if (message == "false") {
        digitalWrite(relay1 , HIGH);
        client.publish("devices/esp-garage/valve2/CurrentState", "CLOSED");
      }
    } else if (topic == "devices/esp-garage/valve3/TargetState") {
      Serial.print("CONTROL");
      if (message == "true") {
        digitalWrite(relay1 , LOW);
        client.publish("devices/esp-garage/valve3/CurrentState", "OPEN");
      } else if (message == "false") {
        digitalWrite(relay1 , HIGH);
        client.publish("devices/esp-garage/valve3/CurrentState", "CLOSED");
      }
    } else {
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
      client.subscribe("devices/esp-garage/gate/TargetState");
      client.subscribe("devices/esp-garage/valve1/TargetState");
      client.subscribe("devices/esp-garage/valve2/TargetState");
      client.subscribe("devices/esp-garage/valve3/TargetState");
      //PUBS
      client.publish("devices/esp-garage/Online", "true");

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
    } else { // U_FS
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

  pinMode(openReed, INPUT);
  pinMode(closeReed, INPUT);

  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, HIGH);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay2, HIGH);
  pinMode(relay3, OUTPUT);
  digitalWrite(relay3, HIGH);
  pinMode(relay4, OUTPUT);
  digitalWrite(relay4, HIGH);

}

void triggerGate() {
  if (currentState == "OPEN" || currentState == "CLOSED") {
    digitalWrite(relay4 , LOW);
    delay(1000);
    digitalWrite(relay4 , HIGH);
  }
}

void openGate() {
  triggerGate();
}

void closeGate() {
  triggerGate();
}

void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int openReedReadout = digitalRead(openReed);
  int closeReedReadout = digitalRead(closeReed);
  String tmpState = currentState;
  if (closeReedReadout == HIGH) {
    tmpState = "CLOSED";
  } else if (openReedReadout == HIGH) {
    tmpState = "OPEN";
  } else {
    if (closeReedReadout == LOW) {
      if (currentState == "CLOSED") {
        tmpState = "OPENING";
      }
    }

    if (openReedReadout == LOW) {
      if (currentState == "OPEN") {
        tmpState = "CLOSING";
      }
    }
  }

  if (currentState != tmpState) {
    currentState = tmpState;
    client.publish("devices/esp-garage/gate/CurrentState", currentState.c_str());
    lastPub = millis();
    stateChangeTimer = millis();
    isObstructed = "false";
  }

  if (currentState == "OPENING" || currentState == "CLOSING") {
    if (millis() - stateChangeTimer >= 30 * 1000UL) {
      client.publish("devices/esp-garage/gate/CurrentState", currentState.c_str());
      isObstructed = "true";
    }
  }


  if (millis() - lastPub >= 10 * 1000UL) {
    client.publish("devices/esp-garage/gate/CurrentState", currentState.c_str());
    lastPub = millis();
  }
}
