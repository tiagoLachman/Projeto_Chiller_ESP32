#include <Arduino.h>
#include <MuxClass.h>

#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WiFi.h"

// Senha e ssid do wifi

// Nome da rede
const char *ssid = "Procopio";
// Senha da rede
const char *password = "Pr0c0p10";

// Pinos entrada e saída

// Pino do led
const int pinLed = 2;

// Pinos dos motores
const int pinMotores[] = {GPIO_NUM_16};
// Tamanho do vetor pinMotores
const int sizePinMotores = sizeof(pinMotores) / sizeof(pinMotores[0]);

// Pinos de controle dos mux
int pinControle[] = {
    21,  // C
    22,  // B
    23,  // A
};
// Tamanho do vetor pinControle
const int sizePinControle = sizeof(pinControle) / sizeof(pinControle[0]);

// Pinos de inhables
int pinInh[] = {
    18,
    19,
};
// Tamanho do vetor pinInh
const int sizePinInh = sizeof(pinInh) / sizeof(pinInh[0]);

// Pino de comum dos mux's
int pinComum = 17;

// Funções

/** @brief Liga um dos motores
 *  @param parameter Parametros enviados pelo taskCreate do
 * FreeRTOS
 *  @return void
 */
void ligarMotor(void *parameter);

/**
 * @brief Manda a página inicial para o usuário
 * @param req Requisição do usuário
 * @return void
 */
void sendHome(AsyncWebServerRequest *req);

/**
 * @brief Configura as portas de entrada e saída
 * da ESP32
 * @return void
 */
void configPortas();

/**
 * @brief Monitora as falhas dos chillers
 * @return void
 */
void monitorarEntradas();

/**
 * @brief Lê a entrada requisitada com o demux
 * @param num Numero da entrada
 * @return int valor lido da entrada
 */
int lerEntrada(int num);

String ledState;

// Servidor WEB
AsyncWebServer server(80);

// Definindo portas de saída e entrada para ler as entradas do MUX
Mux mux(pinInh, sizePinInh, pinControle, sizePinControle, pinComum, MUX);

/*
//Reatribui o valor do placeholder do html
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(pinLed)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}
*/
void setup() {
  Serial.begin(115200);
  configPortas();

  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao carregar SPIFFS");
    return;
  }

  bool estadoLed = false;
  int contReset = 0;
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(pinLed, estadoLed);
    estadoLed = !estadoLed;

    // Se ela não se conectar em 5 segundos
    if (contReset >= 5) {
      ESP.restart();  // Reinicia a esp32

      while (1) {
      }  // Trava a ESP32
    }

    Serial.println("Conectando ao WiFi...");
    delay(1000);
    contReset++;
  }
  Serial.println("Conectado!");
  // Pisca o led várias vezes para mostrar que está conectado
  for (int i = 0; i < 20; i++) {
    digitalWrite(pinLed, estadoLed);
    estadoLed = !estadoLed;
    delay(50);
  }
  digitalWrite(pinLed, 0);
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) { sendHome(req); });

  /*
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  */

  server.on("/motor1", HTTP_GET, [](AsyncWebServerRequest *req) {
    int num = 1;
    // Liga o motor 1
    xTaskCreate(ligarMotor, "teste", 1000, (void *)&num, 1, NULL);
    // Manda a tela inicial
    sendHome(req);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    String res = "";
    for (int i = 0; i <= mux.maxSaidas(); i++) {
      res += String(i) + String(":");
      res += String(mux.lerEntrada(i));
      res += String("\n\r");
    }
    req->send(200, "text/plain", res);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Puts");
  });
  server.begin();
}

void loop() {}

void configPortas() {
  int i = 0;

  // Led de comunicação visual
  pinMode(pinLed, OUTPUT);

  // Pinos de controle dos motores como SAÍDA
  for (int i = 0; i < sizePinMotores; i++) {
    pinMode(pinMotores[i], OUTPUT);
  }
}

void ligarMotor(void *parameter) {
  int numMotor = *((int *)parameter);
  // Se numMotor invalido, para de executar a função
  if (numMotor > sizePinMotores || numMotor < 1) return;

  // Liga a saida
  digitalWrite(pinMotores[numMotor - 1], 1);

  // Delay de 1 segundo
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Desliga a saida
  digitalWrite(pinMotores[numMotor - 1], 0);

  // Se deleta
  vTaskDelete(NULL);
}

void sendHome(AsyncWebServerRequest *req) {
  req->send(SPIFFS, "/index.html", String(), false);
}

void monitorarEntradas() {}

int lerEntrada(int num) {}