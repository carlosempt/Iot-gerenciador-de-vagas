#include <NewPing.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- CONFIGURAÇÕES DE REDE ---
const char* ssid = "Carlos Henrique 2.4G";
const char* password = "17218511";
const char* serverUrl = "https://iot-gerenciador-de-vagas.onrender.com/atualizar_vaga"; 

// --- PINOS DOS SENSORES E LEDS ---
#define TRIGGER_PIN_1 32
#define ECHO_PIN_1    34
#define TRIGGER_PIN_2 33
#define ECHO_PIN_2    35
#define TRIGGER_PIN_3 25
#define ECHO_PIN_3    26
#define MAX_DISTANCE  200

#define GREEN_LED_SENSOR1 2
#define RED_LED_SENSOR1   0
#define GREEN_LED_SENSOR2 4
#define RED_LED_SENSOR2   16
#define GREEN_LED_SENSOR3 17
#define RED_LED_SENSOR3   27

NewPing sonar1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonar3(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

const unsigned int DETECCAO_DISTANCIA = 10;
const unsigned long TEMPO_ANALISE = 60000; // 1 minuto em milissegundos

// Estrutura do Sensor para controlar o estado e o tempo individualmente
struct Sensor {
  bool statusOcupadoEnviado = false; // Guarda o último estado enviado (true=ocupado, false=livre)
  unsigned long inicioDeteccao = 0; // Armazena QUANDO a detecção começou. 0 = não está em contagem.
  unsigned int distancia = 0;
};

Sensor sensor1, sensor2, sensor3;

// --- FUNÇÃO PARA ENVIAR STATUS (sem alterações) ---
void enviarStatusVaga(int vagaId, bool ocupada) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String status = ocupada ? "ocupada" : "disponivel";
    String jsonPayload = "{\"vagaId\":" + String(vagaId) + ", \"status\":\"" + status + "\"}";

    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0) {
      Serial.printf("Vaga %d: Status '%s' enviado. Resposta: %d\n", vagaId, status.c_str(), httpResponseCode);
    } else {
      Serial.printf("Vaga %d: Falha ao enviar status. Erro: %s\n", vagaId, http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi desconectado. Não foi possível enviar o status.");
  }
}

// --- FUNÇÃO PARA PROCESSAR CADA SENSOR INDIVIDUALMENTE ---
void processaSensor(Sensor &sensor, NewPing &sonar, int vagaId, int greenPin, int redPin) {
  sensor.distancia = sonar.convert_cm(sonar.ping_median(5));
  
  // Verifica se há um objeto na vaga
  bool objetoDetectado = (sensor.distancia > 0 && sensor.distancia < DETECCAO_DISTANCIA);

  if (objetoDetectado) {
    // --- Objeto está PRESENTE na vaga ---

    if (sensor.inicioDeteccao == 0) {
      // É a primeira vez que vemos o objeto. Iniciamos o cronômetro.
      sensor.inicioDeteccao = millis();
      Serial.printf("Vaga %d: Objeto detectado. Iniciando análise de 1 minuto.\n", vagaId);
    } else {
      // O objeto continua aqui. Vamos checar se o tempo de análise já passou.
      if (!sensor.statusOcupadoEnviado && (millis() - sensor.inicioDeteccao >= TEMPO_ANALISE)) {
        // O objeto está aqui há mais de 1 minuto E AINDA NÃO informamos o servidor.
        Serial.printf("Vaga %d: Ocupação confirmada! Atualizando status.\n", vagaId);
        
        // 1. Envia o status de "ocupada"
        enviarStatusVaga(vagaId, true);
        sensor.statusOcupadoEnviado = true; // Marcamos que já enviamos o status "ocupado"
        
        // 2. Acende o LED vermelho
        digitalWrite(greenPin, LOW);
        digitalWrite(redPin, HIGH);
      }
    }
  } else {
    // --- Objeto está AUSENTE da vaga ---

    if (sensor.statusOcupadoEnviado) {
      // A vaga estava ocupada e agora foi liberada.
      Serial.printf("Vaga %d: Foi liberada! Atualizando status.\n", vagaId);
      
      // 1. Envia o status de "disponivel"
      enviarStatusVaga(vagaId, false);
      sensor.statusOcupadoEnviado = false; // Marcamos que já enviamos o status "livre"
      
      // 2. Acende o LED verde
      digitalWrite(greenPin, HIGH);
      digitalWrite(redPin, LOW);
    }
    
    // Se não há objeto, o cronômetro é zerado.
    sensor.inicioDeteccao = 0;
  }
}

void setup() {
  Serial.begin(115220);
  
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");

  pinMode(GREEN_LED_SENSOR1, OUTPUT);
  pinMode(RED_LED_SENSOR1, OUTPUT);
  pinMode(GREEN_LED_SENSOR2, OUTPUT);
  pinMode(RED_LED_SENSOR2, OUTPUT);
  pinMode(GREEN_LED_SENSOR3, OUTPUT);
  pinMode(RED_LED_SENSOR3, OUTPUT);
  
  // Todas as vagas começam como livres (LED verde aceso)
  digitalWrite(GREEN_LED_SENSOR1, HIGH);
  digitalWrite(RED_LED_SENSOR1, LOW);
  digitalWrite(GREEN_LED_SENSOR2, HIGH);
  digitalWrite(RED_LED_SENSOR2, LOW);
  digitalWrite(GREEN_LED_SENSOR3, HIGH);
  digitalWrite(RED_LED_SENSOR3, LOW);
}

void loop() {
  // Processa cada sensor de forma independente
  processaSensor(sensor1, sonar1, 1, GREEN_LED_SENSOR1, RED_LED_SENSOR1);
  processaSensor(sensor2, sonar2, 2, GREEN_LED_SENSOR2, RED_LED_SENSOR2);
  processaSensor(sensor3, sonar3, 3, GREEN_LED_SENSOR3, RED_LED_SENSOR3);

  // Um pequeno delay para estabilidade, mas que permite checagens rápidas.
  delay(500); 
}
