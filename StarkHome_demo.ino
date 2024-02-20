// Librerias
  #include <SPI.h>
  #include <WiFiNINA.h>
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  #include <Adafruit_Sensor.h>
  #include <DHT.h>
  #include <DHT_U.h>
  #include <Servo.h>
  #include <pitches.h>
  #include <TimeLib.h>

//Delay no retardante
  void GetDelay(unsigned long tiempo) {
    unsigned long tiempoInicial = millis();
    while (millis() - tiempoInicial < tiempo) {
    }
  }

//Variables Tiempo Riego
  const int DEFAULT_HOUR = 00;
  const int DEFAULT_MINUTE = 00;
  const int DEFAULT_SECOND = 00;

  int hourInput = 00;
  int minuteInput = 00;
  int secondInput = 05;

  int hourOutput = 00;
  int minuteOutput = 00;
  int secondOutput = 10;



//Propiedades de la pantalla LCD
  LiquidCrystal_I2C lcd(0x27, 20, 4);

//Configuración del servidor TCP/IP
  IPAddress ip(192, 168, 0, 33);  // IP de nuestro servidor
  char ssid[] = "RETO_VIVIENDA4";            // SSID de la wifi
  char pass[] = "123456789";  // Contraseña de la wifi
  int port = 7778;                // Puerto TCP de nuestro servidor

  WiFiServer server(port); //Agregacion al servidor del puerto


//Variables Persiana
  Servo SunBlind;          // Crear un objeto de tipo Servo
  int pos = 90;  // Posición inicial
  bool subir = false;  // Indica si se está subiendo la persiana
  bool bajar = false;  // Indica si se está bajando la persiana

//Selector aleatorio del simulador de presencia
  unsigned int presenceLight;


//Variables Alarma
  bool ArmedAlarm; 


// Definicion de pines:
  #define pinLDR A1           // Sensor de luz LDR 
  #define pinGasSensor A0     // Sensor de gas
  #define pinMagnet 8         // Interruptor magnetico de la puerta
  #define pinPrincipLight 5   // Luz del cuarto principal
  #define pinBuzzer 7         // Zumbador
  #define pinGuestLight 3     // Luz del cuarto de invitados
  #define pinKitchenLight 6   // Luz de la cocina
  #define echoPin  2          // Pin de echo del sensor de ultrasonidos
  #define trigPin 4           // Pin de trigger del sensor de ultrasonidos
  #define pinLivinLight 9     // Luz del salon 
  #define pinSunBlind 11      // Persiana
  #define pinTemp 12          // Sensor de humedad y temperatura
  #define pinPresence 13      // Sensor PIR
  #define pinRiego 10         // Irrigador

//Sensor de humedad y temperatura
  #define DHTTYPE DHT11   // Definicion del tipo de sensor de humedad que utilizaremos
  DHT_Unified dht(pinTemp, DHTTYPE); // Creacion de un objeto dht para una utilizacion mas comoda

//Ultrasonidos
  const int umbralDistancia = 10; // Centimetros para el ultrasonidos

//Variables de las luces
  byte brilloDormPrin= 0;
  byte brilloDormInv= 0;
  byte brilloCocina= 0;
  byte brilloSalon= 0;
  bool estadoLedDormPrin= false;
  bool estadoLedDormInv= false;
  bool estadoLedCocina= false;
  bool estadoLedSalon= false;

// Luces
bool ldrActivate= false; // Modo para las luces


// Millis
unsigned long tiempoInicio = 0; // Definimos un tiempo de inicio 0
int tiempoEspera = 3000;        // 3 segundos en milisegundos

bool presenceSim= false;
bool panicAlarm= false;


//Variables Medidor Corriente
  // Sensibilidad del sensor en V/A
    float SENSIBILITY = 0.185;   // Modelo 5A
    //float SENSIBILITY = 0.100; // Modelo 20A
    //float SENSIBILITY = 0.066; // Modelo 30A

  float current;
  float currentRMS;
  float power;

int SAMPLESNUMBER = 2000;

