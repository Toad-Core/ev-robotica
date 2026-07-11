// LIBRERIAS
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
#include "secrets.h"

// CONFIGURACIÓN DE PINES
#define DHTPIN 14      // Pin D5 conectado al S del KY-015
#define DHTTYPE DHT11  // El KY-015 internamente usa un sensor DHT11
#define PIN_AZUL 16    // Pin D0
#define PIN_VERDE 5    // Pin D1
#define PIN_ROJO 4     // Pin D2

DHT dht(DHTPIN, DHTTYPE);

// CONFIGURACIÓN
const char* wifi = SECRET_SSID;        // Wi-Fi
const char* password = SECRET_PASS;    // contraseña
const char* mqtt_server = SECRET_MQTT_SERVER; // Broker
const int mqtt_puerto = SECRET_MQTT_PORT;
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Credenciales HiveMQ
const char* mqtt_usuario = SECRET_MQTT_USER;
const char* mqtt_password = SECRET_MQTT_PASS;

// VARIABLES
float temperatura = 0.0;
float humedad = 0.0;
float umbral_temperatura = 28.0; // Umbral inicial
float umbral_humedad = 60.0;

bool modo_automatico = true; 
bool ventilador_encendido = false;
unsigned long tiempoAnterior = 0;
const long intervaloLectura = 5000; // Lee cada 5 segundos

