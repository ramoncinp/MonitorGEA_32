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
