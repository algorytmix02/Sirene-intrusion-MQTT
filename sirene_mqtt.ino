#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Remplacez les informations ci-dessous par celles de votre réseau Wi-Fi et du broker MQTT
const char* ssid = "TON SSID";
const char* password = "TON MDP WIFI";
const char* mqtt_server = "IP DE MOSQUITTO";
const int mqtt_port = 1883; // Port MQTT standard

bool relayState = false;
unsigned long relayStartTime = 0;
const unsigned long relayDuration =3*60*1000;// 3 minutes en millisecondes  (1.5 min en appartement !)
// Remplacez les informations ci-dessous par celles de votre broker MQTT
const char* mqtt_user = "jeedom";
const char* mqtt_password = "jeedom";
const char* relayetat_topic = "Garage/sireneetat";
const char* relay_topic = "Garage/sirene";
const char* input1_topic = "Garage/Porte1";
const char* input2_topic = "Garage/Porte2";
const char* client_id = "wemos-alarme-Garage";

WiFiClient espClient;
PubSubClient client(espClient);

const int relayPin = D1; // Broche sur laquelle est connecté le relais
const int input1Pin = D6; // Broche sur laquelle est connectée l'entrée 1 (D2 avec résistance de pull-up)
const int input2Pin = D7; // Broche sur laquelle est connectée l'entrée 2 (D3 avec résistance de pull-up)
const int LED = D4; // Broche sur laquelle est connectée l'entrée 1 (D2 avec résistance de pull-up)
void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
 digitalWrite(relayPin, LOW);
 pinMode(LED, OUTPUT);
  pinMode(input1Pin, INPUT_PULLUP);
  pinMode(input2Pin, INPUT_PULLUP);
  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()  {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
 // Vérifier si l'état de l'entrée 1 a changé
  static int prevInput1State = HIGH;
  int input1State = digitalRead(input1Pin);
  if (input1State != prevInput1State) {
    prevInput1State = input1State;
    // Envoyer l'état de l'entrée 1 via MQTT
    if (input1State == LOW) {
      client.publish(input1_topic, "1");
    } else {
      client.publish(input1_topic, "0");
    }
  }

  // Vérifier si l'état de l'entrée 2 a changé
  static int prevInput2State = HIGH;
  int input2State = digitalRead(input2Pin);
  if (input2State != prevInput2State) {
    prevInput2State = input2State;
    // Envoyer l'état de l'entrée 2 via MQTT
    if (input2State == LOW) {
      client.publish(input2_topic, "1");
    } else {
      client.publish(input2_topic, "0");
    }
  }
  // Vérifier si le minuteur est écoulé pour éteindre automatiquement le relais
  if (relayState && millis() - relayStartTime >= relayDuration) {
    digitalWrite(relayPin, LOW);
     client.publish(relayetat_topic,"0");
    relayState = false;
    Serial.println("Relais éteint automatiquement après 10 secondes !");
  }
}

void connectWiFi() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connexion à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.print("Adresse IP: ");
  digitalWrite(LED, HIGH);
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == String(relay_topic)) {
    if (message == "ON") {
      // Allumer le relais et enregistrer l'heure de démarrage
      digitalWrite(relayPin, HIGH);
      client.publish(relayetat_topic,"1");
      relayState = true;
      relayStartTime = millis();
      Serial.println("Relais allumé !");
    } else if (message == "OFF") {
      // Éteindre le relais immédiatement, peu importe l'état du minuteur
      digitalWrite(relayPin, LOW);
       client.publish(relayetat_topic,"0");
      relayState = false;
      Serial.println("Relais éteint !");
    }
  }
}
void reconnect() {
  while (!client.connected()) {
    Serial.println("Connexion MQTT...");
    if (client.connect(client_id, mqtt_user, mqtt_password)) {
      Serial.println("Connecté au broker MQTT !");
      digitalWrite(LED, HIGH);
      client.subscribe(relay_topic);
    } else {
      Serial.print("Échec, rc=");
      Serial.print(client.state());
      Serial.println(" Réessayez dans 5 secondes...");
      delay(5000);
    }
  }
}
