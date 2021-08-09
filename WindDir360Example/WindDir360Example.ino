

#include <ServoEasing.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Script turns on and off at set times and makes use of 2 LEDs and
// 180 degree servo, an SG90 is recomended. Example code is for a Windvane
// using a windDir mqtt feed
// It uses the 2:14 gear train set up in the Connected Environments Workshop
// The code can bbe adaopted for other servo types/timezones and data feeds

// Set Servo to the Servo Easing Library

ServoEasing servo;
//Servo servo;
// create an instance of the servo class

int angle;

//Set up Wifi and MQTT 
const char* ssid = "yourwifi";
const char* password =  "wifipassword";
const char* mqttServer = "mqttserver";
const int mqttPort = mqttport;
const char* mqttUser = "mqttusername";
const char* mqttPassword = "mqttpassword";
WiFiClient windvane;
PubSubClient client(windvane);

// Define NTP Client to get time

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
void setup()

{
  servo.write(0); 
  delay(1000); 

 // turn off on board LED
  digitalWrite(2, HIGH); 
  pinMode (LED_BUILTIN, OUTPUT); // Define LED as output
  pinMode(5, OUTPUT);  //LED STRIP1
  pinMode(10, OUTPUT);  //LED STRIP2
  
 // WiFi.mode(WIFI_STA);
 // WiFi.softAPdisconnect(true);

// Start up Servo Sweep - also allows calibration

  servo.attach(4);
  servo.setEasingType(EASE_SINE_IN_OUT);
  servo.setSpeed(20);
  Serial.print("Moving to 0");
  servo.easeTo(4);
  delay (1000);
  Serial.print("Moving to 360");
  servo.easeTo(180);
  delay (1000);
  Serial.print("Moving to 0");
  servo.easeTo(4);
  Serial.print("Pausing for 5 seconds");
  delay (5000);

 // Keep Wifi from Sleeping
 
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if(client.connect("WindVane1", mqttUser, mqttPassword)) 
{
     Serial.println("connected");

// Publish to an MQTT topic that the device is alive
     
     client.publish("Devices/Wind Vane/","Wind Vane Connected");
    }else{
      Serial.print("failed state ");
      Serial.print(client.state());
      delay(2000);
    }

// Initialize a NTPClient to get time and set timezone
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);   
  }

// Set your MQTT Topic

  client.subscribe("downhamweather/windDir");
}
//Reconnect if connection lost and reset topic

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
     
// ... and resubscribe
      client.subscribe("downhamweather/windDir");
 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void callback(char* topic, byte* payload, unsigned intlength)
{

// Print when a message is received

Serial.print("Message arrived in topic: ");


// Convert the data to integer
  int wind = atoi((char *)payload);

// Map the data value to an angle - this value needs to be edited to calibrate your servo

  angle = map(wind, 4, 360, 0, 180);
  
  servo.setEasingType(EASE_SINE_IN_OUT);
  servo.setSpeed(15);
//servo.write(angle);
  servo.easeTo(angle);
  Serial.print("Wind Direction ");
  Serial.println(wind);
  delay(1000);   // Allow transit time
 
}

void loop()

{


   // get time

  timeClient.update();

  unsigned long epochTime = timeClient.getEpochTime();
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  
  int currentHour = timeClient.getHours();
  int currentHour24 = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  
//Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

// Set up the Awake time - this turns the device on between 7am and 10pm
// and turns off lights, moves servo to 0 if outside this time. 
 
 if ((currentHour24 >= 7) && (currentHour24 <= 22)) {

  // Turns lights on - runs once
  
// Turn on LEDS but keep onboard LED off
 digitalWrite(2, HIGH);
 digitalWrite(5, HIGH);
 digitalWrite(10, HIGH);

 
// Connect to mqtt if not already connected
  if (!client.connected()) {
    client.connect("WindVane", mqttUser, mqttPassword);

// Your MQTT topic    
    client.subscribe("downhamweather/windDir");
  }
  
  // and now wait for 1s, regularly calling the mqtt loop function
  for (int i=0; i<1000; i++) {
    client.loop();
    delay(1);
  }
    
  }

  // If its not within the set hours - turn off light and servo

  else{
    Serial.print("Sleeping");
    servo.easeTo(0);
    delay(1000);
    digitalWrite(2, HIGH);
    digitalWrite(5, LOW);
    digitalWrite(10, LOW);
    delay(60000);

  }
           
}
