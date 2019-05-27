/** LIBRERIAS UTILIZADAS **/
#include <Arduino.h> //Framework de arduino
#include <SPI.h> //Comunicacion SPI
#include <HardwareSerial.h> 
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h" //Liberías para manejo de la pantalla
#include <Adafruit_FT6206.h> //Manejo del touch
#include <WiFi.h> //Manejo de la capa WiFi
#include <time.h> //Librería para obtener el tiempo
#include "BaseDeDatosGEA.h" //Librería para conexión con base de datos


/** DEFINICION DE CONSTANTES **/
#define LED 2 //Led de actividad
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
#define TXD2 17
#define RXD2 16

//Constantes para enviar datos a la base datos
const int REFRESH_SENSORS_DATA_DELAY = 20000; //20 segundos

//Declaración de funciones
String getCurrentTime();
String handleSerial();
time_t getEpochTime();
void connectToWifi();
void evaluateChoseRect(int x, int y);
void getBdData();
void handleDbData();
void handleTouch();
void initPins();
void drawLoadingScreen();
void drawMainScreen();
void drawGasScreen();
void drawQScreen();
void drawElecScreen();
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

//Sensores
float caudalVal = 0, litros = 0;
float potInst = 0, potAcc = 0;
int nivelGas = 0;

//Objetos
//Inicializar driver de TFT
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
//Inicializar driver touch
Adafruit_FT6206 ts = Adafruit_FT6206();
//Instancia de manejador de base de datos
BaseDeDatosGEA db;
//Instancia para manejar interrupción externa
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

//Funcion para manejar interrupcion externa
void IRAM_ATTR handleTouchInterrupt()
{
  //Activar bandera que indica que ocurrió una interrupción
  touched = true;
}

/** FUNCIONES DE INICIALIZACION **/
void setup()
{
  //Inicializar comunicacion serial
  Serial.begin(9600);

  //Inicializar comunicacion serial para bus485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  //Inicializar pines
  initPins();

  //Inicializar LCD
  tft.begin();
  tft.setRotation(1);

  //Mostrar pantalla de "Cargando..."
  drawLoadingScreen();

  //Conectarse a WiFi
  connectToWifi();

  //Obtener datos iniciales
  getBdData();

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

/** FUNCION DE CICLO INFINITO **/
void loop()
{
  //Manejar Comunicacion con modulos
  txRxToModules();

  //Manejar led de actividad
  tooglePin();

  //Manejar touch
  handleTouch();

  //Imprimir la hora actual cada 500 ms
  if (millis() - showCurrentTimeRef > 500)
  {
    //Obtener referencia de tiempo
    showCurrentTimeRef = millis();
    //Actualizar reloj cuando este la pantalla principal
    if (inMainScreen)
      printCurrentTime(getCurrentTime());
  }

  //Actualizar datos en firebase
  if (millis() - updaeFirebaseDataRef > REFRESH_SENSORS_DATA_DELAY)
  {
    handleDbData();
    updaeFirebaseDataRef = millis();
  }
}

/** DEFINICION DE FUNCIONES **/
/**
 * Nombre: initPins
 * Parametros:
 * Retorno
 * 
 * Descripcion: Inicializa los pines de entrada y salida 
 * **/
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

/**
 * Nombre: getBdData
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Obtiene los valores iniciales alojados en la nube
 * **/
void getBdData()
{
  String mData;

  // Obtener string de los datos
  mData = db.begin();

  if (mData != "")
  {
    //Crear un buffer dinámico
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(mData);

    //Terminar si el json no fue valido
    if (!root.success())
    {
      return;
    }

    //Inicializar valores
    litros = root["agua"]["valor_actual"];
    //potAcc = root["electricidad"]["valor_actual"];
  }
}

/**
 * Nombre: tooglePin
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Parpadea el led cada 500 ms
 * **/
void tooglePin()
{
  static unsigned long timeRef;

  if (millis() - timeRef > 500)
  {
    digitalWrite(LED, !digitalRead(LED));
    timeRef = millis();
  }
}

/**
 * Nombre: drawMainScreen
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Dibuja la pantalla principal
 * en donde se muestra el menu para visualizar 
 * los datos de todos los sensores
 * **/
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

  //Indicar que se esta visualizando la pantalla principal
  inMainScreen = true;
}

/**
 * Nombre: printCurrentTime
 * Parametros: String::horaActual
 * Retorno:
 * 
 * Descripcion: Escribe la hora proporcionada 
 * en la esquina superior derecha de la pantalla 
 * principal
 * **/
void printCurrentTime(String time)
{
  tft.setCursor(262, 9);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print(time);
}

