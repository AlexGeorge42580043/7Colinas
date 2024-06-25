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

    ESP32Time rtc;

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


// DECLARACION DE VARIABLES
    float temperatura;
    unsigned long previousMillis = 0;
    unsigned long documentPreviousMillis = 0;
    const unsigned int documentCreationInterval = 3000;
    String Fecha;
    String Hora;


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
}

void loop() {
  
    app.loop();
    Docs.loop();
    timeClient.update();

    Serial.println(timeClient.getFormattedTime()); // Muestra hora en terminal
    Serial.println(rtc.getDate()); //Muestra fecha en terminal
    delay(1000);

    //Verifica si la aplicacion esta lista para usar
    //Verifica si ya pasaron 3 segundos de la ultima vez que se cargaron datos en la base
       if (app.ready() && documentPreviousMillis < millis() - documentCreationInterval){
        documentPreviousMillis = millis(); //Actualiza la variable al valor actual de Millis

        String documentPath = "Produccion/" + String(timeClient.getEpochTime()); //Crea una coleccion llamada Produccion con un documento random en firebase

        temperatura = ++temperatura; //PROVISORIO

        Fecha = (rtc.getDate()); //Se almacena la fecha actual en la variable Fecha de Tipo string
        Hora = (timeClient.getFormattedTime()); //Se almacena la hora actual en la variable Hora de Tipo string

        Values::DoubleValue temperaturaValue(temperatura);
        Values::DoubleValue tanqueValue(1);
        Values::StringValue fechaValue(Fecha);
        Values::StringValue horaValue(Hora);

        Document<Values::Value> 
        doc("temperatura", Values::Value(temperaturaValue)); //Crea una coleccion llamada Temperatura con el valor que toma de la medicion
        doc.add("Tanque", Values::Value(tanqueValue)); //Crea una coleccion llamada Tanque con el valor del numero de tanque al que se refiere la medicion
        doc.add("Fecha", Values::Value (fechaValue)); //Crea una coleccion llamada Fecha con la fecha actual
        doc.add("Hora", Values::Value (horaValue)); //Crea una coleccion llamada Hora con la hora actual

        Docs.createDocument(aClient, Firestore::Parent(FIREBASE_PROYECTO_ID), documentPath, DocumentMask(), doc, asyncCB, "Documento creado  \n");
  }
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
    //, payload : % s\n ", aResult.uid().c_str(), aResult.c_str());
  }
}
