// GPIO32 - GPIO39 são canal ADC 1 e GPIO0, GPIO2, GPIO4, GPIO12 - GPIO15, GOIO25 - GPIO27 são ADC2. Porém o ADC2 NÃO FUNCIONA SE O WIFI ESTIVER LIGADO!
// ------ Bibliotecas --------
#include <driver/adc.h>
#include "EmonLib.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <PubSubClient.h>  // https://github.com/knolleary/pubsubclient
#include "EspMQTTClient.h" // https://github.com/esp32/libraries

// ------ Configurações WiFi e Broker MQTT --------

const char* ssid = "SOUSA HOME 2.4G"; // SSID / nome da rede WI-FI que deseja se conectar
const char* password = "988272928"; // Senha da rede WI-FI que deseja se conectar
const char* mqtt_server = "192.168.100.75";

// ------ Defines e Variáveis --------
EnergyMonitor emon;
EnergyMonitor emon2;
EnergyMonitor emon3;

#define vCalibration 106.8
#define currCalibration 100 // 7 0.52

float kWh = 0;
unsigned long lastmillis = millis();
int i=0;
//static const char* connectionString = "";
long lastTemp = 0;

int pino_sensor_tensao1 = 36; // Mesmo pino VP - ADC1 0
int pino_sensor_corrente1 = 34; // Pino 34 - ADC1 6 - CABO VERDE
int pino_sensor_corrente2 = 35; // Pino 35 - ADC1 7 - CABO AZUL
int pino_sensor_corrente3 = 32; // Pino 32 - ADC1 4 - CABO ROXO

float tensao_rms;
float tensao_pico;
float valor_inst;
double menor_valor=0;

float valor_inst1;
double menor_valor1=0;
float valor_inst2;
double menor_valor2=0;
float valor_inst3;
double menor_valor3=0;



// ------ Configurações PORTAS

//static bool hasIoTHub = false;

WiFiClient espClient;
PubSubClient client(espClient);



void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Conectando à rede: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Conectando ao MQTT...");
    // Conectando 
    if (client.connect("ESP8266Client")) //if (client.connect("ESP32Client"))
    {
      Serial.println("Conectado");      // Conectado
      client.publish("smartMonitor/KeepAlive", "Conectado!");
      // Envia a mensagem ao servidor
      //client.subscribe("SMEMA");    
    }
    else
    {
      Serial.print("Erro:");
      Serial.print(client.state());
      Serial.println("reconectando em 5 segundos");
      // Espera 5 segundo e reconecta
      delay(5000);
    }
  }
}


void setup()
{
  pinMode(pino_sensor_tensao1, INPUT);
  Serial.begin(115200);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // Pino 34
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); // Pino 35
  adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11); // Pino 32
  
  emon.voltage(39, vCalibration, 1.7); // Voltage: input pin, calibration, phase_shift
  emon2.voltage(39, vCalibration, 1.7); // Voltage: input pin, calibration, phase_shift
  emon3.voltage(39, vCalibration, 1.7); // Voltage: input pin, calibration, phase_shift
  
  emon.current(pino_sensor_corrente1, currCalibration); // Current: input pin, calibration.
  emon2.current(pino_sensor_corrente2, currCalibration); // Current: input pin, calibration.
  emon3.current(pino_sensor_corrente3, currCalibration); // Current: input pin, calibration.
  setup_wifi();
  client.setServer(mqtt_server, 1883);
//  client.setCallback(callback);
  
}
//void callback(char* topic, byte* payload, unsigned int length)
//{
//  Serial.print("Mensagem recebida[");
//  Serial.print(topic);
//  Serial.print("] ");
//  for (int i = 0; i < length; i++)
//  {
//    Serial.print((char)payload[i]);
//  }
//  Serial.println();

//  if ((char)payload[0] == '0')
//  {
//    Serial.println("NOT AVAILABLE");
//    digitalWrite(oupAVAILABLE, HIGH);
//  }

//if ((char)payload[0] == '1')
//  {
//    Serial.println("LIGANDO D19");
//    digitalWrite(oupAVAILABLE, LOW);
//  }
//}