void setup() {
  
  Serial.begin(9600); // Inicializamos el puerto serie
  
  // Setup de los pines
    pinMode(pinPrincipLight, OUTPUT);          
    pinMode(pinGuestLight, OUTPUT);              
    pinMode(pinKitchenLight, OUTPUT);                 
    pinMode(pinPresence, INPUT);                    
    pinMode(pinBuzzer, OUTPUT);               
    SunBlind.attach(pinSunBlind);                   
    pinMode(pinLivinLight, OUTPUT);                 
    pinMode(trigPin, OUTPUT);                   
    pinMode(echoPin, INPUT);                    
    pinMode(pinMagnet, INPUT_PULLUP);  
    pinMode(pinRiego, OUTPUT);

 setTime(DEFAULT_HOUR, DEFAULT_MINUTE, DEFAULT_SECOND, 1, 1, 2022); // Configura la hora por defecto

  noTone(pinBuzzer);
  digitalWrite(pinBuzzer, HIGH);

  while (!Serial) {   // Espera a que el puerto serie se conecte
  }

  // Intenta conectarse a la red Wi-Fi
  while (WiFi.status() != WL_CONNECTED) { //Bucle while para iniciar el servidor
    Serial.print("Conectando a SSID: ");  // Imprime la SSID
    Serial.println(ssid);                 // Imprime la SSID
    WiFi.begin(ssid, pass);               // Inicializacion del server en una
    GetDelay(1000);                          // Espera 1 segundos para la conexión
    WiFi.config(ip);
  }


  // Inicia el servidor en el puerto especificado
  server.begin();                                    // Inicio del servidor
  Serial.print("Servidor esperando en el puerto: "); // Comprobacion de que se ha iniciado en el puerto correspondiente 
  Serial.println(port);                              // Comprobacion de que se ha iniciado en el puerto correspondiente 

  // Sensor de humedad
  dht.begin();                       // Inicio del sensor de humedad
  sensor_t sensor;                   // Creamos un objeto sensor
  dht.humidity().getSensor(&sensor); // Obtenemos el valor de la humedad


  // Pantalla LCD
  lcd.init();  // Inicializa la pantalla lcd 
  lcd.backlight();
  lcd.clear(); // Limpia la pantalla lcd


  IPAddress ip = WiFi.localIP();
  Serial.print("IP address : ");
  Serial.println(ip);

}


void printMeasure(String prefix, float value, String postfix)
{
  Serial.print(prefix);
  Serial.print(value, 3);
  Serial.println(postfix);
}

