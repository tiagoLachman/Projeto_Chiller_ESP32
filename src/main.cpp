// Versão do projeto
#define VERSAO_PROJETO "1.0"

// Nome do projeto
#define NOME_PROJETO "Leitor_Chiller"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <MuxClass.h>

#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WiFi.h"

enum {
  e_Energia = 0,
  e_Chiler1,
  e_Chiler2,
  e_Chiler3,
  e_BombaHidraulica1,
  e_BombaHidraulica2,
  e_BombaHidraulica3,
  e_BombaTorre1,
  e_BombaTorre2,
  e_BombaTorre3,
  e_Ventilador1,
  e_Ventilador2,
  e_Ventilador3
};

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
 * @param parameter Parametros enviados pelo taskCreate do
 * FreeRTOS
 * @return void
 */
void monitorarEntradas(void *paramter);

/**
 * @brief Altera a variavel STATE no HTML
 *
 * @param var Nome da variavel colocada no HTML para ser alterada
 * @return String vazia se não achar o parametro passado,
 * e uma String com um valor associado caso achar
 */
String processor(const String &var);

/**
 * @brief Handle do OTA
 * @param parameter Parametros enviados pelo taskCreate do
 * FreeRTOS
 * @return void
 */
void OTA_Handle(void *paramter);

/**
 * @brief Handle para reiniciar a esp caso fique sem internet
 *
 * @param paramter Parametros enviados pelo taskCreate do FreeRTOS
 */
void wifiRestart_Handle(void *paramter);

/**
 * @brief Transforma WiFi.status() em String
 *
 * @param wifiStatus WiFi.status() status da conexão wifi
 * @return String status do wifi em string
 */
String wifiStatusToString(int wifiStatus);

/**
 * @brief Transforma WiFi.RSSI() em String
 *
 * @param wifiRssi WiFi.RSSI(), força da conexão wifi
 * @return String força da conexão
 */
String wifiRssiToString(int8_t wifiRssi);

// Objetos

// Servidor WEB
AsyncWebServer server(80);

// Definindo portas de saída e entrada para ler as entradas do MUX
Mux mux(pinInh, sizePinInh, pinControle, sizePinControle, pinComum, MUX);

// Variáveis em geral

// Estado das entradas de falhas
int estadoEntradas[20];

/**
 * Para saber quando o usuario apertou algum botão de ligar motor
 * e não criar mais uma task sem necessidade
 */
bool ligandoMotor = false;

// Tamanho do vetor estadoEntradas
const int sizeEstadoEntradas = sizeof(estadoEntradas) / sizeof(estadoEntradas[0]);

// Programa

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

    if (contReset >= 5) {
      // Reinicia a esp se ela não se conectar em 5 segundos
      ESP.restart();
      // Trava a ESP32
      while (1) {
      }
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

  // Nome da esp na rede
  ArduinoOTA.setHostname("esp32Chiller");

  // Inicia o gerenciamento das atualizações via WIFI
  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else  // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
        // using SPIFFS.end()
        Serial.println("Iniciando atualização " + type);
      })
      .onEnd([]() { Serial.println("\nFim"); })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Erro[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Falha na autentificação");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Falha na inicialização");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Falha na conexão");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Falha na recepção");
        else if (error == OTA_END_ERROR)
          Serial.println("Falha no final");
      });

  ArduinoOTA.begin();

  xTaskCreate(monitorarEntradas, "Monitorar_Entradas", 1000, NULL, 1, NULL);

  /*
// Não funciona, pq? n sei.
xTaskCreate(OTA_Handle, "Ota_Handle", 2000, NULL, 1, NULL);
*/

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) { sendHome(req); });

  /*
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  */

  server.on("/motor1", HTTP_GET, [](AsyncWebServerRequest *req) {
    int num = 1;
    // Liga o motor 1
    if (!ligandoMotor) {
      ligandoMotor = true;
      xTaskCreate(ligarMotor, "teste", 1000, (void *)&num, 1, NULL);
    }
    // Manda a tela inicial
    sendHome(req);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    String res = "";
    for (int i = 0; i <= mux.maxSaidas(); i++) {
      res += String(i) + String(":");
      res += String(estadoEntradas[i]);
      res += String("\n\r");
    }
    req->send(200, "text/plain", res);
  });

  // Dados para ser enviados para servidor Node
  server.on("/jsonRes", HTTP_GET, [](AsyncWebServerRequest *req) {
    String res = "";
    StaticJsonDocument<500> dados;

    // Colocar os dados em um arquivo Json
    dados["Nome"] = NOME_PROJETO;
    dados["Versao"] = VERSAO_PROJETO;

    // Leitura das entradas de falhas
    for (int i = 0; i < mux.maxSaidas(); i++) {
      dados["E" + String(i)] = mux.lerEntrada(i);
    }

    serializeJson(dados, res);
    req->send(200, "application/json", res);
  });

  server.on("/WiFiStatus", HTTP_GET, [](AsyncWebServerRequest *req) {
    String res = "";
    StaticJsonDocument<500> dados;

    // Colocar os dados em um arquivo Json
    dados["SSID"] = WiFi.SSID();
    dados["HostName"] = WiFi.getHostname();
    dados["IP"] = WiFi.localIP();
    dados["Status"] = wifiStatusToString(WiFi.status());
    dados["RSSI"] = WiFi.RSSI();

    serializeJson(dados, res);
    req->send(200, "application/json", res);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Puts");
  });

  server.begin();

  // No final pois, o WiFi.status() demora para ser atualizado
  xTaskCreate(wifiRestart_Handle, "Monitorar_Wifi", 1000, NULL, 1, NULL);
}

