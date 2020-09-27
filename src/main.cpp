#include <Arduino.h>

/*****
 
 All the resources for this project:
 http://randomnerdtutorials.com/
 
*****/
#include "credentials.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include "DHT.h"
#include <FastLED.h>

// Uncomment one of the lines bellow for whatever DHT sensor type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "WIFI-SSID";
const char* password = "WIFI-PASSWD";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "BROKER-IP";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor - GPIO 16 = D0 on ESP-12E NodeMCU board
//const int DHTPin = 16;

// Declaring pin for led strip in litter area
const int lamp = 4; // GPIO 04 = D2, going to mosfet gate

// Declaring pins for parastytic water pump/motor controlled by L298 H-bridge
const int water = 14; // GPIO 14 = D5 to mosfet gate
//const int in1 = 12;  // GPIO 12 = D6
//const int in2 = 13;  // GPIO 13 = D7

// Setup of stepper motor for cat feeder
const int dir = 2; // GPIO 02 = D4
const int steps = 0; // GPIO 00 = D3
int speed = 2000;  //stepper motor rotation speed

// PIR sensor set up
boolean lockLow = true;
boolean takeLowTime;  
long unsigned int lowIn;
long unsigned int pause = 5000;
char occupied[] = "red";
char notOccupied[] = "green";
//int lastPirValue = LOW;
const int pir = 5; // GPIO 05 = D1


// Initialize DHT sensor.
//DHT dht(DHTPin, DHTTYPE);

// FastLED setup
#define NUM_LEDS 39
#define DATA_PIN  8  // GPIO 15 = D8
#define FASTLED_ESP8266_RAW_PIN_ORDER

CRGB leds[NUM_LEDS];

// Timers auxiliar variables
//long now = millis();
//long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic == "pets/catHouse"){
    if(messageTemp == "feed"){
      Serial.print("Feeding cat");
      digitalWrite(dir, HIGH);
      for(int i = 0; i < 200; i++){
        digitalWrite(steps, HIGH);
        delayMicroseconds(speed);
        digitalWrite(steps, LOW);
        delayMicroseconds(speed);
      }
      digitalWrite(dir, LOW);
    }
    else if(messageTemp == "water"){
        Serial.print("Watering the cat");
        digitalWrite(water, HIGH);
        delay(10000);
        digitalWrite(water, LOW);
      }
  }

  if(topic == "pets/catHouse/accent"){
    long color = strtol(messageTemp.c_str(), NULL, 16);
      for(int i = 0; i < NUM_LEDS; i ++){
        leds[i] = color;
        FastLED.show();
        
    }
  }  
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("pets/catHouse/lights");
      client.subscribe("pets/catHouse");
      client.subscribe("pets/catHouse/accent");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  delay(2000);
  pinMode(lamp, OUTPUT);
  pinMode(water, OUTPUT);
  pinMode(pir, INPUT);
  pinMode(dir, OUTPUT);
  pinMode(steps, OUTPUT);

  pinMode(dir, LOW);
  pinMode(steps, LOW);
  pinMode(water, LOW);
  
  //dht.begin();
  FastLED.addLeds<WS2811, DATA_PIN, BRG>(leds, NUM_LEDS);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

if(digitalRead(pir) == HIGH){
       digitalWrite(lamp, HIGH);   //the led visualizes the sensors output pin state
       if(lockLow){  
         client.publish("pets/catHouse/occupied", occupied);  
         //makes sure we wait for a transition to LOW before any further output is made:
         //lockLow = false;            
         //Serial.println("---");
         //Serial.print("motion detected at ");
         //Serial.print(millis()/1000);
         //Serial.println(" sec"); 
         delay(50);
         }         
         takeLowTime = true;
       }


     if(digitalRead(pir) == LOW){       
       digitalWrite(lamp, LOW);  //the led visualizes the sensors output pin state


       if(takeLowTime){
        lowIn = millis();          //save the time of the transition from high to LOW
        takeLowTime = false;       //make sure this is only done at the start of a LOW phase
        }
       //if the sensor is low for more than the given pause, 
       //we assume that no more motion is going to happen
       if(!lockLow && millis() - lowIn > pause){  
           client.publish("pets/catHouse/occupied", notOccupied);  
           //makes sure this block of code is only executed again after 
           //a new motion sequence has been detected
           lockLow = true;                        
           //Serial.print("motion ended at ");      //output
           //Serial.print((millis() - pause)/1000);
           //Serial.println(" sec");
           delay(50);
           }
       }




  ///////////// uncomment to use temp/hum sensor ///////////////////

  // now = millis();
  // // Publishes new temperature and humidity every 30 seconds
  // if (now - lastMeasure > 30000) {
  //   lastMeasure = now;
  //   // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  //   float h = dht.readHumidity();
  //   // Read temperature as Celsius (the default)
  //   float t = dht.readTemperature();
  //   // Read temperature as Fahrenheit (isFahrenheit = true)
  //   float f = dht.readTemperature(true);

  //   // Check if any reads failed and exit early (to try again).
  //   if (isnan(h) || isnan(t) || isnan(f)) {
  //     Serial.println("Failed to read from DHT sensor!");
  //     return;
  //   }

  //   // Computes temperature values in Celsius
  //   float hic = dht.computeHeatIndex(t, h, false);
  //   static char temperatureTemp[7];
  //   dtostrf(hic, 6, 2, temperatureTemp);
    
  //   // Uncomment to compute temperature values in Fahrenheit 
  //   // float hif = dht.computeHeatIndex(f, h);
  //   // static char temperatureTemp[7];
  //   // dtostrf(hic, 6, 2, temperatureTemp);
    
  //   static char humidityTemp[7];
  //   dtostrf(h, 6, 2, humidityTemp);

  //   // Publishes Temperature and Humidity values
  //   client.publish("room/temperature", temperatureTemp);
  //   client.publish("room/humidity", humidityTemp);
    
  //   Serial.print("Humidity: ");
  //   Serial.print(h);
  //   Serial.print(" %\t Temperature: ");
  //   Serial.print(t);
  //   Serial.print(" *C ");
  //   Serial.print(f);
  //   Serial.print(" *F\t Heat index: ");
  //   Serial.print(hic);
  //   Serial.println(" *C ");
  //   // Serial.print(hif);
  //   // Serial.println(" *F");
  // }
} 
