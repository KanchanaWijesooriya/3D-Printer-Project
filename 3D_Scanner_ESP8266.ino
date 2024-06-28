#include <ESP32SharpIR.h>
//For ESP32
#include <WiFi.h>
//For ESP8266
//#include <ESP8266WiFi.h>

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Denne na plyn"
#define WIFI_PASSWORD "012345678"
#define API_KEY "AIzaSyBiBoqei-iTbf34dsFn5MdV8Ybj9qpHv94"
#define DATABASE_URL "https://test001-971d5-default-rtdb.asia-southeast1.firebasedatabase.app/test"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

//Editable variables
int scan_amount = 40;                    //Amount of scans for each point. The result is the mean. This would increase the delay for each scan.
int steps_per_rotation_for_motor = 200;  //Steps that the motor needs for a full rotation.
int distance_to_center = 10;             //In cm. Distance from sensor to the turntable center in cm

//Variables
float distance = 0;  //Measured distance
float angle = 0;     //Rotation angle for each loop (0º-360º)
float x = 0;         //X, Y and Z coordinate
float y = 0;
float z = 0;
float measured_analog = 0;  //Analog read from the distance sensor
float analog = 0;           //Analog MEAN
float sensor_read = 34;     //Reads sensor values
float activate = 35;        //Activates the board
float RADIANS = 0.0;        //Angle in radians. We calculate this value later in Setup loop
String value_string = "";

ESP32SharpIR sensor(ESP32SharpIR::GP2Y0A21YK0F, 34);

void setup() {
  Serial.begin(115200);
  // Setting the filter resolution to 0.1
  sensor.setFilterRate(0.1f);
  connectToWiFi();
  connectToFirebase();
  //Calculate variables
  RADIANS = (3.141592 / 180.0) * (360 / steps_per_rotation_for_motor);  //For 200 steps, RADIANS is approximately 0.0314136
}

void loop() {
  int enabled = analogRead(activate);
  if (enabled == 4095) {
    if (
      Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
      sendDataPrevMillis = millis();
      //Store Sensor Data to a RTDB
      getDistance();
      storeString();
    }
  }
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\nConnected with IP: ");
  Serial.print(WiFi.localIP());
  Serial.println();
}

void connectToFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Signup Successfull!");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

//Function that gets the distance from sensor
void getDistance() {
  //40*2 = 80ms
  for (int aa = 0; aa < scan_amount; aa++) {
    distance += sensor.getDistanceFloat();
    //measured_analog = sensor.getDistanceFloat();
    delay(2);
    //analog = analog + measured_analog;
  }
  //distance = analog / scan_amount;                                                                     //Get the mean. Divide the scan by the amount of scans.
  distance = distance / scan_amount;                                                                     //Get the mean. Divide the scan by the amount of scans.
  //analog = 0;                                                                                          //reset the andlog read total value
  //measured_analog = 0;                                                                                 //reset the andlog read value
  //distance = mapFloat(distance, 0.0, 1023.0, 0.0, 3.3);                                                //Convert analog pin reading to voltage
  //distance = -5.40274 * pow(distance, 3) + 28.4823 * pow(distance, 2) - 49.7115 * distance + 31.3444;  //From datasheet
  distance = distance_to_center - distance;                                                            //the distance d = distance from sensor to center - measured distance
  y = (cos(angle) * distance);
  x = (sin(angle) * distance);

  if (angle >= 3.47) {
    angle = 0;
    z++;
  }
  angle = angle + RADIANS;
  value_string += x;
  value_string += " ";
  value_string += y;
  value_string += " ";
  value_string += z;
  value_string += ",";
  
  //For debug
  /*Serial.print(distance);
  Serial.print("   ");
  Serial.print(x);
  Serial.print("   ");
  Serial.print(y);
  Serial.print("   ");
  Serial.print(z);
  Serial.print("   ");
  Serial.print(angle);
  Serial.println("   ");*/
}

//Function that maps the value in a float format
float mapFloat(float fval, float val_in_min, float val_in_max, float val_out_min, float val_out_max) {
  return (fval - val_in_min) * (val_out_max - val_out_min) / (val_in_max - val_in_min) + val_out_min;
}

void storeX() {
  if (Firebase.RTDB.setFloat(&fbdo, "test/x", x)) {
    /*Serial.println();
    Serial.print("x : ");
    Serial.print(x);
    Serial.print(" Succesfully saved to: " + fbdo.dataPath());
    Serial.println("(" + fbdo.dataType() + ")");*/
  } else {
    Serial.print("FAILED : " + fbdo.errorReason());
  }
}

void storeY() {
  if (Firebase.RTDB.setFloat(&fbdo, "test/y", y)) {
    /*Serial.println();
    Serial.print("y : ");
    Serial.print(x);
    Serial.print(" Succesfully saved to: " + fbdo.dataPath());
    Serial.println("(" + fbdo.dataType() + ")");*/
  } else {
    Serial.print("FAILED : " + fbdo.errorReason());
  }
}

void storeZ() {
  if (Firebase.RTDB.setFloat(&fbdo, "test/z", z)) {
    /*Serial.println();
    Serial.print("z : ");
    Serial.print(z);
    Serial.print(" Succesfully saved to: " + fbdo.dataPath());
    Serial.println("(" + fbdo.dataType() + ")");*/
  } else {
    Serial.print("FAILED : " + fbdo.errorReason());
  }
}

void storeString() {
  if (Firebase.RTDB.setString(&fbdo, "test/reading", value_string)) {
    Serial.println();
    Serial.print("string : ");
    Serial.print(value_string);
    Serial.print(" Succesfully saved to: " + fbdo.dataPath());
    Serial.println("(" + fbdo.dataType() + ")");
  } else {
    Serial.print("FAILED : " + fbdo.errorReason());
  }
}