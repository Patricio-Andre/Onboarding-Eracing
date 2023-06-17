#include <ESP8266WiFi.h>  // Bibliotecas para gerar um webserver
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>          // Biblioteca de comunicação SPI Nativa
#include <SD.h>           // Biblioteca de comunicação com cartão SD Nativa
#include <Wire.h>         // Biblioteca de comunicação I2C
#include <VL53L0X.h>      // Biblioteca para usar as funções do sensor VL53L0X
#define botaoPin D3
#define tempoOperacao 100000
#define distOperacao 2400 
// distância a qual se detectado em distância menor que essa por tempo maior que 800 ms é considerada uma passagem do veículo

// Variáveis globais usadas para estabelecer o WebServer
const char* ssid = "PATRICIO-PC";
const char* password = "J4P!7M40E";
ESP8266WebServer server(80);
// Variáveis globais para o funcionamento geral
int estadoReceptor = HIGH;
int estadoAnterior = HIGH; // Estado anterior do botão
int lap = 0;
int i = 0;
bool iniciarLeitura = false;
bool cartaoOk = true;
unsigned long tempoInicial = 0;
unsigned long tempoFinal = 0;
unsigned long t_voltas[101];
unsigned long interrupcoes[101];
String fileName = "datalog.csv";
File dataFile;            // Objeto responsável por escrever/Ler do cartão SD
const int chipSelect = D8; 
// Constante que indica em qual pino está conectado o Chip Select do módulo de comunicação
String leitura = "";
// Variaveis para armazenar a distancia atual e o ultimo valor lido
int dist = 0, dist_old = 0;
// Variavel para armazenar o tempo na parte do timeout
unsigned long timeout = 0;
VL53L0X sensor;


void handleRoot() {
  // Caso esteja acessando o endereço raiz, essa função será acessada.
  String textoHTML;
  textoHTML = "";
  textoHTML += "<meta http-equiv=\"refresh\" content=\"5\">";
  // a cada 5 segundos o site é recarregado, assim os dados também são
    for (int i = 0; i < lap; i++){
      textoHTML += i;
      textoHTML += ",";
      textoHTML += t_voltas[i];
      textoHTML += ",";
      textoHTML += interrupcoes[i];
      textoHTML += "<br>";
    }
  // envia o texto html para a raiz
  server.send(200, "text/html", textoHTML);
}

void handleNotFound(){
  // Caso não acessemos a raiz, estaremos em uma página não encontrada.
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  // As configurações Básicas são chamadas aqui
  pinMode(botaoPin, INPUT_PULLUP); // Botão para iniciar leitura
  Serial.begin(115200);
  // O webserver Wi-Fi é estabelecido nas linhas seguintes
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Até se conectar a cada meio segundo será printado um .
  // Caso algo esteja errado com a conexão (senha ou nome da rede errados) ele
  // deixara pontos para sempre
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // devolve o IP que se inserido em um navegador acessa a raiz do webserver
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  // Checa se o SD esta conectado corretamente
  if (!SD.begin(chipSelect)) {
    Serial.println("Erro na leitura do arquivo não existe um cartão SD ou o módulo está conectado corretamente ?");
    cartaoOk = false;
  } else {
    Serial.println("Leitura correta");
  }
  while (cartaoOk && SD.exists(fileName)){
    i++;
    fileName = "datalog" + String(i) + ".csv";
    delay(200);
  }
  Wire.begin(); // Inicia comunicação I2C
  // Acionando o funcionamento do sensor 
  sensor.init();
  sensor.setTimeout(500);

  // Perfil de longo alcance
  sensor.setSignalRateLimit(0.1);
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  // Definindo tempo de operação / coleta dos dados
  // sensor.setMeasurementTimingBudget(tempoOperacao);
}

void loop() {
  dist = sensor.readRangeSingleMillimeters();
  if (dist != dist_old && dist == 65535) {
    // Para indicar um erro caso o cabeamento esteja solto
    Serial.println("Erro Interno");
  }
  // Filtro de interferências do sol
  filtrar_sinal();
  if (dist < distOperacao) {
    estadoReceptor = LOW;
  } else {
    estadoReceptor = HIGH;
  }
  int botaoInicio = digitalRead(botaoPin);
  if (botaoInicio == LOW && !iniciarLeitura) {
    iniciarLeitura = true;
    Serial.println("leitura iniciada");
    if (cartaoOk){
      dataFile = SD.open(fileName, FILE_WRITE); 
      Serial.println("Cartão SD Inicializado para escrita");
      leitura = "";
    }
    delay(1000);
    tempoFinal = millis();
  } else if (botaoInicio == LOW && iniciarLeitura) {
    iniciarLeitura = false;
    Serial.println("leitura finalizada");
    // Se o arquivo estiver realmente aberto para leitura executamos as seguintes linhas de código
    if (dataFile) {   
      Serial.println(leitura);    // Mostramos no monitor a linha que será escrita
      dataFile.println(leitura);  // Escrevemos no arquivos e pulamos uma linha
      dataFile.close();           // Fechamos o arquivo
    }
    delay(1000);
  }
  if (iniciarLeitura && estadoReceptor != estadoAnterior) { // Verifica se houve mudança no estado do botão
    if (estadoReceptor == LOW) { // Verifica se o botão foi pressionado
      tempoInicial = millis(); // Registra o tempo inicial
    } else { // Botão foi liberado
      unsigned long duracaoSinal = millis() - tempoInicial;
      if (duracaoSinal > 800){ //só considera a volta se o tempo de interrupção for maior que meio segundo
        t_voltas[lap] = tempoInicial - tempoFinal; // adiciona o tempo de volta
        // Calcula a duração do tempo pressionado
        interrupcoes[lap] = duracaoSinal; // a interrupção é a duracão do sinal
        tempoFinal = millis() - duracaoSinal; // O tempo Final da volta é registrado
        // a linha do CSV é escrita em leitura
        leitura += String(lap) + "," + String(t_voltas[lap]) + "," + String(interrupcoes[lap]) + "\n";
        lap++;
      }
    }
  }
  
  estadoAnterior = estadoReceptor; // Atualiza o estado anterior
  // Atualiza o web server
  server.handleClient();
}

void filtrar_sinal() {
  // Função que filtra interferências no sinal
  // Se a distância medida for maior que 8000 e ainda não tiver passado 1 segundo de timeout
  if (dist > 8000 && ((millis() - timeout) < 1000)) {
    // Descarta a medição feita e iguala ela à anterior
    dist = dist_old;
  } else {
    // Caso contrário (medição < 8000 ou passou do timeout) 
    // Não descarta a medição atual e atualiza a medição antiga para a atual
    dist_old = dist;
    timeout = millis(); // Reseta o valor da variável do timeout
  }
}
