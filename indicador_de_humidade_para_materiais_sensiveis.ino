#include "WiFi.h"
#include "WiFiClient.h"
#include "PubSubClient.h"
#include "DHT.h"

#define PINO_WIFI_LED 2
#define DHTPIN 12     
#define DHTTYPE DHT11 


DHT dht(DHTPIN, DHTTYPE);

void connectWiFi();
void connectMQTT();


WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

const char* wifi_ssid = "NPITI-IoT";
const char* wifi_password = "NPITI-IoT";

//const char* wifi_ssid = "Gabriel";
//const char* wifi_password = "abacate1";

int wifi_timeout = 100000;

const char* mqtt_broker = "io.adafruit.com";
const int mqtt_port = 1883;
int mqtt_timeout = 10000;

const char* mqtt_usernameAdafruitIO = "gabriel26077";
const char* mqtt_keyAdafruitIO = "aio_lyRh94s9kfzhbW4kCBoVWCkbU5bJ";

int valor = 0;


float min_t = 0;
float max_t = 50; 

float min_u = 30;
float max_u = 60; 

float ideal_t = (max_t + min_t)/2; 
float ideal_u = (max_u + min_u)/2;

float raio_t = (max_t - min_t)/2; 
float raio_u = (max_u - min_u)/2;



bool isDanger(float observed_t, float observed_u);//Modelo "matemático" para classificar

void setup() {
  
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));
  //Serial.println(("DHTxx test!"));
  dht.begin();

  //Conectando no wifi
  WiFi.mode(WIFI_STA); 
  WiFi.begin(wifi_ssid, wifi_password);
  connectWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
}

void loop() {
 
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print("\n");

  bool danger = isDanger(t,h);

  /*
  Esse código não é mais necessário, em tese.
  if(h > 50 || t > 25){
    perigo = true;
  }else{
    
  }
  */

  if (!mqtt_client.connected()) { // Se MQTT não estiver conectado
    connectMQTT();
  }

  if (mqtt_client.connected()) {

    if(h >= 20 && h <= 90 && t >= 0 && t <= 50){//tratamento de dados
      mqtt_client.publish("gabriel26077/feeds/umidade", String(h).c_str());
      mqtt_client.publish("gabriel26077/feeds/temperatura", String(t).c_str());
      mqtt_client.publish("gabriel26077/feeds/corrosao", String(danger).c_str());

      Serial.printf("Publicou os dado:\n\tUmidade: %s\n\tTemperatura: %s\n\tCorrosao: %s\n", String(h), String(t), String(danger));

    }

    delay(10000);
    mqtt_client.loop();
  }

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);


}

void connectWiFi() {
  Serial.print("Conectando à rede WiFi ... ");

  unsigned long tempoInicial = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - tempoInicial < wifi_timeout)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conexão com WiFi falhou!");
    digitalWrite(PINO_WIFI_LED, LOW); //Garante que o LED interno está desligado indicando que o wifi está desconectado.
  } else {
    Serial.print("Conectado com o IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(PINO_WIFI_LED, HIGH); //Ascende o LED interno indicando que o wifi está conectado
  }
}

void connectMQTT() {
  unsigned long tempoInicial = millis();
  while (!mqtt_client.connected() && (millis() - tempoInicial < mqtt_timeout)) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
    Serial.print("Conectando ao MQTT Broker..");

    if (mqtt_client.connect("ESP32Client", mqtt_usernameAdafruitIO, mqtt_keyAdafruitIO)) {
      Serial.println();
      Serial.print("Conectado ao broker MQTT!");
    } else {
      Serial.println();
      Serial.print("Conexão com o broker MQTT falhou!\n");
      delay(500);
    }
  }
  Serial.println();
}



bool isDanger(float observed_t, float observed_u){

  float delta_t = pow((observed_t - ideal_t)/raio_t,2);
  float delta_u = pow((observed_u - ideal_u)/raio_u,2);

  return delta_t + delta_u > 1;
  
}