/**
 * Nombre: handleTouch
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Maneja le interacción del usuario con la pantalla
 * **/
void handleTouch()
{
  //Si hubo un evento de interrupción..
  //Y ya pasaron 200 ms desde la ultima vez..
  //Entrar 
  if (touched && millis() - touchDebounceRef > 200)
  {
    //Obtener coordenada
    TS_Point p = ts.getPoint();

    //Si las coordenadas obtenidas no son en el origen...
    if (p.x != 0 && p.x != 0)
    {
      //Obtener coordenadas deacuerdo a la orientacion actual
      int realY = tft.height() - p.x;
      int realX = p.y;

      //Obtener referencia de tiempo actual para debounce
      touchDebounceRef = millis();

      //Si esta en la pantalla principal...
      if (inMainScreen)
      {
        //Mostrar la información del sensor seleccionado
        evaluateChoseRect(realX, realY);
      }
      else
      {
        //Regresar a la pantalla principal
        inQScreen = false;
        inGasScreen = false;
        inElecScreen = false;
        drawMainScreen();
      }
    }

    //Desactivar la bandera de iterrupción
    touched = false;
  }
}

/**
 * Nombre: evaluateChoseRect
 * Parametros: int::puntoEnX, int::puntoEnY
 * Retorno:
 * 
 * Descripcion: Mostrar pantalla deacuerdo a las coordenada
 * seleccionada
 * **/
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

/**
 * Nombre: handleSerial
 * Parametros:
 * Retorno: String::mensaje recibido por puerto serial2
 * 
 * Descripcion: Espera por un dato en el puerto serial 
 * y lo lee cuando hay disponible
 * **/
String handleSerial()
{
  //Inicializar String que contenga el mensaje
  String message = "";

  //Si hay datos disponibles en Serial2...
  if (Serial2.available())
  {
    //Esperar a que lleguen todos los datos por Serial2
    delay(20);
    while (Serial2.available())
    {
      //Leer char entrante por Serial2
      char mChar = (char)Serial2.read();
      //En puerto Serial1, ver el caracter entrante
      Serial.print(mChar);
      //Validar si no es un char nulo ni un espacio... 
      if (mChar != 0 && mChar != ' ')
      {
        //Validar que no sea un caracter no numérico
        if (mChar > '9')
        {
          mChar = '0';
        }
        //Concatenar en String
        message += mChar;
      }
    }
    //Agregar un salto de linea a Serial1
    Serial.println();
  }

  return message;
}

/**
 * Nombre: drawLoadingScreen
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Mostrar pantalla indicando 
 * que se estan obteniendo los datos
 * **/
void drawLoadingScreen()
{
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0, (tft.height() / 2) - 16);
  tft.print("  Obteniendo datos...");
}

/**
 * Nombre: drawGasScreen
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Mostrar nivel actual de gas
 * **/
