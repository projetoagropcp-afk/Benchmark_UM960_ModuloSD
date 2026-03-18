/*
 * Arquitetura de Integração: UM960 GNSS + SD Card Logger para Benchmark A/B
 * Autor: Pedro Vinicius
 * Plataforma Alvo: ESP32
 */

#include <SPI.h>
#include <SD.h>
#include <SparkFun_Unicore_GNSS_Arduino_Library.h>

// --- Configurações de Pinos (Mantidos idênticos para compatibilidade de hardware) ---
// GNSS UART
const int pin_UART1_RX = 16;
const int pin_UART1_TX = 17;

// Cartão SD SPI (Padrão VSPI do ESP32)
const int chipSelect = 5; 

// --- Objetos Globais ---
UM980 myGNSS; // A classe base chama-se UM980, mas é 100% compatível com a engine do UM960
HardwareSerial SerialGNSS(1); // Usa UART1 do ESP32
File logFile;

// --- Variáveis de Controle ---
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL_MS = 1000; // Taxa de gravação: 1Hz
const String filename = "/benchmark_um960.csv"; // ARQUIVO ALTERADO PARA O BENCHMARK

// --- Declaração da Função de Callback ---
void output(uint8_t * buffer, size_t length);

void setup() {
  Serial.begin(115200);
  delay(1000); // Aguarda estabilização do monitor serial
  
  Serial.println("\n--- Iniciando Sistema de Benchmark GNSS (Modo UM960) ---");

  // 1. Inicialização do Cartão SD
  Serial.print("Montando o Cartão SD...");
  if (!SD.begin(chipSelect)) {
    Serial.println(" FALHA! Verifique as conexões SPI.");
    while (true); 
  }
  Serial.println(" SUCESSO!");

  // Criação do cabeçalho CSV para o UM960
  bool fileExists = SD.exists(filename);
  logFile = SD.open(filename, FILE_APPEND); 
  if (logFile) {
    if (!fileExists) {
      logFile.println("Millis,Latitude,Longitude,Altitude_m,SIV,PositionType");
    }
    logFile.close();
  } else {
    Serial.println("Erro ao abrir arquivo para gravar o cabeçalho.");
    while(true);
  }

  // 2. Inicialização do GNSS UM960
  Serial.print("Conectando ao Módulo UM960...");
  SerialGNSS.begin(115200, SERIAL_8N1, pin_UART1_RX, pin_UART1_TX);

  // Inicia a comunicação. A biblioteca gerencia o protocolo Unicore.
  if (myGNSS.begin(SerialGNSS, "SFE_Unicore_GNSS_Library", output) == false) {
    Serial.println(" FALHA! Módulo não detectado na UART1.");
    while (true);
  }
  Serial.println(" SUCESSO! Módulo Unicore detectado.");

  // Configurações do GNSS
  myGNSS.setModeRoverSurvey(); 
  myGNSS.saveConfiguration();

  Serial.println("Sistema UM960 Pronto. Coletando dados a 1Hz...");
}

void loop() {
  myGNSS.update(); 

  if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = millis();

    // Extração
    double lat = myGNSS.getLatitude();
    double lon = myGNSS.getLongitude();
    float alt = myGNSS.getAltitude();
    int siv = myGNSS.getSIV(); 
    int posType = myGNSS.getPositionType(); 

    // Formatação do Payload CSV
    String dataString = String(millis()) + ",";
    dataString += String(lat, 9) + ",";
    dataString += String(lon, 9) + ",";
    dataString += String(alt, 3) + ",";
    dataString += String(siv) + ",";
    dataString += String(posType);

    // Gravação SD
    logFile = SD.open(filename, FILE_APPEND);
    if (logFile) {
      logFile.println(dataString);
      logFile.close();
      
      Serial.print("[UM960] Gravado: ");
      Serial.println(dataString);
    } else {
      Serial.println("Erro de I/O no Cartão SD!");
    }
  }
}

//----------------------------------------
// Função Output Obrigatória
//----------------------------------------
void output(uint8_t * buffer, size_t length)
{
    size_t bytesWritten;
    if (Serial)
    {
        while (length)
        {
            while (Serial.availableForWrite() == 0);
            bytesWritten = Serial.write(buffer, length);
            buffer += bytesWritten;
            length -= bytesWritten;
        }
    }
}