void loop() {
  

WiFiClient client = server.available();           // Verifica si hay clientes conectados

  int LDR_Value = analogRead(pinLDR);  // Lee el valor del LDR


  if (ldrActivate == 1){                       // Diferencia entre modo manual y modo LDR
    ldrLoop();
  }



  if (ArmedAlarm) {                // Alarma Armada
    pirLoop();
    ultrasonidoLoop();
    pantallaLoopAlarma();
  }
  if(presenceSim){  //Sensor de presencia
      simuladorPresencia();
  
  } 
   TempHumSensor();                 
   detectorGasLoop();                        

  if (client) {                              // Cada vez que el cliente envia un caracter ejecuta esta seccion del codigo

    Serial.println("¡Cliente conectado!");   // Verifica que el cliente se ha conectado
    char data = client.read();               // Lee el primer los datos del cliente

  switch(data) {                            // Uso de switch para evitar el anidamiento de ifs
    //Modo alarma
      case 'A':
        if (ArmedAlarm) {
          Serial.println("Alarma Desactivada");
        } else {
          Serial.println("Alarma Activada");
        }
        ArmedAlarm = !ArmedAlarm;
        break;

    //Modo de luz autoregulable
      case 'l':                               // Activa o desactiva la intensidad de la luz en relación con el LDR
        ldrActivate = !ldrActivate;
        break;

      //Controles de la persiana
        //Subir la persiana
          case 'P':
            subir = !subir;  // Invertir el estado de subir
            bajar=false;     // Apaga la función de bajar si estuviera encendida
          break;
        //Bajar la persiana
          case 'p':
            bajar = !bajar;  // Invertir el estado de bajar
            subir=false;     // Apaga la función de bajar si estuviera encendida
          break;

      //Botón del pánico
        case 'Z':
          panicAlarm= !panicAlarm;
          break;

      //Simulador de presencia
        case 's':
          presenceSim= !presenceSim;
          break;
      //Recibir Datos    
        case 'r': 
          datos();
          break;

    //Luces
      //Dormitorio Principal
        case 'Q':
          if ((brilloDormPrin < 255)) {
            brilloDormPrin= brilloDormPrin +51;
            }
          analogWrite(pinPrincipLight, brilloDormPrin);
          if (estadoLedDormPrin== false){
            estadoLedDormPrin=true;
          }
          break;
        
        case 'R':
          if(brilloDormPrin==0){
            brilloDormPrin=255;
            analogWrite(pinPrincipLight, brilloDormPrin);
            estadoLedDormPrin=true;     
          }
          else if(estadoLedDormPrin){
            estadoLedDormPrin=!estadoLedDormPrin;
            analogWrite(pinPrincipLight, 0);
          }
          else{
            estadoLedDormPrin= !estadoLedDormPrin;
            analogWrite(pinPrincipLight, brilloDormPrin);
          }
          break;

        case 'q':
          if ((brilloDormPrin>0)) {
            brilloDormPrin= brilloDormPrin -51;
            }
          analogWrite(pinPrincipLight, brilloDormPrin);
          if (estadoLedDormPrin== false){
            estadoLedDormPrin=true;
          }
          if(brilloDormPrin == 0){
            estadoLedDormPrin=false;
          }
          break;

      //Cocina
        case 'K':
          if ((brilloCocina < 255)) {
            brilloCocina= brilloCocina +51;
          }
          analogWrite(pinKitchenLight, brilloCocina);
          if (estadoLedCocina== false){
            estadoLedCocina=true;
          }
          break;

        case 'C':
          if(brilloCocina==0){
            brilloCocina=255;
            analogWrite(pinKitchenLight, brilloCocina);
            estadoLedCocina=true;     
          }
          else if(estadoLedCocina){
            estadoLedCocina=!estadoLedCocina;
            analogWrite(pinKitchenLight, 0);
          }
          else{
            estadoLedCocina= !estadoLedCocina;
            analogWrite(pinKitchenLight, brilloCocina);
          }
          break;

        case 'k':
          if ((brilloCocina>0)) {
            brilloCocina= brilloCocina -51;
            }
          analogWrite(pinKitchenLight, brilloCocina);
          if (estadoLedCocina== false){
            estadoLedCocina=true;
          }
          if(brilloCocina == 0){
            estadoLedCocina=false;
          }
          break;

      //Invitados
        case 'U':
          if ((brilloDormInv < 255)) {
            brilloDormInv= brilloDormInv +51;
          }
          analogWrite(pinGuestLight, brilloDormInv);
          if (estadoLedDormInv== false){
            estadoLedDormInv=true;
          }
          break;

        case 'I':
          if(brilloDormInv==0){
            brilloDormInv=255;
            analogWrite(pinGuestLight, brilloDormInv);
            estadoLedDormInv=true;     
          }
          else if(estadoLedDormInv){
            estadoLedDormInv=!estadoLedDormInv;
            analogWrite(pinGuestLight, 0);
          }
          else{
            estadoLedDormInv= !estadoLedDormInv;
            analogWrite(pinGuestLight, brilloDormInv);
          }
          break;
        
        case 'u':
          if ((brilloDormInv>0)) {
            brilloDormInv= brilloDormInv -51;
          }
          analogWrite(pinGuestLight, brilloDormInv);
          if (estadoLedDormInv== false){
            estadoLedDormInv=true;
          }
          if(brilloDormInv == 0){
            estadoLedDormInv=false;
          }
          break;

      //Salon
        case 'V':
          if ((brilloSalon < 255)) {
            brilloSalon= brilloSalon +51;
          }
          analogWrite(pinLivinLight, brilloSalon);
          if (estadoLedSalon== false){
            estadoLedSalon=true;
          }
          break;

        case 'S':
          if(brilloSalon==0){
            brilloSalon=255;
            analogWrite(pinLivinLight, brilloSalon);
            estadoLedSalon=true;     
          }
          else if(estadoLedSalon){
            estadoLedSalon=!estadoLedSalon;
            analogWrite(pinLivinLight, 0);
          }
          else{
            estadoLedSalon= !estadoLedSalon;
            analogWrite(pinLivinLight, brilloSalon);
          }
          break;
        
        case 'v':
          if ((brilloSalon>0)) {
            brilloSalon= brilloSalon -51;
          }
          analogWrite(pinLivinLight, brilloSalon);
          if (estadoLedSalon== false){
            estadoLedSalon=true;
          }
          if(brilloSalon == 0){
            estadoLedSalon=false;
          }
          break;
      

      //Interruptor General
        case 'G':
          // Verifica si todos los LEDs están apagados
          if ((estadoLedSalon==false) &&
              (estadoLedCocina==false) &&
              (estadoLedDormInv==false) &&
              (estadoLedDormPrin==false)) {
            // Si todos los LEDs están apagados, enciéndelos todos
            digitalWrite(pinPrincipLight, HIGH);
            digitalWrite(pinKitchenLight, HIGH);
            digitalWrite(pinGuestLight, HIGH);
            digitalWrite(pinLivinLight, HIGH);
            Serial.println("Encendiendo todos los LEDs");
            estadoLedDormPrin= true;
            estadoLedDormInv= true;
            estadoLedCocina= true;
            estadoLedSalon= true;
          } else {
            // Si al menos un LED está encendido, apaga todos los LEDs
            digitalWrite(pinPrincipLight, LOW);
            digitalWrite(pinKitchenLight, LOW);
            digitalWrite(pinGuestLight, LOW);
            digitalWrite(pinLivinLight, LOW);
            Serial.println("Apagando todos los LEDs");
            estadoLedDormPrin= false;
            estadoLedDormInv= false;
            estadoLedCocina= false;
            estadoLedSalon= false;
          }
          break;
      }
  }
    
  
  if (bajar) {
        // Bajar el toldo gradualmente
      BajarToldo();
    }

  if (subir) {
        // Subir el toldo gradualmente
      SubirToldo();
    }

  if(panicAlarm){
    TriggeredAlarm();
  }
  
}