void drawGasScreen()
{
  tft.fillScreen(ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor((tft.width() / 2) - 24, (tft.height() / 2) - 8);
  tft.printf("%d%c", nivelGas, '%');
}

/**
 * Nombre: drawQScreen
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Muestra el valor actual de caudal
 * y los litros acumulados
 * **/
void drawQScreen()
{
  tft.fillScreen(ILI9341_YELLOW);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(0, (tft.height() / 2) - 16);
  tft.printf("  %.3f mL/m\n  %.3f L", caudalVal, litros);
}

/**
 * Nombre: drawElecScreen
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Muestra los datos 
 * actuales de potencia y de kW/h
 * **/
void drawElecScreen()
{
  tft.fillScreen(ILI9341_GREEN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(0, (tft.height() / 2) - 16);
  tft.printf("  %.3f W\n  %.3f kW/h", potInst, potAcc);
}

/**
 * Nombre: txRxToModules
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Maneja todo el ciclo para 
 * pedir un dato a un sensor y obtener su respuesta
 * **/
void txRxToModules()
{
  //Variable que indica el dato que estamos recolectando
  static int sensorType = 0; //Agua por default

  //Verdadero para enviar, falso para recibir
  static bool sendOrReceive = false;

  //Referencia para solicitar datos
  static unsigned long timeRef;

  //Si es verdadero... envia
  if (sendOrReceive)
  {
    //Enviar a cada 50ms
    if (millis() - timeRef > 50)
    {
      //Preparar bits para transmitir
      digitalWrite(TX_ENABLE, HIGH);
      digitalWrite(RX_ENABLE, HIGH);

      switch (sensorType)
      {
      case 0: //Pedir valor de gas
        Serial2.print('2');
        break;

      case 1: //Pedir valor de potencia electrica
        Serial2.print('1');
        break;

      case 2: //Pedir valor de caudal
        Serial2.print('0');
        break;

      default:
        break;
      }
      //Esperar 4ms para continuar
      delay(4);

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
        //Convertir valor
        nivelGas = message.toInt();
        if (nivelGas > 100) //Agregar tope
          nivelGas = 0;
        
        //Mostrar valor si esta en pantalla de gas
        if (inGasScreen)
        {
          drawGasScreen();
        }

        //Cambiar a pedir agua
        sensorType = 1;
        break;

      case 1: //Obtener valor de potencia eléctrica
        Serial.println("Llego potencia");
        //Acumular a totalizador de potencia
        potAcc += (potInst * (millis() - acumuladorPotenciaRef)) / 3600000.0;
        //Convertir valor a float
        potInst = message.toFloat();
        //Evaluar si esta en la pantalla de potencia
        if (inElecScreen)
        {
          drawElecScreen();
        }

        //Comenzar a contar tiempo de potencia actual
        acumuladorPotenciaRef = millis();
        //Cambiar a pedir agua
        sensorType = 2;
        break;

      case 2: //Obtener valor de agua
        //Acumular a totalizador de litros
        litros += (caudalVal / 1000.000) * ((millis() - acumuladorCaudalRef) / 60000.000);
        //Convertir mensaje de entrada a float
        caudalVal = message.toFloat();
        //Si la ventana actual es la de caudal, refrescar
        if (inQScreen)
          drawQScreen();

        //Comenzar a contar tiempo de caudal actual
        acumuladorCaudalRef = millis();
        //Cambiar a pedir gas
        sensorType = 0;
        break;

      default:
        break;
      }

      //Finalizar proceso
      sendOrReceive = true;
      //Tomar referencia de tiempo
      timeRef = millis();
    }

    //Revisar si sucede un timeout
    if (millis() - timeRef > 1500)
    {
      //Cambiar indice al siguiente
      switch (sensorType)
      {
      case 0:
        sensorType = 1;
        break;

      case 1:
        sensorType = 2;
        Serial.println("Potencia no respondió");
        break;

      case 2:
        sensorType = 0;
        break;

      default:
        break;
      }

      //Indicar que ahora se va aenviar
      sendOrReceive = true;
      //Tomar referencia de tiempo
      timeRef = millis();
    }
  }
}

/**
 * Nombre: connectToWifi
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Conectarse a una red WiFi
 * **/
void connectToWifi()
{
  //Nombre y contraseña de la red
  const char *ssid = "Eventos2";
  const char *password = "Invitad0s22018";

  //Iniciar conexión
  WiFi.begin(ssid, password);

  //Verificar que ya se conectó
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  //Configurar reloj
  configTime(0, 0, "pool.ntp.org");
}

/**
 * Nombre: getEpochTime
 * Parametros:
 * Retorno: time_t::timepoActual en EPOCH
 * 
 * Descripcion: Obtener en formato epoch, la fecha
 * actual
 * **/
time_t getEpochTime()
{
  //Crear instancia
  time_t now;
  //Asignarle el tiempo actual
  time(&now);

  //Retornar el resultado
  return now;
}

/**
 * Nombre: getCurrentTime
 * Parametros:
 * Retorno: String::
 * 
 * Descripcion: Obtener en formato de String, la hora
 * actual
 * **/
String getCurrentTime()
{
  //Crear instancia de informacion de tiempo
  struct tm timeinfo;

  //Intentar obtener la hora actual
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

  //Convertir el objeto de tiempo a un arreglo de caracteres
  //con formato específico
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);

  //Convertir arreglo a una cadena
  String asString(timeStringBuff);
  //Retornar cadena
  return timeStringBuff;
}

/**
 * Nombre: handleDbData
 * Parametros:
 * Retorno:
 * 
 * Descripcion: Manejar rutina para enviar los datos 
 * de cada sensor a la nube
 * **/
void handleDbData()
{
  //Indice para manejar el envio de datos de sensores
  static int op = 0;

  switch (op)
  {
  //Actualizar datos de gas
  case 0:
    db.modificarValoresGas(nivelGas);
    db.agregarRegistroGas(nivelGas, getEpochTime());
    op++;
    break;

  //Actualizar datos de potencia
  case 1:
    if (potAcc == 0)
      return;
    db.modificarValoresElec(potAcc);
    db.agregarRegistroElec(potAcc, getEpochTime());
    op++;
    break;

  //Actualizar daos de agua
  case 2:
    db.modificarValoresAgua(litros);
    db.agregarRegistroAgua(litros, getEpochTime());
    op = 0;
    break;
  }
}