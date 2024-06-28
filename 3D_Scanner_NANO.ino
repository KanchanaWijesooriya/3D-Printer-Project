//Editable variables
int scan_amount = 40;                    //Amount of scans for each point. The result is the mean. This would increase the delay for each scan.
int z_axis_height = 10;                  //in cm         //Maximum height of the scaned file
int step_delay = 3000;                   //in us          //Delay for each step for the stepper motor in microseconds
float z_layer_height = 0.5;              //in mm     //Layer height. The amount of mm for each layer.
int lead_screw_rotations_per_cm = 8;     //How many rotations needs the lead screw to make in order to make 1cm.
int steps_per_rotation_for_motor = 200;  //Steps that the motor needs for a full rotation.
int distance_to_center = 10;             //In cm. Distance from sensor to the turntable center in cm

//I/O
int button = 2;
int limit_switch = 10;
int enable_pin = 13;
//Turntable driver pins
int dir_r = 7;
int step_r = 8;
int enable_r = 9;
//Z-axis driver pins
int dir_z = 3;
int step_z = 5;
int enable_z = 6;

//Variables
int scan = 0;          //Activate/deactivate scanning
int scan_changed = 0;  //Scan process was changed
float distance = 0;    //Measured distance
float angle = 0;       //Rotation angle for each loop (0º-360º)
float x = 0;           //X, Y and Z coordinate
float y = 0;
float z = 0;
int z_loop = 0;             //variable used for the z-axis motor rotation
int r_loop = 0;             //variable used for the turntable motor rotation
float measured_analog = 0;  //Analog read from the distance sensor
float analog = 0;           //Analog MEAN
float RADIANS = 0.0;        //Angle in radians. We calculate this value later in Setup loop
int steps_z_height = 0;     //Variable used for the amount of steps in z-axis
int homed = 0;

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(enable_pin, OUTPUT);
  pinMode(limit_switch, INPUT);
  pinMode(button, INPUT);
  analogReference(INTERNAL);
  pinMode(dir_z, OUTPUT);
  pinMode(step_z, OUTPUT);
  pinMode(enable_z, OUTPUT);
  pinMode(dir_r, OUTPUT);
  pinMode(step_r, OUTPUT);
  pinMode(enable_r, OUTPUT);
  digitalWrite(enable_z, HIGH);  //disable the z_azis driver
  digitalWrite(enable_r, HIGH);  //disable the z_azis driver
  digitalWrite(enable_pin, LOW);  //disable the esp32 board

  //Calculate variables
  RADIANS = (3.141592 / 180.0) * (360 / steps_per_rotation_for_motor);  //For 200 steps, RADIANS = 0.0314136
  steps_z_height = (z_layer_height * steps_per_rotation_for_motor * lead_screw_rotations_per_cm) / 10;


  while (!digitalRead(limit_switch) && homed == 0) {
    digitalWrite(enable_z, LOW);  //enable the z_azis driver
    digitalWrite(dir_z, HIGH);    //z_azis spin to left (lowering z-axis)
    digitalWrite(step_z, HIGH);   //z_azis make a step
    delayMicroseconds(step_delay);
    digitalWrite(step_z, LOW);
    delayMicroseconds(step_delay);
  }
  homed = 1;
}

void loop() {
  //Wait till the push button is pressed
  if (digitalRead(button)) {
    if (scan == 1 && scan_changed == 0) {
      scan = 0;
      delay(3000);
      scan_changed = 1;
    }
    if (scan == 0 && scan_changed == 0) {
      scan = 1;
      delay(3000);
      scan_changed = 1;
    }
    scan_changed = 0;
  }

  //If the scanning proces is ON
  if (scan == 1) {
    //We stop when we reach the maximum height
    if (z < z_axis_height) {
      for (int loop_cont = 0; loop_cont < steps_per_rotation_for_motor; loop_cont++) {
        getDistance();
        digitalWrite(enable_r, LOW);  //enable the turntable driver
        digitalWrite(dir_r, HIGH);     //turntable spin to right
        digitalWrite(step_r, HIGH);   //make a step
        delayMicroseconds(step_delay);
        digitalWrite(step_r, LOW);
        delayMicroseconds(step_delay);
        angle = angle + RADIANS;  //Increase the angle by one more unit MAX ANGLE = 3.47

        //Uncomment this for Serial debug
        /*Serial.print(loop_cont);     Serial.print("   "); 
        Serial.print(angle);     Serial.print("   "); 
        Serial.print(distance);     Serial.print("   "); 
        Serial.print(x);     Serial.print("   "); 
        Serial.print(y);     Serial.print("   "); 
        Serial.print(z);     Serial.println("   ");*/
      }
      angle = 0;  //Reset the angle value for next rotation

      /*My threaded rod needs 8 full rotations for 1cm. A full rotation is 200 steps in my case.
      We need 1600 for 1cm. So, we need 80 for 0.5mm. The amount is calulated automaically.
      Just change the variables at the beginning if you want*/
      while (z_loop < steps_z_height) {
        digitalWrite(enable_z, LOW);  //enable the z_azis driver
        digitalWrite(dir_z, HIGH);     //z_azis spin to right
        digitalWrite(step_z, HIGH);   //z_azis make a step
        delayMicroseconds(step_delay);
        digitalWrite(step_z, LOW);
        delayMicroseconds(step_delay);
        z_loop = z_loop + 1;  //Increase the loop by 1
      }
      z = z + z_layer_height;  //Increase the made z-height by 1 unit
      z_loop = 0;              //Reset the z-axis rotation variable

    }  //end of if z_height

    //We finished the scan, we stop the drivers
    else {
      digitalWrite(enable_z, HIGH);
      digitalWrite(enable_r, HIGH);
    }

  }  //if scan
}  //End of void loop

//Function that gets the distance from sensor
double getDistance() {
  //40 * 2 = 80ms
  enable_ESP();
  delay(81);
  disable_ESP();
  /*
  for (int aa = 0; aa < scan_amount; aa++) {
    enable_ESP();
    //measured_analog = analogRead(A0);
    delay(2);
    //analog = analog + measured_analog;
    disable_ESP();
  }*/
  /*distance = analog / scan_amount;                                                                     //Get the mean. Divide the scan by the amount of scans.
  analog = 0;                                                                                          //reset the andlog read total value
  measured_analog = 0;                                                                                 //reset the andlog read value
  distance = mapFloat(distance, 0.0, 1023.0, 0.0, 3.3);                                                //Convert analog pin reading to voltage
  distance = -5.40274 * pow(distance, 3) + 28.4823 * pow(distance, 2) - 49.7115 * distance + 31.3444;  //From datasheet
  distance = distance_to_center - distance;                                                            //the distance d = distance from sensor to center - measured distance
  y = (cos(angle) * distance);
  x = (sin(angle) * distance);*/

  /*//For debug
   * Serial.print(distance); Serial.print("   "); 
  Serial.print(x); Serial.print("   "); 
  Serial.print(y); Serial.print("   "); 
  Serial.print(z); Serial.print("   "); 
  Serial.print(angle); Serial.println("   "); */
}

//Function that maps the value in a float format
float mapFloat(float fval, float val_in_min, float val_in_max, float val_out_min, float val_out_max) {
  return (fval - val_in_min) * (val_out_max - val_out_min) / (val_in_max - val_in_min) + val_out_min;
}

void enable_ESP() {
  //analogWrite(enable_pin, 122);
  digitalWrite(enable_pin, HIGH);
}

void disable_ESP() {
  digitalWrite(enable_pin, LOW);
}