//Methods



void SubirToldo(){
      if(pos==0){
        subir= false;
      }
      else if (pos > 0) {
        pos -= 1;
        SunBlind.write(pos);
      }
      GetDelay(20);
      Serial.println("Subiendo persiana");
}

void BajarToldo(){
  if (pos == 165){
        bajar= false;
      }
      else if (pos < 165) {
        pos += 1;
        SunBlind.write(pos);
      }
      
      GetDelay(20);
      Serial.println("Bajando persiana");
}



long medirDistancia() {
  // Generar pulso ultrasónico
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Medir la duración del eco
  return pulseIn(echoPin, HIGH);
}

int calcularDistancia(long duration) {
  // Calcular la distancia en centímetros
  return (duration / 2) / 29.1;
}

 
  
 
void TriggeredAlarm(){ 
  int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
  int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};
  for (int i = 1000; i < 2000; i++) { 
    tone(pinBuzzer, i);
    GetDelay(1);
  }
  for (int i = 2000; i > 1000; i--) { 
    tone(pinBuzzer, i);
    GetDelay(1);
  }
  noTone(pinBuzzer);
  digitalWrite(pinBuzzer, HIGH);
}


void datos(){
  WiFiClient client = server.available();
  
  
      sensors_event_t event;
      dht.temperature().getEvent(&event);
      int tempAct = event.temperature;
      int tempSent= tempAct + 500;
      Serial.println(tempAct);
      client.print(tempSent);
      GetDelay(10);
      dht.humidity().getEvent(&event);  
      int hum = event.relative_humidity;
      Serial.println(hum);
      client.print(hum);
      currentsensor();
      int curr= current +1000;
      Serial.println(curr);
      client.print(curr);

}

