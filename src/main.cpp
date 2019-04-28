#include <Arduino.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_FT6206.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

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

//Declaración de funciones
String getCurrentTime();
String handleSerial();
time_t getEpochTime();
void connectToWifi();
void evaluateChoseRect(int x, int y);
void firebaseRequest();
void handleTouch();
void initPins();
void drawMainScreen();
void drawQScreen();
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
unsigned long touchDebounceRef = 0;
unsigned long epoch;
volatile bool touched = false;

//Sensores
String caudal = "";

//Objetos
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_FT6206 ts = Adafruit_FT6206();
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

  //Conectarse a WiFi
  connectToWifi();

  //Conectarse con la base de datos
  firebaseRequest();
}

void loop()
{
  //Toogle Pin
  tooglePin();

  //Manejar touch
  handleTouch();

  //Manejar Comunicacion con modulos
  txRxToModules();
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
  tft.println("Monitor GEA ");

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

  tft.setCursor(centerText1X + 8, centerText1Y);
  tft.println("Gas");

  tft.setCursor(centerText2X + 8, centerText2Y);
  tft.println("Elec.");

  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(centerText3X + 8, centerText3Y);
  tft.println("Agua");

  inMainScreen = true;
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
    tft.print("15%");

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
    Serial.println("Conectando a la red...");
  }

  Serial.println("Conectado :D!");

  //Configurar reloj
  //configTime(0, 0, "pool.ntp.org");
}

void firebaseRequest()
{
  const char *root_ca =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIEGDCCAwCgAwIBAgIQI+/QKD6ld0Sb9j5ZiOHrwzANBgkqhkiG9w0BAQsFADCB\n"
      "gTE6MDgGA1UECwwxZ2VuZXJhdGVkIGJ5IEF2YXN0IEFudGl2aXJ1cyBmb3IgU1NM\n"
      "L1RMUyBzY2FubmluZzEeMBwGA1UECgwVQXZhc3QgV2ViL01haWwgU2hpZWxkMSMw\n"
      "IQYDVQQDDBpBdmFzdCBXZWIvTWFpbCBTaGllbGQgUm9vdDAeFw0xOTAzMTMyMTMw\n"
      "NTdaFw0yMDAzMTEyMTMwNTdaMGgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpDYWxp\n"
      "Zm9ybmlhMRYwFAYDVQQHEw1Nb3VudGFpbiBWaWV3MRMwEQYDVQQKEwpHb29nbGUg\n"
      "TExDMRcwFQYDVQQDEw5maXJlYmFzZWlvLmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
      "ggEPADCCAQoCggEBALPLKPeoCQYm8JoHVI/6gZ8KMR4VXYJ7h56gjKv0KmyBgg7E\n"
      "LLsTe2PtgaouiQFG5nPBr5ctwKCDqoEBcSI8UhiMxsDwkQMXrvff79qTeTG+ZE4G\n"
      "C3R0I9eu0cO5t3YWJgqVGI2CAzNDjt//IaRIMcRGGELB5Gsbv6HYh5wfBspEOisn\n"
      "NoIwKyvfFIBUcN26s7A53mIZ9DuXl5CzKevBWbQ4kbhREhzfPadtkgp4zVHt8gRk\n"
      "rtyvIpEWWY+hEv+TW6jQtw/SdOczWd+OzlIpOkhaIzlYBV3wPEwQUvoRvjJRHvoL\n"
      "Zl86ElSsGAH4efiFKk1qK8eOf/K7Ux87c409LuUCAwEAAaOBozCBoDAOBgNVHQ8B\n"
      "Af8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDAYDVR0TAQH/BAIwADAdBgNV\n"
      "HQ4EFgQUtN2IVe9IOvuzFMbTopbkYtU9hgMwHwYDVR0jBBgwFoAUQFLnnEmlC9Fr\n"
      "e7ITUsPZj7LT03AwKwYDVR0RBCQwIoIOZmlyZWJhc2Vpby5jb22CECouZmlyZWJh\n"
      "c2Vpby5jb20wDQYJKoZIhvcNAQELBQADggEBAMPJppLHCVlGNg6NZcSKcVhYK3RX\n"
      "Xv6oOf6TAu+x3R/85+bsVTWcIUNt7N0DfVWmh524lSB6EFkssnroEVx1PMujgLKd\n"
      "kHEYzJGA83uf2VFXE3Yw4uKC9smke8eITisAjrA6b/93r6YH5v8994ePrWiCgpq2\n"
      "pSeH3BEq/+XLswi3V6P6jnnayghxU0re5vEHF1BHTNE1OTIPwwkMyeDiWMtWdgC3\n"
      "buqmGhQ6e6C0GH9ebWIshN8SAs+dTY6plVrWBV79FDTt6cFjNKuzrDowVKhxETJF\n"
      "hjOwrPfeTkC2XCOoZntU+7A9c9C8Or+UyWMDPbwD/rulVFqFwfM2yw5qHvI=\n"
      "-----END CERTIFICATE-----\n";

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Obtener datos de sensores...");

    HTTPClient http;

    http.begin("https://monitorgea.firebaseio.com/sensores.json", root_ca);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      String payload = http.getString();
      Serial.println(payload);
    }
    else
    {
      Serial.println("Error on HTTP request");
    }

    http.end();
  }
}

time_t getEpochTime()
{
  time_t now;
  time(&now);

  return now;
}

String getCurrentTime()
{
  char mTime[7];
  struct tm timeinfo;

  if (getLocalTime(&timeinfo))
  {
    snprintf(mTime, sizeof(mTime), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(mTime);
  }
  else
  {
    return "";
  }
}