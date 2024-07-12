#include <ESP32Time.h>//
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Arduino.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <secrets.h>
#include <ArduinoJson.h>
#include <CTBot.h>
#include "Utilities.h"

    ESP32Time rtc;
    CTBot miBot;


//DE ACA PARA ABAJO ESTA LO QUE TIENE QUE VER CON LA HORA ACTUAL
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", -10800, 60000); //configura la zona horaria


// DE ACA PARA ABAJO ESTA LO QUE TIENE QUE VER CON FIRESTORE
    UserAuth user_auth(API_KEY, USUARIO_EMAIL, USUARIO_CONTRA, 3000);

    void asyncCB(AsyncResult &aResult);

    void printResult(AsyncResult &aResult);

    DefaultNetwork network; // Inicializar con un parámetro booleano para habilitar/deshabilitar la reconexión de red

    FirebaseApp app;

    WiFiClientSecure ssl_client; // es una clase que proporciona una conexión segura (usando SSL/TLS) a través de WiFi.

    using AsyncClient = AsyncClientClass; // Cambia de nombre

    AsyncClient aClient(ssl_client, getNetwork(network));

    Firestore::Documents Docs; // es parte de la plataforma Firebase y se utiliza para interactuar con la base de datos Firestore.


    void crearDocumento(float, String, String, int); //declaracion de la funcion que se encarga de crear un documento en firebase
    float convertir_rango(int); //declaracion de la funcion que se encarga de llevar la temp al rango deseado

// DECLARACION DE VARIABLES Y VECTORES
    const int adcPins[] = {34, 35, 32, 33, 39, 36};  // Pines GPIO asociados al ADC1
    int valores[6]; //Vector que almacena el valor (0 - 1024) leido por el adc
    int tanqueNumero = 1; //numero del tanque que se esta leyendo la temperatura
    int publicacionNumero = 1; //numero del tanque al que pertenece la publicacion que se debe realizar
    unsigned long previousMillisLeer = 0; //sirve para calcular el tiempo que trascurre y compararlo para leer
    unsigned long previousMillisPublicar = 0;  //sirve para calcular el tiempo que trascurre y compararlo para Publicar
    const unsigned int intervalLeer = 3000; //Intervalo para leer (3seg)
    const unsigned int intervalPublicar = 120000; //Intervalo para publicar (10min)
    float tempTanque1; //aca se almacena la temperatura (-10 - 25) de el tanque
    float tempTanque2;//aca se almacena la temperatura (-10 - 25) de el tanque
    float tempTanque3;//aca se almacena la temperatura (-10 - 25) de el tanque
    float tempTanque4;//aca se almacena la temperatura (-10 - 25) de el tanque
    float tempTanque5;//aca se almacena la temperatura (-10 - 25) de el tanque
    float tempTanque6;//aca se almacena la temperatura (-10 - 25) de el tanque
    //const char mensaje[60];


void setup() {

// CONFIGURACION VELOCIDAD PUETO SERIE
  Serial.begin(115200);

// ACA COMIENZA LA CONFIGURACION DE RTC PARA OTENCION DE FECHA ACTUAL
  configTime(-10800, 0, "europe.pool.ntp.org");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
    rtc.setTimeStruct(timeinfo); 
  }

// DE ACA PARA ABAJO ESTA LO QUE TIENE QUE VER CON LA CONEXION WIFI
    WiFiManager wifiManager;//Crea una instancia de WiFiManager
    //wifiManager.resetSettings();// Esto puede ser útil durante las pruebas o cuando deseas borrar las configuraciones guardadas

    //Conexion automatica a wifi
    bool res;
    res = wifiManager.autoConnect(AP_NOMBRE, AP_CONTRA); //Levanta un AP con nombre ESP-AP y contraseña 7colinas

    if(!res) { //verifica si la variable res es falsa (es decir, si no se pudo establecer la conexión WiFi correctamente).
        Serial.println("Error al conectar"); 
        ESP.restart(); //Si hubo un error de conexion, el micro se reinicia
    } 
    else { //Si se pudo conectar correctamente:   
        Serial.println("conectado correctamente");
    }

    // Una vez conectado a wifi, en el terminal mostrara el ip de la red
    Serial.print("Conectado con IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();


//ACA COMIENZA LO QUE TIENE QUE VER CON FIREBASE
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    Serial.println("Initializing app...");

    #if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040) // Verifica si alguna de esas macros está definida
        ssl_client.setInsecure(); // llama a una función llamada setInsecure() en un objeto llamado ssl_client
    #endif
    
    initializeApp(aClient, app, getAuth(user_auth), asyncCB, "authTask"); //Se usa para inicializar una aplicación de Firebase con la configuración proporcionada
    
    app.getApp<Firestore::Documents>(Docs);

    miBot.setTelegramToken(TOKEN);
    if (miBot.testConnection()){
      Serial.println("\n Bot conectado");
    }
    else{
      Serial.println("\n Bot no conectado");
    }

//CONFIGURA PARA AJUSTAR LA SENCIBILIDAD DEL ADC
    for (int tanqueNumero = 0; tanqueNumero < 6; ++tanqueNumero) {
        // Configura la atenuación para 11 dB (rango de 0-3.3V)
        analogSetAttenuation(ADC_11db);
        analogReadResolution(10);  // Resolución de 10 bits
    }

}