void TempHumSensor(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  long temperaturaActual = event.temperature;

  dht.humidity().getEvent(&event);
  long humedadActual = event.relative_humidity;

 
  // Imprime en el monitor serie
  //Serial.print("Temperatura: ");
  //Serial.print(temperaturaActual);
  //Serial.print(" °C, Humedad: ");
  //Serial.print(humedadActual);
  //Serial.println(" %");

  // Actualiza la información en el LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperaturaActual, 1);  // Imprime temperatura con 1 decimal
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("Humedad: ");
  lcd.print(humedadActual, 1);  // Imprime humedad con 1 decimal
  lcd.print("%");
 
  // Temperatura
  if (temperaturaActual >= 32.0) { //Activa la alarma de incendio si detecta temperaturas por encima de 40ºC
          lcd.clear();
          lcd.backlight();
          lcd.setCursor(0, 0);
          lcd.print("ALERTA");
          lcd.setCursor(0, 1);
          lcd.print("INCENDIO");
          TriggeredAlarm();
          Serial.println("Alerta incendio");
  } 
  
  // Humedad
  dht.humidity().getEvent(&event);
  if (event.relative_humidity >= 60.0) {//Activa la alarma de inundacion si detecta humedad por encima del 70%
            lcd.clear();
            lcd.backlight();
            lcd.setCursor(0, 0);
            lcd.print("ALERTA");
            lcd.setCursor(0, 1);
            lcd.print("INUNDACION");
            TriggeredAlarm();
            Serial.println("Alerta inundacion");
  } 

 
}

  void pirLoop(){
    // PIR LOOP
    int pirValue = digitalRead(pinPresence); // Leer el valor del PIR
    if (pirValue == HIGH) {
     TriggeredAlarm();
    }
  }
void pir() {
  // Si la alarma está desactivada, usar el PIR para controlar la luz
  if (!ArmedAlarm) { // Debe ser !ArmedAlarm para verificar si la alarma NO está armada
    bool movimiento = digitalRead(pinPresence);
    if (movimiento) {
      digitalWrite(pinLivinLight, HIGH); // Encender el LED si se detecta movimiento
      GetDelay(5000);
    } else {
      digitalWrite(pinLivinLight, LOW); // Apagar el LED si no hay movimiento
    }
  }
  

 }
  void ultrasonidoLoop(){
        // Utrasonido VOIDLOOP
    long duration = medirDistancia();
    int distance = calcularDistancia(duration);
   
    if (distance < umbralDistancia) {
      TriggeredAlarm();
    }
    GetDelay(500);  // Esperar medio segundo entre las mediciones
  }

  void ldrLoop () {
    int LDR_Value = analogRead(pinLDR);

    // Establecer umbral para decidir la intensidad de los leds
    int umbralDiaNoche = 0;  // Ajusta según sea necesario

    // Calcular intensidad basada en el valor del sensor de luz
    int bright = map(LDR_Value, 150, 320, 0, 255);

    // Encender el LED para la intensidad calculada 
    if (LDR_Value > umbralDiaNoche) {
      analogWrite(pinKitchenLight, bright);
      analogWrite(pinGuestLight, bright);
      analogWrite(pinLivinLight, bright);
      analogWrite(pinPrincipLight, bright);
    }


    GetDelay(100);  // Ajusta según sea necesario
  }

  void detectorGasLoop(){

    int GasSensor_Value = analogRead(pinGasSensor);

    // Comprobar si el valor del sensor indica la presencia de gas (ajusta el umbral según sea necesario)
    if (GasSensor_Value > 550) {
      TriggeredAlarm();  
    } 
  }








