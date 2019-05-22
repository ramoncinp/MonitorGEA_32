#include <Arduino.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_FT6206.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoOTA.h>
#include "BaseDeDatosGEA.h"
#include "MemoryManager.h"

#define LED 2
//Pins de LCD
#define TFT_DC 5
#define TFT_CS 4
#define TFT_RST 19
//Pin interrupcion Touch
#define TOUCH_INT 39
//Pines de transceiver
#define TX_ENABLE 14
#define RX_ENABLE 13
//Pines de Serial2
#define TXD2 16
#define RXD2 17

//Constantes de tiempos
const int REFRESH_SENSORS_DATA_DELAY = 5 * 60000; //5 minutos

//Declaración de funciones
String getCurrentTime();
String handleSerial();
time_t getEpochTime();
void connectToWifi();
void evaluateChoseRect(int x, int y);
void handleDbData();
void handleSaveMemoryData();
void handleTouch();
void initOTA();
void initPins();
void drawMainScreen();
void drawGasScreen();
void drawQScreen();
void drawElecScreen();
void getMemoryData();
void printCurrentTime(String time);
void saveData();
void setGasData();
void setElecData();
void setAguaData();
void txRxToModules();
void tooglePin();
void testCircles();
unsigned long testFillScreen();

//Coordenadas de cuadros
int startRec1X, startRec1Y, endRec1X, endRec1Y;
int startRec2X, startRec2Y, endRec2X, endRec2Y;
int startRec3X, startRec3Y, endRec3X, endRec3Y;

//Coordenadas de datos de sensores en pantalla principal
int startXGasData, startYGasData;
int startXElecData, startYElecData;
int startXAguaData, startYAguaData;

//Variables
bool inMainScreen = true;
bool inQScreen = false;
bool inGasScreen = false;
bool inElecScreen = false;
String currentTime;
volatile bool touched = false;

//Referencias de tiempo
unsigned long touchDebounceRef;
unsigned long showCurrentTimeRef;
unsigned long updaeFirebaseDataRef;
unsigned long acumuladorCaudalRef;
unsigned long acumuladorPotenciaRef;
unsigned long mostrarValor;

//Sensores
float caudalVal = 0, litros = 0, litrosRef = 0;
float potInst = 0, potAcc = 0, potAccRef = 0;
int nivelGas = 0;

//Objetos
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_FT6206 ts = Adafruit_FT6206();
BaseDeDatosGEA db;
MemoryManager memoryManager;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

//Funcion para manejar interrupcion externa
void IRAM_ATTR handleTouchInterrupt()
{
  touched = true;
}

void setup()
{
  //Inicializar comunicacion serial
  Serial.begin(9600);

  //Inicializar comunicacion serial para bus485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  //Inicializar pines
  initPins();

  //Inicializar manejador de memoria
  memoryManager.begin();

  //Conectarse a WiFi
  connectToWifi();

  //Inicializar LCD
  tft.begin();
  tft.setRotation(1);

  //Inicializar touch
  if (!ts.begin(40))
  {
  }
  else
  {
  }

  //Dibujar pantalla principal
  drawMainScreen();

  //Obtener datos
  getMemoryData();
}

void loop()
{
  //Toogle Pin
  tooglePin();

  //Manejar touch
  handleTouch();

  //Manejar Comunicacion con modulos
  txRxToModules();

  //Evaluar el guardar datos
  //handleSaveMemoryData();

  //Imprimir la hora actual cada 500 ms
  if (millis() - showCurrentTimeRef > 500)
  {
    showCurrentTimeRef = millis();
    if (inMainScreen)
      printCurrentTime(getCurrentTime());
  }

  //Actualizar datos en firebase
  if (millis() - updaeFirebaseDataRef > REFRESH_SENSORS_DATA_DELAY)
  {
    updaeFirebaseDataRef = millis();
    //handleDbData();
  }

  /*if (millis() - acumuladorPotenciaRef > 2000)
  {
    potAcc += (potInst * (millis() - acumuladorPotenciaRef)) / 3600000.000;

    if (inElecScreen)
    {
      drawElecScreen();
    }
    else if (inMainScreen)
    {
      setElecData();
    }

    //Tomar referencia
    acumuladorPotenciaRef = millis();
  }*/

  //Manejar OTA
  ArduinoOTA.handle();
}

