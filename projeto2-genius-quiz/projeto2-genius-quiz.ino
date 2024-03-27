// Jogo Genius Quiz

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// =================================================================================================================================================

// Definindo as notas para os sons
#define NOTE_D4  294
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_A5  880

// =================================================================================================================================================

// Informações de rede
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Endereço MAC arbitrário
IPAddress ip(192, 168, 1, 20); // Endereço IP do Arduino
IPAddress mqtt_server(192, 168, 1, 100); // Endereço IP do servidor MQTT
const int mqtt_port = 1883; // Porta padrão para o protocolo MQTT

EthernetClient ethClient;
PubSubClient client(ethClient);

// =================================================================================================================================================

// Variáveis do jogo
int tons[4] = { NOTE_A5, NOTE_A4, NOTE_G4, NOTE_D4 };
int sequencia[100] = {};
int rodada_atual = 0;
int passo_atual_na_sequencia = 0;
int pinoAudio = 12;
int pinosLeds[4] = { 2, 4, 6, 8 };
int pinosBotoes[4] = { 3, 5, 7, 9 };
int botao_pressionado = 0;
bool perdeu_o_jogo = false;
int pontos_acumulados = 0;

// =================================================================================================================================================

// FUNÇÃO CALLBACK:  Esta função é chamada quando uma mensagem MQTT é recebida. 
// Atualmente, não está sendo utilizada no código, mas pode ser usada para processar 
// mensagens recebidas, caso necessário.

void callback(char* topic, byte* payload, unsigned int length) {
  // Lidar com mensagens recebidas (se necessário)
}

// =================================================================================================================================================

// FUNÇÃO RECONNECT: Esta função é responsável por reconectar o cliente MQTT ao broker MQTT, 
// caso a conexão seja perdida. Ela tenta reconectar a cada 5 segundos até conseguir uma conexão bem-sucedida.

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando se conectar ao MQTT Broker...");
    if (client.connect("ArduinoClient")) {
      Serial.println("Conectado!");
      client.subscribe("jogo/genius");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando reconectar em 5 segundos...");
      delay(5000);
    }
  }
}

// =================================================================================================================================================

// FUNÇÃO SETUP: Esta função é chamada uma vez ao inicializar o Arduino. Nela, são configuradas as 
// conexões de rede, os pinos dos LEDs e botões, além da conexão com o servidor MQTT.

void setup() {
  Serial.begin(115200);
  Ethernet.begin(mac, ip);
  while (!Ethernet.ready());
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  for (int i = 0; i <= 3; i++) {
    pinMode(pinosLeds[i], OUTPUT);
  }

  for (int i = 0; i <= 3; i++) {
    pinMode(pinosBotoes[i], INPUT_PULLUP);
  }

  pinMode(pinoAudio, OUTPUT);
  randomSeed(analogRead(0));
}

// =================================================================================================================================================

// FUNÇÃO LOOP: Esta é a função principal do código, que é executada repetidamente. Aqui, é verificado se o cliente MQTT está conectado e, 
// se não estiver, é feita uma tentativa de reconexão. Em seguida, o jogo é executado, com as seguintes etapas: inicialização 
// da próxima rodada, reprodução da sequência de sons, espera pelo jogador e envio dos pontos acumulados.

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (perdeu_o_jogo) {
    int sequencia[100] = {};
    rodada_atual = 0;
    passo_atual_na_sequencia = 0;
    perdeu_o_jogo = false;
  }

  if (rodada_atual == 0) {
    tocarSomDeInicio();
    delay(500);
  }

  proximaRodada();
  reproduzirSequencia();
  aguardarJogador();
  enviarPontos(); // Envio dos pontos acumulados para o MQTT
  delay(1000);
}

// =================================================================================================================================================

// FUNÇÃO DE PRÓXIMA RODADA: Esta função sorteia um novo número (representando um som) e adiciona à sequência do jogo para a próxima rodada.

void proximaRodada() {
  int numero_sorteado = random(0, 4);
  sequencia[rodada_atual++] = numero_sorteado;
}

// =================================================================================================================================================

// FUNÇÃO PARA REPETIR A SEQUÊNCIA DE LUZES: Esta função reproduz a sequência de sons correspondentes aos botões que o jogador precisa pressionar.