void simuladorPresencia(){
 presenceLight=(random(4));
 Serial.println(presenceLight);
 switch (presenceLight){
    case 0:
      digitalWrite(pinLivinLight, HIGH);
      GetDelay(random(1000,5000));
      digitalWrite(pinLivinLight, LOW);
    break;
    case 1:
      digitalWrite(pinKitchenLight, HIGH);
      GetDelay(random(1000,5000));
      digitalWrite(pinKitchenLight, LOW);
    break;
    case 2:
      digitalWrite(pinPrincipLight, HIGH);
      GetDelay(random(1000,5000));
      digitalWrite(pinPrincipLight, LOW);
    break;
    case 3:
      digitalWrite(pinGuestLight, HIGH);
      GetDelay(random(1000,5000));
      digitalWrite(pinGuestLight, LOW);
    break;
  }
}

void pantallaLoopAlarma() {

  long duration = medirDistancia();
  int distance = calcularDistancia(duration);
  const char *magneticState = "";
  

    if (digitalRead(pinMagnet) == HIGH) {
      magneticState = "Puerta abierta";
      TriggeredAlarm();
    }
    else{      
    }

  static unsigned long tiempoAnterior = 0; // Variable estática para almacenar el tiempo anterior
  unsigned long tiempoActual = millis();
  unsigned long tiempoEspera = 2000;  // 2 segundos

  // Arreglo de mensajes
  const char *etiquetas[] = {"Distancia: ", "Zona "};
  int valores[] = {distance, 1};
  const char *termino[] = {"cm", magneticState};
  static int indiceVariable = 0;

  // Comprobar si ha pasado el tiempo de espera
  // Imprimir etiqueta y valor cada 2 segundos
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(etiquetas[0]); // Etiqueta de distancia
  lcd.print(valores[0]);   // Valor de distancia
  lcd.print(termino[0]);   // Unidad de distancia

  lcd.setCursor(0, 1);
  lcd.print(etiquetas[1]); // Etiqueta de estado de la puerta
  if (valores[1] == HIGH) {
    lcd.print("Abierta");
  } else {
    lcd.print("Cerrada");
  }
  GetDelay(2000);

}

void currentsensor(){
   current = getCorriente(SAMPLESNUMBER);
   currentRMS = 0.707 * current;
   power = 230.0 * currentRMS;

  printMeasure("Intensidad: ", current, "A ,");
  printMeasure("Irms: ", currentRMS, "A ,");
  printMeasure("Potencia: ", power, "W");
  GetDelay(1000);
}

float getCorriente(int samplesNumber){
  float voltage;
  float corrienteSum = 0;
  for (int i = 0; i < samplesNumber; i++)
  {
    voltage = analogRead(A0) * 5.0 / 1023.0;
    corrienteSum += (voltage - 2.5) / SENSIBILITY;
  }
  return(corrienteSum / samplesNumber);
}

void Riego() {
  if (Serial.available() > 0) { // Verifica si hay datos disponibles en el puerto serie
    char command = Serial.read(); // Lee el comando ingresado por el usuario
    
    switch(command) {
      case 'V': // Reinicia la hora, minutos y segundos a 00
        setTime(DEFAULT_HOUR, DEFAULT_MINUTE, DEFAULT_SECOND, 1, 1, 2022);
        break;
      default:
        break;
    }
  }
  
  // Lee la hora actual
  int currentHour = hour();
  int currentMinute = minute();
  int currentSecond = second();
  
  // Imprime la hora actual por el puerto serie
  Serial.print("Hora actual: ");
  Serial.print(currentHour);
  Serial.print(":");
  Serial.print(currentMinute);
  Serial.print(":");
  Serial.println(currentSecond);
  
  // Compara la hora actual con la hora de entrada
  if (currentHour == hourInput && currentMinute == minuteInput && currentSecond == secondInput) {
    digitalWrite(pinRiego, HIGH); // Enciende el LED si coincide con la hora de entrada
  } else if (currentHour == hourOutput && currentMinute == minuteOutput && currentSecond == secondOutput) {
    digitalWrite(pinRiego, LOW); // Apaga el LED si coincide con la hora de salida
  }
}
