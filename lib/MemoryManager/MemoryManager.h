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
#define EEPROM_SIZE 521

class MemoryManager
{
public:
  MemoryManager();
  ~MemoryManager();
  void begin();

private:
};

#endif