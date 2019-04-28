#include <Arduino.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_FT6206.h>
#include <WiFi.h>
#include <time.h>
#include "BaseDeDatosGEA.h"

#define LED 2
//Pins de LCD
#define TFT_DC 5
#define TFT_CS 4
#define TFT_RST 19
//Pin interrupcion Touch
#define TOUCH_INT 25
//Pines de transceiver
#define TX_ENABLE 14
#define RX_ENABLE 13

//Constantes de tiempos
const int REFRESH_SENSORS_DATA_DELAY = 12000;

//Declaración de funciones
String getCurrentTime();
String handleSerial();
time_t getEpochTime();
void connectToWifi();
void evaluateChoseRect(int x, int y);
void handleDbData();
void handleTouch();
void initPins();
void drawMainScreen();
void drawQScreen();
void printCurrentTime(String time);
void txRxToModules();
void tooglePin();
void testCircles();
unsigned long testFillScreen();

//Coordenadas de cuadros
int startRec1X, startRec1Y, endRec1X, endRec1Y;
int startRec2X, startRec2Y, endRec2X, endRec2Y;
int startRec3X, startRec3Y, endRec3X, endRec3Y;

//Variables
bool inMainScreen = true;
bool inQScreen = false;
String currentTime;
unsigned long touchDebounceRef;
unsigned long showCurrentTimeRef;
unsigned long updaeFirebaseDataRef;
volatile bool touched = false;

//Sensores
String caudal = "";
double caudalVal = 500, litros = 10;
double potInst = 3.24, potAcc = 5.10; 
int nivelGas = 15;

//Objetos
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_FT6206 ts = Adafruit_FT6206();
BaseDeDatosGEA db;
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

  //Inicializar pines
  initPins();

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
}

void loop()
{
  //Toogle Pin
  tooglePin();

  //Manejar touch
  handleTouch();

  //Manejar Comunicacion con modulos
  txRxToModules();

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
    handleDbData();
  }
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

  if (millis() - timeRef > 250)
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

  //Dibujar Elementos
  startRec1X = 6;
  startRec1Y = 28;
  endRec1X = startRec1X + (tft.width() - 18) / 3;
  endRec1Y = startRec1Y + tft.height() - 36;
  tft.fillRect(startRec1X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_RED);
  tft.drawRect(startRec1X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  startRec2X = endRec1X + 6;
  startRec2Y = 28;
  endRec2X = startRec2X + (tft.width() - 18) / 3;
  endRec2Y = startRec2Y + tft.height() - 36;
  tft.fillRect(startRec2X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_GREEN);
  tft.drawRect(startRec2X, startRec1Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  startRec3X = endRec2X + 6;
  startRec3Y = 28;
  endRec3X = startRec3X + (tft.width() - 18) / 3;
  endRec3Y = startRec3Y + tft.height() - 36;
  tft.fillRect(startRec3X, startRec3Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_YELLOW);
  tft.drawRect(startRec3X, startRec3Y, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  //Dibujar textos
  tft.setTextSize(1);

  int centerText1X = startRec1X + 28;
  int centerText1Y = (tft.height() / 2) + 8;

  int centerText2X = startRec2X + 28;
  int centerText2Y = centerText1Y;

  int centerText3X = startRec3X + 28;
  int centerText3Y = centerText1Y;

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(centerText1X + 8, centerText1Y);
  tft.println("Gas");

  tft.setCursor(centerText2X + 8, centerText2Y);
  tft.println("Elec.");

  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(centerText3X + 8, centerText3Y);
  tft.println("Agua");

  inMainScreen = true;
}

void printCurrentTime(String time)
{
  tft.setCursor(262, 9);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print(time);
}

unsigned long testFillScreen()
{
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  delay(100);
  tft.fillScreen(ILI9341_RED);
  delay(100);
  tft.fillScreen(ILI9341_GREEN);
  delay(100);
  tft.fillScreen(ILI9341_BLUE);
  delay(100);
  tft.fillScreen(ILI9341_BLACK);
  delay(100);
  return micros() - start;
}

void testCircles()
{
  static int radius = 2;
  static int x = tft.width() / 2;
  static int y = tft.height() / 2;
  static bool incrementDecrement = true;

  tft.fillScreen(ILI9341_BLACK);
  tft.drawCircle(x, y, radius, ILI9341_NAVY);

  if (incrementDecrement)
  {
    radius += 3;
    if (radius > tft.width() / 2)
    {
      radius = tft.width() / 2;
      incrementDecrement = false;
    }
  }
  else
  {
    radius -= 3;
    if (radius < 2)
    {
      radius = 2;
      incrementDecrement = true;
    }
  }
  delay(5);
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
    tft.fillScreen(ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.setCursor((tft.width() / 2) - 24, (tft.height() / 2) - 8);
    tft.print(String(nivelGas) + "%");

    inMainScreen = false;
  }
  else if (x >= startRec2X && x <= endRec2X &&
           y >= startRec2Y && y <= endRec2Y)
  {
    tft.fillScreen(ILI9341_GREEN);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.setCursor(tft.width() / 3, (tft.height() / 2) - 8);
    tft.print("1 kW/h");

    inMainScreen = false;
  }
  else if (x >= startRec3X && x <= endRec3X &&
           y >= startRec3Y && y <= endRec3Y)
  {
    drawQScreen();
    inMainScreen = false;
    inQScreen = true;
  }
}

String handleSerial()
{
  String message = "";

  if (Serial.available())
  {
    while (Serial.available())
    {
      char mChar = (char)Serial.read();
      if (mChar != 0 && mChar != ' ')
      {
        message += mChar;
      }
      delay(2);
    }
  }

  return message;
}

void drawQScreen()
{
  tft.fillScreen(ILI9341_YELLOW);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(tft.width() / 3, (tft.height() / 2) - 8);
  tft.print(caudal);
}

void txRxToModules()
{
  //Verdadero para enviar, falso para recibir
  static bool sendOrReceive = false;

  //Referencia para solicitar datos
  static unsigned long timeRef;

  if (sendOrReceive)
  {
    //Enviar a cada ms
    if (millis() - timeRef > 30)
    {
      //Preparar bits para transmitir
      digitalWrite(TX_ENABLE, HIGH);
      digitalWrite(RX_ENABLE, HIGH);

      //Pedir dato a Caudal
      Serial.print('0');
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
      caudal = message;
      caudal += " L/m";

      //Si la ventana actual es la de caudal, refrescar
      if (inQScreen)
        drawQScreen();

      //Finalizar proceso
      sendOrReceive = true;
      timeRef = millis();
    }

    //Revisar si sucede un timeout
    if (millis() - timeRef > 1500)
    {
      sendOrReceive = true;
      timeRef = millis();
    }
  }
}

void connectToWifi()
{
  const char *ssid = "ARRIS-1EC2";
  const char *password = "17A33538439C84A4";

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  //Configurar reloj
  configTime(0, 0, "pool.ntp.org");
}

time_t getEpochTime()
{
  time_t now;
  time(&now);

  return now;
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

void handleDbData()
{
  static int op = 0;

  switch (op)
  {
  case 0:
    potInst += 0.10;
    db.actualizarSensores(++nivelGas, potInst, ++caudalVal, ++potAcc, ++litros);
    op++;
    break;

  case 1:
    db.agregarRegistroGas(nivelGas, getEpochTime());
    op = 0;
    break;
  }
}