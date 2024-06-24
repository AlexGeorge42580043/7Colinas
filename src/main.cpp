#include <WiFiManager.h>
#include <Arduino.h>
#include <secrets.h>

void setup() {

    Serial.begin(115200);
    
    //Crea una instancia de WiFiManager
    WiFiManager wifiManager;

    // Esto puede ser útil durante las pruebas o cuando deseas borrar las configuraciones guardadas
     wifiManager.resetSettings();

 
    //Conexion automatica a wifi
    bool res;
    res = wifiManager.autoConnect(AP_NOMBRE, AP_CONTRA); //Levanta un AP con nombre ESP-AP y contraseña 7colinas

    if(!res) { //verifica si la variable res es falsa (es decir, si no se pudo establecer la conexión WiFi correctamente).
        Serial.println("Error al conectar"); 
        ESP.restart(); //Si hubo un error de conexion, el micro se reinicia
    } 
    else {
        //Si se pudo conectar correctamente:   
        Serial.println("conectado correctamente");
    }

    // Una vez conectado a wifi, en el terminal mostrara el ip de la red
    Serial.print("Conectado con IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void loop() {
  

}