/** Definición de Funciones **/
void initPins()
{
  pinMode(LED, OUTPUT);

  //Definir pines de transceiver
  pinMode(TX_ENABLE, OUTPUT);
  pinMode(RX_ENABLE, OUTPUT);

  //Inicializar como transmisor
  digitalWrite(TX_ENABLE, HIGH); //Habilitar TX
  digitalWrite(RX_ENABLE, HIGH); //Deshabilitar RX

  //Pin de interrupcion touch
  pinMode(TOUCH_INT, INPUT_PULLUP);
  attachInterrupt(TOUCH_INT, handleTouchInterrupt, FALLING);
}

void tooglePin()
{
  static unsigned long timeRef;

  if (millis() - timeRef > 500)
  {
    digitalWrite(LED, !digitalRead(LED));
    timeRef = millis();
  }
}

void drawMainScreen()
{
  tft.fillScreen(ILI9341_BLACK);

  //Escribir título
  tft.setCursor((tft.width() / 3) - 8, 6);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("Monitor GEA");

  //Escribir hora
  printCurrentTime(getCurrentTime());

  //Obtener coordenadas
  //Rectangulos
  startRec1X = 6;
  startRec1Y = 28;
  endRec1X = startRec1X + (tft.width() - 18) / 3;
  endRec1Y = startRec1Y + tft.height() - 36;

  startRec2X = endRec1X + 6;
  startRec2Y = 28;
  endRec2X = startRec2X + (tft.width() - 18) / 3;
  endRec2Y = startRec2Y + tft.height() - 36;

  startRec3X = endRec2X + 6;
  startRec3Y = 28;
  endRec3X = startRec3X + (tft.width() - 18) / 3;
  endRec3Y = startRec3Y + tft.height() - 36;

  //Titulos de variables
  int centerText1X = startRec1X + 28;
  int centerText1Y = 36;

  int centerText2X = startRec2X + 28;
  int centerText2Y = 36;

  int centerText3X = startRec3X + 28;
  int centerText3Y = 36;

  //Valores de los sensores
  startXGasData = startRec1X + 8;
  startYGasData = startRec1Y + 80;
  startXElecData = startRec2X + 8;
  startYElecData = startRec2Y + 80;
  startXAguaData = startRec3X + 8;
  startYAguaData = startRec3Y + 80;

  //Dibujar Rectangulos
  tft.fillRect(startRec1X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_RED);
  tft.drawRect(startRec1X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  tft.fillRect(startRec2X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_GREEN);
  tft.drawRect(startRec2X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  tft.fillRect(startRec3X, startRec3Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_YELLOW);
  tft.drawRect(startRec3X, startRec3Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  //Dibujar titulos
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(centerText1X, centerText1Y);
  tft.println("Gas");

  tft.setCursor(centerText2X, centerText2Y);
  tft.println("Elec.");

  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(centerText3X, centerText3Y);
  tft.println("Agua");

  //Dibujar valores de sensores
  setGasData();
  setElecData();
  setAguaData();

  inMainScreen = true;
}

void setGasData()
{
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
  tft.setCursor(startXGasData, startYGasData);
  tft.printf("%d%c", nivelGas, '%');
  tft.setCursor(startXGasData, startYGasData + 16);
  tft.printf("%c%d", '$', 500);
}

void setElecData()
{
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_GREEN);
  tft.setCursor(startXElecData, startYElecData);
  tft.printf("%dkW/h", (int)potAcc);
  tft.setCursor(startXElecData, startYElecData + 16);
  tft.printf("%c%d", '$', 224);
}

void setAguaData()
{
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_YELLOW);
  tft.setCursor(startXAguaData, startYAguaData);
  tft.printf("%dL", (int)litros);
  tft.setCursor(startXAguaData, startYAguaData + 16);
  tft.printf("%c%d", '$', 149);
}

void printCurrentTime(String time)
{
  tft.setCursor(262, 9);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print(time);
}

void handleTouch()
{
  if (touched && millis() - touchDebounceRef > 200)
  {
    //Obtener coordenada
    TS_Point p = ts.getPoint();

    if (p.x != 0 && p.x != 0)
    {
      int realY = tft.height() - p.x;
      int realX = p.y;

      touchDebounceRef = millis();

      if (inMainScreen)
      {
        evaluateChoseRect(realX, realY);
      }
      else
      {
        inQScreen = false;
        inGasScreen = false;
        inElecScreen = false;
        drawMainScreen();
      }
    }

    touched = false;
  }
}

void evaluateChoseRect(int x, int y)
{
  if (x >= startRec1X && x <= endRec1X &&
      y >= startRec1Y && y <= endRec1Y)
  {
    drawGasScreen();
    inMainScreen = false;
    inQScreen = false;
    inElecScreen = false;
    inGasScreen = true;
  }
  else if (x >= startRec2X && x <= endRec2X &&
           y >= startRec2Y && y <= endRec2Y)
  {
    drawElecScreen();
    inMainScreen = false;
    inGasScreen = false;
    inQScreen = false;
    inElecScreen = true;
  }
  else if (x >= startRec3X && x <= endRec3X &&
           y >= startRec3Y && y <= endRec3Y)
  {
    drawQScreen();
    inMainScreen = false;
    inGasScreen = false;
    inElecScreen = false;
    inQScreen = true;
  }
}

String handleSerial()
{
  String message = "";

  if (Serial2.available())
  {
    while (Serial2.available())
    {
      char mChar = (char)Serial2.read();
      if (mChar != 0 && mChar != ' ')
      {
        if (mChar > '9')
        {
          mChar = '0';
        }
        message += mChar;
      }
      delay(2);
    }
  }

  return message;
}

void drawGasScreen()
{
  tft.fillScreen(ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor((tft.width() / 2) - 24, (tft.height() / 2) - 8);
  tft.printf("%d%c", nivelGas, '%');
}

void drawQScreen()
{
  tft.fillScreen(ILI9341_YELLOW);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(0, (tft.height() / 2) - 16);
  tft.printf("  %.3f mL/m\n  %.3f L", caudalVal, litros);
}

void drawElecScreen()
{
  tft.fillScreen(ILI9341_GREEN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(0, (tft.height() / 2) - 16);
  tft.printf("  %.3f W\n  %.3f kW/h", potInst, potAcc);
}

void txRxToModules()
{
  //Variable que indica el dato que estamos recolectando
  static int sensorType = 2; //Agua por default

  //Verdadero para enviar, falso para recibir
  static bool sendOrReceive = true;

  //Referencia para solicitar datos
  static unsigned long timeRef;

  if (sendOrReceive)
  {
    //Enviar a cada ms
    if (millis() - timeRef > 50)
    {
      //Preparar bits para transmitir
      digitalWrite(TX_ENABLE, HIGH);
      digitalWrite(RX_ENABLE, HIGH);

      switch (sensorType)
      {
      case 0: //Pedir valor de gas
        Serial2.print('2');
        Serial.println("Pidiendo valor de gas");
        break;

      case 1: //Pedir valor de potencia electrica
        Serial2.print('@');
        break;

      case 2: //Pedir valor de caudal
        Serial2.print('0');
        Serial.println("Pidiendo valor de caudal");
        break;

      default:
        break;
      }
      delay(1);

      //Esperar respuesta
      sendOrReceive = false;
    }
  }
  else
  {
    //Preparar bits para recibir
    digitalWrite(TX_ENABLE, LOW);
    digitalWrite(RX_ENABLE, LOW);

    //Esperar mensaje
    String message = handleSerial();

    //Validar si se recibió un mensaje
    if (message != "")
    {
      //Evaluar el dato que se espera recibir
      switch (sensorType)
      {
      case 0: //Obtener valor de gas
        Serial.print("Llego dato de gas -> ");
        Serial.println(message);

        //Valor entrante
        int nuevoValor;
        nuevoValor = message.toInt();

        if (nuevoValor >= 0 && nuevoValor <= 100)
        {
          //Evaluar si es diferente al actual
          if (nuevoValor != nivelGas)
          {
            Serial.println("Cambio valor de gas, guardar...");
            //Guardar en memoria el nuevo valor
            memoryManager.saveData(nuevoValor, NIVEL_GAS_ADDR);
          }
        }

        //Convertir valor
        nivelGas = nuevoValor;
        //Mostrar valor si esta en pantalla de gas
        if (inGasScreen)
        {
          drawGasScreen();
        }
        else if (inMainScreen)
        {
          setGasData();
        }

        //Cambiar a pedir agua
        sensorType = 2;
        break;

      case 1: //Obtener valor de potencia eléctrica
        //Acumular a totalizador de potencia
        potAcc += (potInst * (millis() - acumuladorPotenciaRef)) / 3600000.0;
        //Obtener potencia instantanea en string
        potInst = message.toFloat();

        //Mostrar valores
        if (inElecScreen)
        {
          drawElecScreen();
        }
        else if (inMainScreen)
        {
          setElecData();
        }
        //Comenzar a contar tiempo de potencia actual
        acumuladorPotenciaRef = millis();

        break;

      case 2: //Obtener valor de agua
        Serial.print("Llego dato de agua -> ");
        Serial.println(message);

        //Acumular a totalizador de litros
        litros += (caudalVal / 1000.000) * ((millis() - acumuladorCaudalRef) / 60000.000);
        //Convertir caudal a float
        caudalVal = message.toFloat();
        //Si la ventana actual es la de caudal, refrescar
        if (inQScreen)
        {
          drawQScreen();
        }
        else if (inMainScreen)
        {
          setGasData();
        }

        //Comenzar a contar tiempo de caudal actual
        acumuladorCaudalRef = millis();
        //Cambiar a pedir gas
        sensorType = 0;

        break;
      }

      //Finalizar proceso
      sendOrReceive = true;
      timeRef = millis();
    }

    //Revisar si sucede un timeout
    if (millis() - timeRef > 3000)
    {
      Serial.println("Time out!");

      switch (sensorType)
      {
      case 0:
        sensorType = 2;
        break;

      case 2:
        sensorType = 0;
        break;

      default:
        break;
      }

      sendOrReceive = true;
      timeRef = millis();
    }
  }
}

void handleSaveMemoryData()
{
  static unsigned long timeRef;

  if (millis() - timeRef > 60000)
  {
    //Evaluar caudal
    if (caudalVal > 0)
    {
      //Guardar si hay caudal
      memoryManager.saveData(litros, LITROS_ADDR);
    }

    //Evaluar potencia instantánea
    if (potInst > 0)
    {
      //Si hay consumo de potencia actual, guardar
      memoryManager.saveData(potAcc, POTENCIA_ADDR);
    }

    //El nivel de gas se almacena a cada cambio
    //Tomar referencia de tiempo
    timeRef = millis();
  }
}

void connectToWifi()
{
  const char *ssid = "ARRIS-1EC2";
  const char *password = "17A33538439C84A4";

  Serial.println("Conectando a WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  Serial.println("Conectado a WiFi!");

  //Configurar reloj
  configTime(0, 0, "pool.ntp.org");

  //Inicializar OTA
  initOTA();
}

time_t getEpochTime()
{
  time_t now;
  time(&now);

  return now;
}

void initOTA()
{
  //Definir nombre de OTA
  ArduinoOTA.setHostname("MonitorGEA");
  //Definir password
  ArduinoOTA.setPassword("1234");
  //Inicializar OTA
  ArduinoOTA.begin();
}

String getCurrentTime()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
  {
    return "";
  }

  //Ajustar zona horaria
  for (int i = 0; i < 5; i++)
  {
    timeinfo.tm_hour--;
    if (timeinfo.tm_hour == -1)
      timeinfo.tm_hour = 23;
  }

  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);

  String asString(timeStringBuff);
  return timeStringBuff;
}

//Obtener los valores almacenados en memoria
void getMemoryData()
{
  litros = memoryManager.getData(LITROS_ADDR);
  litrosRef = memoryManager.getData(LITROS_REF_ADDR);
  potAcc = memoryManager.getData(POTENCIA_ADDR);
  potAccRef = memoryManager.getData(POTENCIA_REF_ADDR);
  nivelGas = memoryManager.getData(NIVEL_GAS_ADDR);
}

//Actualizar valores de base de datos
void handleDbData()
{
  static int op = 0;

  switch (op)
  {
  case 0:
    potInst += 0.10;
    db.actualizarSensores(nivelGas, potInst, ++caudalVal, ++potAcc, ++litros);
    op++;
    break;

  case 1:
    db.agregarRegistroGas(nivelGas, getEpochTime());
    op = 0;
    break;
  }
}