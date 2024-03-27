// Protocolo mqtt

// ============ BIBLIOTECAS EXTERNAS ==============
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ========== CONFIGURAÇÃO O MQTT =============

// Endereço IP do broker mqtt
char server[] = {"iot.eclipse.org"};
int port = 1883
char topic[] = {"codifythings/testMessage"};


// ========== INICIALIZAÇÃO DO MQTT E FUNÇÃO CALLBACK =============

PubSubClient pubSubClient(server, 1883, callback, client);

void callback(char* topic, byte* payload, unsigned int length) {
  
  // Exibe o payload 
  String payloadContent = String((char*) payload);
  Serial.println("[INFO] Payload: " + payloadContent);
}


// ========== CÓDIGO PARA AS FUNÇÕES-PADRÃO DO ARDUINO ============

void setup() {
  
  // Inicia a porta serial
  Serial.begin(9600);

  // Conecta o Arduino à Internet
  connectToInternet();

  // Conecta-se ao broker mqtt
  Serial.println("[INFO] Connecting to MQTT Broker");

  if(pubSubClient.connect("arduinoClient")) {
    Serial.println("[INFO] Connection to MQTT Broker Successful");

    pubSubClient.subscribe(topic);
    Serial.println("[INFO] Successfully Subscribed to MQTT Topic");

    Serial.println("[INFO] Publishing to MQTT Broker");
    pubSubClient.publish(topic, "Test Message");

  } else {
    Serial.println("[INFO] Connection to MQTT Broker Failed");
  }

}


void loop() {

  // Aguarda mensagens do Broker MQTT
  pubSubClient.loop();
}


