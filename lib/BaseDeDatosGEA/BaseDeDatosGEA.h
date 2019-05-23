/**
 * Programa: BaseDeDatosGEA.h
 * Descripción: Encabezados para el programa de BaseDeDatosGEA.cpp
 * Fecha de creación: 16/ENE/2019
 * Por: Ramón Parra
 * **/

#ifndef BaseDeDatosGEA_h
#define BaseDeDatosGEA_h

#include <ArduinoJson.h>
#include <HTTPClient.h>

//Dominio
const String FIREBASE_URL = "https://monitorgea.firebaseio.com";

class BaseDeDatosGEA
{
public:
  BaseDeDatosGEA();
  ~BaseDeDatosGEA();
  String begin();
  bool actualizarSensores(int gas, double elec, double agua, double totElec, double totAgua);
  bool agregarRegistroGas(int valor, time_t timeStamp);
  bool agregarRegistroElec(float valor, time_t timeStamp);
  bool agregarRegistroAgua(float valor, time_t timeStamp);
  bool modificarValoresGas(int valor);
  bool modificarValoresElec(float valor);
  bool modificarValoresAgua(float valor);

private:
};

#endif