void loop()
{
 if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  //long now = millis();


// ------ CALCULOS CORRENTE ----
  String corrente1;
  String corrente2;
  String corrente3;
  String tensao_rms1;
  double somatorio = 0;
  
  corrente1 = String(emon.Irms);
  corrente2 = String(emon2.Irms);
  corrente3 = String(emon3.Irms);


// ---- CALCULOS DE TENSAO ---------

menor_valor = 3000;
menor_valor1 = 4095;
menor_valor2 = 4095;
menor_valor3 = 4095;
  for(int i = 0; i < 2000; i++)
  {
    
    valor_inst = analogRead(pino_sensor_tensao1);
    //Serial.println(valor_inst);
    //somatorio = valor_inst + somatorio;
    if (i==1){
      menor_valor = valor_inst;
      Serial.println(valor_inst);
    }
    if(menor_valor > valor_inst)
    {
      if ( (menor_valor - valor_inst) < 0.05*menor_valor){
      menor_valor = valor_inst;
      }
    }
    delayMicroseconds(10);
    //menor_valor = somatorio/2000;
  }

  //tensao_pico=map(menor_valor,2701,625,0,315);
  tensao_pico=map(menor_valor,2500,400,0,315);
  if (tensao_pico < 0){
    tensao_pico = 0;
  }
  tensao_rms1 = String(tensao_pico/1.4);
  Serial.print("Leitura: ");
  Serial.println(menor_valor);
  
  Serial.print("Tensão da Rede Elétrica: ");
  Serial.println(tensao_rms1);
  client.publish("smartMonitor/Tensao1", (char*) tensao_rms1.c_str());
  
// EXEMPLO!
 //String comsg;
 //comsg="Connected";
 //client.publish("/test/confirm", (char*) comsg.c_str());


  
    //double leitura = analogRead(34);


  for(int i = 0; i < 2; i++)
  {
    emon.calcVI(17, 1000);  // Calcula corrente e Tensao Calcula corrente e Tensao (17 SEMICICLOS, 1s de TEMPO LIMITE PARA FAZER A MEDIÇÃO)
    emon2.calcVI(17, 1000);  // Calcula corrente e Tensao Calcula corrente e Tensao (17 SEMICICLOS, 1s de TEMPO LIMITE PARA FAZER A MEDIÇÃO)
    emon3.calcVI(17, 1000);  // Calcula corrente e Tensao Calcula corrente e Tensao (17 SEMICICLOS, 1s de TEMPO LIMITE PARA FAZER A MEDIÇÃO)
    valor_inst1 = emon.Irms;
    valor_inst2 = emon2.Irms;
    valor_inst3 = emon3.Irms;
    
    if(menor_valor1 > valor_inst1)
    {
      menor_valor1 = valor_inst1;
    }
    if(menor_valor2 > valor_inst2)
    {
      menor_valor2 = valor_inst2;
    }
    if(menor_valor3 > valor_inst3)
    {
      menor_valor3 = valor_inst3;
    }
    delayMicroseconds(10);
  }
  
  if ( (menor_valor1 - 2.4) > 0)  { corrente1 = String(menor_valor1 - 2.4); }
  else  { corrente1 = "0"; }
  
  if ( (menor_valor2 - 2) > 0)  { corrente2 = String(menor_valor2 - 2); }
  else  { corrente2 = "0"; }
  
  if ( (menor_valor3 - 3) > 0)  { corrente3 = String(menor_valor3 - 3); }
  else  { corrente3 = "0"; }
    
  Serial.print("\tIrms 1: ");
  if (emon.Irms < 0){
      emon.Irms = 0;
  }
  Serial.print(corrente1);
  Serial.println("A");
  client.publish("smartMonitor/Corrente1", (char*) corrente1.c_str());
  
  Serial.print("\tIrms 2: ");
  if (emon2.Irms < 0){
      emon2.Irms = 0;
  }
  Serial.print(corrente2);
  Serial.println("A");
  client.publish("smartMonitor/Corrente2", (char*) corrente2.c_str());

  Serial.print("\tIrms 3: ");
  if (emon3.Irms < 0){
      emon3.Irms = 0;
  }
  Serial.print(corrente3);
  Serial.println("A");
  client.publish("smartMonitor/Corrente3", (char*) corrente3.c_str());
  
//  Serial.print("Leitura: ");         // Apparent power
//  client.publish("smartMonitor/KeepAlive", "Ultima Amostra Filtrada:"+String(corrente);
//  Serial.println(leitura);          // Irms
  //delay(300);
   


}