void setup_wifi() {
  delay(10);
  Serial.print("Conectando a ");
  Serial.println(wifi);
  WiFi.begin(wifi, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}

void reconnect() {
  // Bucle para reintentar la conexion
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Intenta conectar con el Last Will
  if (client.connect("NodeMCU_SalaServidores", mqtt_usuario, mqtt_password, "proyectoFinal/sala_servidores/estado/conexion", 1, true, "Offline")) {      Serial.println("\nConectado a HiveMQ");
      // Publica estado de conexión
      client.publish("proyectoFinal/sala_servidores/estado/conexion", "Online", true);
      
      // Suscribirse a los tópicos de control para recibir comandos de MQTT
      client.subscribe("proyectoFinal/sala_servidores/control/modo");
      client.subscribe("proyectoFinal/sala_servidores/control/ventilador");
      client.subscribe("proyectoFinal/sala_servidores/config/umbral_temperatura");
      client.subscribe("proyectoFinal/sala_servidores/config/umbral_humedad");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configuramos pines del LED RGB
  pinMode(PIN_ROJO, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);
  pinMode(PIN_AZUL, OUTPUT);
  
  // Apagamos el LED al inicio
  digitalWrite(PIN_ROJO, LOW);
  digitalWrite(PIN_VERDE, LOW);
  digitalWrite(PIN_AZUL, LOW);

  dht.begin();
  setup_wifi();
  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_puerto);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Convertir el mensaje a String para manejarlo más fácil
  String mensaje = "";
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }

  Serial.print("Mensaje recibido en tópico: ");
  Serial.println(topic);
  Serial.print("Mensaje: ");
  Serial.println(mensaje);

  // --- LÓGICA DE CONTROL

  // Cambiar el Modo Automático / Manual
  if (String(topic) == "proyectoFinal/sala_servidores/control/modo") {
    if (mensaje == "AUTO") {
      modo_automatico = true;
      client.publish("proyectoFinal/sala_servidores/estado/modo", "AUTO", true);
      Serial.println("Sistema en Modo AUTOMÁTICO");
    } 
    else if (mensaje == "MANUAL") {
      modo_automatico = false;
      client.publish("proyectoFinal/sala_servidores/estado/modo", "MANUAL", true);
      Serial.println("Sistema en Modo MANUAL");
    }
  }

  // Controlar el Ventilador manualmente
  if (String(topic) == "proyectoFinal/sala_servidores/control/ventilador") {
    if (!modo_automatico) { // Solo hace caso si el sistema está en MANUAL
      if (mensaje == "ON") {
        digitalWrite(PIN_AZUL, LOW);
        digitalWrite(PIN_ROJO, LOW);
        digitalWrite(PIN_VERDE, HIGH);
        client.publish("proyectoFinal/sala_servidores/estado/ventilador", "ON", true);
        ventilador_encendido = true;
        Serial.println("Ventilador ENCENDIDO manualmente");
      } 
      else if (mensaje == "OFF") {
        digitalWrite(PIN_AZUL, LOW);
        digitalWrite(PIN_ROJO, HIGH);
        digitalWrite(PIN_VERDE, LOW);
        client.publish("proyectoFinal/sala_servidores/estado/ventilador", "OFF", true);
        ventilador_encendido = false;
        Serial.println("Ventilador APAGADO manualmente");
      }
    } else {
      Serial.println("Comando ignorado: El sistema está en AUTOMATICO");
    }
  }

  // Umbral de Temperatura remotamente
  if (String(topic) == "proyectoFinal/sala_servidores/config/umbral_temperatura") {
    umbral_temperatura = mensaje.toFloat(); // Convertir el texto a decimal
    Serial.print("Nuevo umbral de temperatura configurado a: ");
    Serial.println(umbral_temperatura);
  }
    // Umbral de Humedad
  if (String(topic) == "proyectoFinal/sala_servidores/config/umbral_humedad") {
    umbral_humedad = mensaje.toFloat();
    Serial.print("Nuevo umbral de humedad: ");
    Serial.println(umbral_humedad);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Mantener la conexión y recibir mensajes

  unsigned long tiempoActual = millis();
  
  // Lectura del sensor cada 5 segundos
  if (tiempoActual - tiempoAnterior >= intervaloLectura) {
    tiempoAnterior = tiempoActual;
    
    humedad = dht.readHumidity();
    temperatura = dht.readTemperature();

    if (isnan(humedad) || isnan(temperatura)) {
      Serial.println("¡Fallo al leer el sensor DHT!");
      return;
    }

    // Publicar lecturas MQTT
    Serial.print("humedad: ");
    Serial.println(humedad);
    Serial.print("temperatura: ");
    Serial.println(temperatura);
    client.publish("proyectoFinal/sala_servidores/sensores/temperatura", String(temperatura).c_str());
    client.publish("proyectoFinal/sala_servidores/sensores/humedad", String(humedad).c_str());

    // Sistema automático
    // ventilador encendido: LED azul
    // ventilador apagado: LED rojo
    if (modo_automatico) {
      if (temperatura > umbral_temperatura) {
        // Enciende "ventilador" 
        digitalWrite(PIN_AZUL, HIGH);
        digitalWrite(PIN_ROJO, LOW);
        digitalWrite(PIN_VERDE, LOW);
        if (!ventilador_encendido) {
          client.publish("proyectoFinal/sala_servidores/estado/ventilador", "ON", true);
          client.publish("proyectoFinal/sala_servidores/eventos/alertas", "ALERTA: Temperatura demasiado alta. VENTILADOR ENCENDIDO.");
          ventilador_encendido = true;
        }
      } else if (humedad > umbral_humedad){
        // Enciende "ventilador" 
        digitalWrite(PIN_AZUL, HIGH);
        digitalWrite(PIN_ROJO, LOW);
        digitalWrite(PIN_VERDE, LOW);
        if (!ventilador_encendido) {
          client.publish("proyectoFinal/sala_servidores/estado/ventilador", "ON", true);
          client.publish("proyectoFinal/sala_servidores/eventos/alertas", "ALERTA: Humedad demasiado alta. VENTILADOR ENCENDIDO.");
          ventilador_encendido = true;
        }
      }else {
        // Apaga "ventilador" 
        digitalWrite(PIN_AZUL, LOW);
        digitalWrite(PIN_ROJO, HIGH);
        digitalWrite(PIN_VERDE, LOW);
        if (ventilador_encendido) {
          client.publish("proyectoFinal/sala_servidores/estado/ventilador", "OFF", true);
          client.publish("proyectoFinal/sala_servidores/eventos/alertas", "Normal");
          ventilador_encendido = false;
        }
      }
    }
  }
}
