#include <WiFi.h>
#include <WiFiManager.h>
#include <Arduino.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <secrets.h>

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

void setup() {

    Serial.begin(115200);


// DE ACA PARA ABAJO ESTA LO QUE TIENE QUE VER CON LA CONEXION WIFI
    //Crea una instancia de WiFiManager
    WiFiManager wifiManager;
    // Esto puede ser útil durante las pruebas o cuando deseas borrar las configuraciones guardadas
    //wifiManager.resetSettings();

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

    //Verifica si la aplicacion esta lista para usar
    //Verifica si ya pasaron 3 segundos de la ultima vez que se cargaron datos en la base
       if (app.ready() && documentPreviousMillis < millis() - documentCreationInterval){
        documentPreviousMillis = millis();

        String documentPath = "Produccion/"; //Crea una coleccion llamada Produccion con un documento random en firebase

        temperatura = ++temperatura;

        Values::DoubleValue temperaturaValue(temperatura);
        Values::DoubleValue tanqueValue(1);

        Document<Values::Value> 
        doc("temperatura", Values::Value(temperaturaValue)); //Crea una coleccion llamada Temperatura con el valor que toma de la medicion
        doc.add("Tanque", Values::Value(tanqueValue)); //Crea una coleccion llamada Tanque con el valor del numero de tanque al que se refiere la medicion

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
