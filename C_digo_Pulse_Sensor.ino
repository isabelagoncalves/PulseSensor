

#include <ESP8266WiFi.h>
#include <PubSubClient.h>


const int PulseSensorPurplePin = A0; // Sensor de pulso conectado ao pino A0 do ESP8266.
int LED = 4;   //  LED indicador de sinal
unsigned int loop_main = 0;

unsigned long time_anterior, time_atual, time_total;
char toggle = 0;

const char* SSID = " "; // Nome da rede.
const char* PASSWORD = " "; // Senha da rede.
const char* BROKER_MQTT = "broker.hivemq.com"; // Mediador entre ESP8266 e Smartphone.
const int BROKER_PORT = 1883;
const char* ID_MQTT = "PulseSensor";

char msg[32];

WiFiClient espClient;
PubSubClient mqtt(espClient);
 
int Signal; // Mantém os dados brutos de entrada. O valor do sinal pode variar de 0 a 1024.
int Threshold = 550; // Determina qual sinal "conta como uma batida" e qual deve ser ignorado.
double BPM;

void wifi_connect(void)
{
  // Conexão a rede Wi-Fi.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
  }else
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void initMQTT(void) 
{
    mqtt.setServer(BROKER_MQTT, BROKER_PORT);   //Informa qual broker e porta deve ser conectado.
    mqtt.setCallback(mqttCallback);            //Atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega).
}

void connectMQTT(void) 
{
    while (!mqtt.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (mqtt.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
        } 
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentativa de conexao em 2s");
            delay(2000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) 
{
    char c;
    String msg;
    
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.write(payload, length);
    Serial.println();

    //Obtem a string do payload recebido.
    for(int i = 0; i < length; i++) 
    {
       c = (char)payload[i];
       msg += c;
    }
    
    //Avalia se a mensagem é para este NodeMCU.
    if (msg.equals("1"))
    {
      Serial.println("");   
    }

    if (msg.equals("0"))
    {
      Serial.println("");   
    } 
}

void reconectWiFi() 
{
    //Se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão.
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
  
    Serial.println();
    Serial.print("Reconectado com sucesso na rede ");
    Serial.println(SSID);
    Serial.print("IP obtido: ");
    Serial.println(WiFi.localIP());
}

void checkConnections(void)
{
  if (!(WiFi.status() == WL_CONNECTED))
  {

    Serial.print("Lost WiFi!");
    
    reconectWiFi();
  }
  
  if(mqtt.connected())
  {
    Serial.print("            ");
    return;
  }
    else
        {
          Serial.println("Lost Server!");
      
          connectMQTT();
        }
}
 

void setup() {
  pinMode(LED,OUTPUT); // LED piscará de acordo com as batidas do coração.
  Serial.begin(115200); 
  wifi_connect();
  initMQTT();
  connectMQTT();
}


void loop() {
checkConnections();
  Signal = analogRead(PulseSensorPurplePin);  // Lê o valor do pino analógico e atribua este valor à variável "Sinal".
 
   Serial.println(Signal);                  

   snprintf(msg, 8, "%3d", (int)Signal);
   mqtt.publish("/univap/proj/sinal", msg); // Publica o valor no subscrito.
 
 
 
   if(Signal > Threshold){   // Se o sinal estiver acima de "550", "ligará" o LED integrado ao ESP8266.
     digitalWrite(LED,HIGH);
     if(toggle)
     {
      time_anterior = time_atual;
      time_atual = millis();
     }
     toggle = 0;
     
   } else {
     digitalWrite(LED,LOW); //  Se o sinal estiver abaixo de "550", "desligará" o LED integrado ao ESP8266.
     toggle = 1;
   }
 
loop_main++;
if(loop_main > 200)
{
  
  if(time_atual > time_anterior) time_total = time_atual - time_anterior;
  if(time_anterior > time_atual) time_total = time_anterior - time_atual;

  BPM = (double)60000/time_total;
  if(BPM < 50.0 || BPM > 150) BPM = 0;
  Serial.print("BPM: ");
  Serial.println((int)BPM);
  snprintf(msg, 8, "%3d", (int)BPM);
  mqtt.publish("/univap/proj/pulse", msg);
  //Serial.print("Tempo1: ");
  //Serial.println(time_anterior);
  //Serial.print("Tempo2: ");
  //Serial.println(time_atual);
  //Serial.print("Tempo: ");
  //Serial.println(time_total);
}
delay(10); 

mqtt.loop();
}