void loop() {
  ArduinoOTA.handle();
}

void configPortas() {
  int i = 0;

  // Led de comunicação visual
  pinMode(pinLed, OUTPUT);

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

  // Reseta o ligando motor
  ligandoMotor = false;

  // Se deleta
  vTaskDelete(NULL);
}

void sendHome(AsyncWebServerRequest *req) {
  req->send(SPIFFS, "/index.html", String(), false, processor);
}

void monitorarEntradas(void *paramter) {
  while (1) {
    for (int i = 0; i <= mux.maxSaidas(); i++) {
      estadoEntradas[i] = mux.lerEntrada(i);
    }
    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
  // Se deleta caso saia do loop
  vTaskDelete(NULL);
}

String processor(const String &var) {
  // Serial.println(var);
  if (var.substring(0, 5).equals("dados")) {
    String temp = var.substring(5);
    // Serial.println(temp);
    int aux = temp.toInt();
    if (aux < sizeEstadoEntradas && aux >= 0) {
      return estadoEntradas[aux] == 1 ? "Falha" : "---";
    }
  } else if (var == "sinalWifi") {
    return wifiRssiToString(WiFi.RSSI());
  }
  return String();
}

void OTA_Handle(void *paramter) { ArduinoOTA.handle(); }

String wifiStatusToString(int wifiStatus) {
  if (wifiStatus == WL_NO_SHIELD)
    return "WL_NO_SHIELD";
  else if (wifiStatus == WL_IDLE_STATUS)
    return "WL_IDLE_STATUS";
  else if (wifiStatus == WL_NO_SSID_AVAIL)
    return "WL_NO_SSID_AVAIL";
  else if (wifiStatus == WL_SCAN_COMPLETED)
    return "WL_SCAN_COMPLETED";
  else if (wifiStatus == WL_CONNECTED)
    return "WL_CONNECTED";
  else if (wifiStatus == WL_CONNECT_FAILED)
    return "WL_CONNECT_FAILED";
  else if (wifiStatus == WL_CONNECTION_LOST)
    return "WL_CONNECTION_LOST";
  else if (wifiStatus == WL_DISCONNECTED)
    return "WL_DISCONNECTED";
  else
    return "UNKNOWN";
}

String wifiRssiToString(int8_t wifiRssi) {
  if (wifiRssi < 0 && wifiRssi >= -50)
    return "Excelente";
  else if (wifiRssi < -50 && wifiRssi >= -60)
    return "Muito bom";
  else if (wifiRssi < -60 && wifiRssi >= -70)
    return "Bom";
  else if (wifiRssi < -70 && wifiRssi >= -80)
    return "Ruim";
  else if (wifiRssi < -90 && wifiRssi >= -90)
    return "Muito Ruim";
  else if (wifiRssi < -90)
    return "Sem sinal";
  else
    return "Unknown";
}

void wifiRestart_Handle(void *paramter) {
  while (1) {
    // delay nessa posição pois WiFi.status() demora para ser atualizado
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    if (WiFi.status() != WL_CONNECTED) {
      // Reinicia a esp se ela perder a conexão
      ESP.restart();
      while (1) {
      }
    }
  }
  vTaskDelete(NULL);
}