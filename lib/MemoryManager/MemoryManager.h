/**
 * Programa: MemoryManager.h
 * Descripción: Encabezados para el programa de MemoryManager.cpp
 * Fecha de creación: 19/MAY/2019
 * Por: Ramón Parra
 * **/

#ifndef MemoryManager_h
#define MemoryManager_h

#include <ArduinoJson.h>
#include <Arduino.h>
#include <EEPROM.h>

//Utilizar todo el rango disponible
#define EEPROM_SIZE 512

//Direcciones de las variables
#define LITROS_ADDR 508
#define LITROS_REF_ADDR 504
#define POTENCIA_ADDR 500
#define POTENCIA_REF_ADDR 496
#define NIVEL_GAS_ADDR 495

class MemoryManager
{
public:
  MemoryManager();
  ~MemoryManager();
  void begin();
  void saveData(float data, int address);
  void saveData(int data, int address);
  float getData(int address);
  int getIntData(int address);

private:
};

#endif