void reproduzirSequencia() {
  for (int i = 0; i < rodada_atual; i++) {
    tone(pinoAudio, tons[sequencia[i]]);
    digitalWrite(pinosLeds[sequencia[i]], HIGH);
    delay(500);
    noTone(pinoAudio);
    digitalWrite(pinosLeds[sequencia[i]], LOW);
    delay(100);
  }
  noTone(pinoAudio);
}

// =================================================================================================================================================

// FUNÇÃO QUE AGUARDA O JOGADOR PRESSIONAR OS BOTÕES: Esta função espera o jogador pressionar os botões correspondentes 
// à sequência de sons que foi reproduzida. Ela também verifica se o jogador pressionou o botão correto e atualiza os 
// pontos acumulados.

void aguardarJogador() {
  for (int i = 0; i < rodada_atual; i++) {
    aguardarJogada();
    if (sequencia[passo_atual_na_sequencia] != botao_pressionado) {
      gameOver();
    } else {
      pontos_acumulados++; // Incrementa os pontos acumulados
    }
    if (perdeu_o_jogo) {
      break;
    }
    passo_atual_na_sequencia++;
  }
  passo_atual_na_sequencia = 0;
}

// =================================================================================================================================================

// FUNÇÃO QUE AGUARDA A JOGADA DO JOGADOR:  Esta função espera até que o jogador pressione um botão para fazer sua jogada. 
// Ela verifica constantemente o estado dos botões até que um deles seja pressionado.

void aguardarJogada() {
  boolean jogada_efetuada = false;
  while (!jogada_efetuada) {
    for (int i = 0; i <= 3; i++) {
      if (!digitalRead(pinosBotoes[i])) {
        botao_pressionado = i;
        tone(pinoAudio, tons[i]);
        digitalWrite(pinosLeds[i], HIGH);
        delay(300);
        digitalWrite(pinosLeds[i], LOW);
        noTone(pinoAudio);
        jogada_efetuada = true;
      }
    }
    delay(10);
  }
}

// =================================================================================================================================================

// FUNÇÃO DE FIM DE JOGO - O JOGADOR PERDEU: Esta função é chamada quando o jogador perde o jogo. Ela reproduz uma 
// sequência de sons e pisca os LEDs para indicar que o jogo acabou.

void gameOver() {
  for (int i = 0; i <= 3; i++) {
    tone(pinoAudio, tons[i]);
    digitalWrite(pinosLeds[i], HIGH);
    delay(200);
    digitalWrite(pinosLeds[i], LOW);
    noTone(pinoAudio);
  }
  tone(pinoAudio, tons[3]);
  for (int i = 0; i <= 3; i++) {
    digitalWrite(pinosLeds[0], HIGH);
    digitalWrite(pinosLeds[1], HIGH);
    digitalWrite(pinosLeds[2], HIGH);
    digitalWrite(pinosLeds[3], HIGH);
    delay(100);
    digitalWrite(pinosLeds[0], LOW);
    digitalWrite(pinosLeds[1], LOW);
    digitalWrite(pinosLeds[2], LOW);
    digitalWrite(pinosLeds[3], LOW);
    delay(100);
  }
  noTone(pinoAudio);
  perdeu_o_jogo = true;
}

// =================================================================================================================================================

// FUNÇÃO QUE TOCA O SOM DE INÍCIO DO JOGO:  Esta função toca um som no início do jogo para sinalizar o início da partida. 
// Ela também faz os LEDs piscarem brevemente.

void tocarSomDeInicio() {
  tone(pinoAudio, tons[0]);
  digitalWrite(pinosLeds[0], HIGH);
  digitalWrite(pinosLeds[1], HIGH);
  digitalWrite(pinosLeds[2], HIGH);
  digitalWrite(pinosLeds[3], HIGH);
  delay(500);
  digitalWrite(pinosLeds[0], LOW);
  digitalWrite(pinosLeds[1], LOW);
  digitalWrite(pinosLeds[2], LOW);
  digitalWrite(pinosLeds[3], LOW);
  delay(500);
  noTone(pinoAudio);
}

// =================================================================================================================================================

// FUNÇÃO PARA ENVIAR OS PONTOS ACUMULADOS POR UM JOGADOR DURANTE A PARTIDA: Esta função é responsável por enviar os pontos 
// acumulados pelo jogador para o servidor MQTT. Os pontos são convertidos em uma string e publicados em um tópico específico.

void enviarPontos() {
  char pontos_str[4];
  snprintf(pontos_str, sizeof(pontos_str), "%d", pontos_acumulados);
  client.publish("jogo/genius/pontos", pontos_str);
}

