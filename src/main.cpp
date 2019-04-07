#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define LED 2

//Pins de LCD
#define TFT_DC 5
#define TFT_CS 4
#define TFT_RST 17

//Declaración de funciones
void initPins();
void drawMainScreen();
void tooglePin();
void testCircles();
unsigned long testFillScreen();

//Objetos
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup()
{
  //Inicializar pines
  initPins();

  //Inicializar LCD
  tft.begin();

  //Inicializar comunicacion serial
  Serial.begin(9600);

  //Tests de pantalla
  testFillScreen();
  drawMainScreen();
}

void loop()
{
  //Toogle Pin
  tooglePin();
}

/** Definición de Funciones **/

void initPins()
{
  pinMode(LED, OUTPUT);
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
  //Definir rotaion
  tft.setRotation(1);

  //Mostrar dimensiones
  Serial.print("\n\n\n\n\n");
  Serial.print("Width -> ");
  Serial.println(tft.width());
  Serial.print("Height -> ");
  Serial.println(tft.height());

  //Escribir título
  tft.setCursor((tft.width() / 3) - 8, 6);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Monitor GEA");

  //Dibujar Elementos
  int centerRec1X = 6;
  tft.fillRect(centerRec1X, 28, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_BLUE);
  tft.drawRect(centerRec1X, 28, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  int centerRec2X = centerRec1X + (tft.width() - 18) / 3 + 6;
  tft.fillRect(centerRec2X, 28, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_BLUE);
  tft.drawRect(centerRec2X, 28, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  int centerRec3X = centerRec2X + (tft.width() - 18) / 3 + 6;
  tft.fillRect(centerRec3X, 28, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_BLUE);
  tft.drawRect(centerRec3X, 28, (tft.width() - 18) / 3, tft.height() - 36, ILI9341_WHITE);

  //Dibujar textos
  tft.setTextSize(1);

  int centerText1X = centerRec1X + 28;
  int centerText1Y = (tft.height() / 2) + 8;

  int centerText2X = centerRec2X + 28;
  int centerText2Y = centerText1Y;

  int centerText3X = centerRec3X + 28;
  int centerText3Y = centerText1Y;

  tft.setCursor(centerText1X + 8, centerText1Y);
  tft.println("Gas");

  tft.setCursor(centerText2X + 8, centerText2Y);
  tft.println("Elec.");

  tft.setCursor(centerText3X + 8, centerText3Y);
  tft.println("Agua");
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