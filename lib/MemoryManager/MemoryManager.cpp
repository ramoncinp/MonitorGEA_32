#include "MemoryManager.h"

//Constructor
MemoryManager::MemoryManager()
{
}

//Destructor.
MemoryManager::~MemoryManager()
{
}

//Funcion para inicializar
void MemoryManager::begin()
{
    //Inicializar EEPROM
    EEPROM.begin(EEPROM_SIZE);    
}

//Funcion para agregar un dato float
void MemoryManager::saveData(float data, int address)
{
    uint32_t mData = data * 1000;
    EEPROM.writeLong(address, mData);
    EEPROM.commit();
}

//Funcion para agregar un dato de un byte
void MemoryManager::saveData(int data, int address)
{
    uint8_t mData = data;
    EEPROM.write(address, data);
    EEPROM.commit();
}

//Funcion para obtener un int desde la memoria
int MemoryManager::getIntData(int address)
{
    byte mByte = EEPROM.readByte(address);
    return mByte;
}

//Funcion para obtener un float desde la memoria
float MemoryManager::getData(int address)
{
    return EEPROM.readLong(address) / 1000.000;
}