void loop() {
  
    app.loop();
    Docs.loop();
    timeClient.update();
    
  //Estas lineas son para mastrar la hora y la fecha en el terminal
  // Serial.println(timeClient.getFormattedTime()); // Muestra hora en terminal
  //Serial.println(rtc.getDate()); //Muestra fecha en terminal
  // delay(1000);

   
  //intervalLeer=3seg
  if (millis() - previousMillisLeer >= intervalLeer){ 
    previousMillisLeer = millis();
    //intervalPublicar=10min
    if (millis() - previousMillisPublicar >= intervalPublicar){
      // Publica cada 3 seg si pasaron 10 min
      
      switch (publicacionNumero){
        case 1:
          crearDocumento(tempTanque1, rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
          miBot.sendMessage(ID_CHAT, "Hay temperaturas fuera de rango, revisar lo antes posible"); //Manda este mensaje a telegram, al id correspondiente
          publicacionNumero=2;
        break;
        case 2:
          crearDocumento(tempTanque2, rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
          publicacionNumero=3;
          break;
        case 3:
          crearDocumento(tempTanque3, rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
          publicacionNumero=4;
          break;
        case 4:
          crearDocumento(tempTanque4, rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
          publicacionNumero=5;
          break;
        case 5:
          crearDocumento(tempTanque5, rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
          publicacionNumero=6;
          break;
        case 6:
          crearDocumento(tempTanque6, rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
          publicacionNumero=1;
          previousMillisPublicar = millis();
          break;
      }
    }
    //Lee los valores de temperatura cada 3seg
      switch (tanqueNumero){
      case 1:
        valores[0] = analogRead(adcPins[tanqueNumero]);
        tempTanque1 = convertir_rango(valores[0]);
        tanqueNumero = 2;
      break;
      case 2:
        valores[1] = analogRead(adcPins[tanqueNumero]);
        tempTanque2 = convertir_rango(valores[1]);
        tanqueNumero = 3;
        break;
      case 3:
        valores[2] = analogRead(adcPins[tanqueNumero]);
        tempTanque3 = convertir_rango(valores[2]);
        tanqueNumero = 4;
        break;
      case 4:
        valores[3] = analogRead(adcPins[tanqueNumero]);
        tempTanque4 = convertir_rango(valores[3]);
        tanqueNumero = 5;
        break;
      case 5:
        valores[4] = analogRead(adcPins[tanqueNumero]);
        tempTanque5 = convertir_rango(valores[4]);
        tanqueNumero = 6;
        break;
      case 6:
        valores[5] = analogRead(adcPins[tanqueNumero]);
        tempTanque6 = convertir_rango(valores[5]);
        tanqueNumero = 1;
        break;
    }
  }
  //Estas lineas devuelven un mensaje con el id correspondiente del chat a cualquier persona que escriba a este bot
  //TBMessage msg;
  //if(CTBotMessageText==miBot.getNewMessage(msg)){
    //miBot.sendMessage(msg.sender.id, "ID: " + (String)msg.sender.id);
  //}
}

void asyncCB(AsyncResult &aResult){
  printResult(aResult);
}

void printResult(AsyncResult &aResult){
  if (aResult.isEvent()){
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
  }

  if (aResult.isDebug()){
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()){
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  if (aResult.available()){
    Firebase.printf("task: %s", aResult.uid().c_str());
  }
}

float convertir_rango(int valor_ADC) {
    float m = (25.0 - (-10.0)) / 1023.0;  // Pendiente
    float b = 25.0 - m * 1023.0;  // Ordenada al origen
    return m * valor_ADC + b;
}

void crearDocumento(float temperatura, String fecha, String hora, int tanque){
    //Verifica si la aplicacion esta lista para usar
    if (app.ready()){
        String documentPath = "Produccion/" + String(timeClient.getEpochTime()); //Crea una coleccion llamada Produccion con un documento random en firebase

        Values::DoubleValue temperaturaValue(temperatura);
        Values::IntegerValue tanqueValue(tanque);
        Values::StringValue fechaValue(fecha);
        Values::StringValue horaValue(hora);

        Document<Values::Value> 
        doc("Temperatura", Values::Value(temperaturaValue)); //Crea una coleccion llamada Temperatura con el valor que toma de la medicion
        doc.add("Tanque", Values::Value(tanqueValue)); //Crea una coleccion llamada Tanque con el valor del numero de tanque al que se refiere la medicion
        doc.add("Fecha", Values::Value (fechaValue)); //Crea una coleccion llamada Fecha con la fecha actual
        doc.add("Hora", Values::Value (horaValue)); //Crea una coleccion llamada Hora con la hora actual

        Docs.createDocument(aClient, Firestore::Parent(FIREBASE_PROYECTO_ID), documentPath, DocumentMask(), doc, asyncCB, "Documento creado  \n");
    }
}   