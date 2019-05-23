//
// Copyright 2015 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/**
 * Programa: BaseDeDatosGEA.cpp
 * Descripción: Objeto para manejar la base de datos Firebase.
 * Fecha de creación: 16/ENE/2019
 * Por: Ramón Parra
 * **/

#include "BaseDeDatosGEA.h"

//Certificador https
const char *certificate =
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

//Constructor
BaseDeDatosGEA::BaseDeDatosGEA()
{
}

//Destructor.
BaseDeDatosGEA::~BaseDeDatosGEA()
{
}

/**
 * Método: begin
 * Obtiene los datos actuales de las variables
 * --------------------------
 * */

String BaseDeDatosGEA::begin()
{
    //Pedir datos iniciales
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(FIREBASE_URL + "/periodos.json", certificate);
        int httpCode = http.GET();

        if (httpCode == 200)
        {
            String payload = http.getString();
            http.end();
            return payload;
        }
        else
        {
            http.end();
            return "";
        }
    }
}

bool BaseDeDatosGEA::actualizarSensores(int gas, double elec, double agua, double totElec, double totAgua)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        //Crear body
        String body = "";

        //Crear un buffer dinámico
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.createObject();

        //Agregar datos
        JsonObject &gasJson = root.createNestedObject("gas");
        gasJson["valor"] = gas;

        JsonObject &elecJson = root.createNestedObject("electricidad");
        elecJson["valor"] = elec;
        elecJson["totalizador"] = totElec;

        JsonObject &aguaJson = root.createNestedObject("agua");
        aguaJson["valor"] = agua;
        aguaJson["totalizador"] = totAgua;

        //Almacenar en String
        root.printTo(body);

        HTTPClient http;
        http.begin(FIREBASE_URL + "/sensores.json", certificate);
        int httpCode = http.PUT(body);

        if (httpCode == 200)
        {
            http.end();
            return true;
        }
        else
        {
            http.end();
            return false;
        }
    }
    return false;
}

bool BaseDeDatosGEA::agregarRegistroGas(int valor, time_t timeStamp)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        //Crear body
        String body = "";

        //Crear un buffer dinámico
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.createObject();

        //Agregar datos
        root["valor"] = valor;
        root["fecha"] = timeStamp;
        root.printTo(body);

        HTTPClient http;

        http.begin(FIREBASE_URL + "/gas.json", certificate);
        int httpCode = http.POST(body);

        if (httpCode == 200)
        {
            http.end();
            return true;
        }
        else
        {
            http.end();
            return false;
        }
    }
    return false;
}