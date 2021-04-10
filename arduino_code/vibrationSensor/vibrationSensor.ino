#include <Ethernet.h>

#include <Adafruit_LIS3DH.h>

#include <Adafruit_Sensor.h>

#include <PubSubClientColour.h>


// I2C
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
byte mac[] = {
  0x3E,
  0x8A,
  0x53,
  0xAA,
  0x09,
  0xCE
};

IPAddress ip(192, 168, 0, 16);
IPAddress server(192, 168, 0, 21);

EthernetClient ethClient;
PubSubClientColour client(ethClient);

bool isWorking = true;
//bool posReseived = false;
byte motorInfoReceived[5]; //0 or 1
byte location = 0;
byte sensor_id = 0;
byte motor_state[5]; //0 or 1
byte motor_speed[5]; //0 .. 255
char motor_direction[5]; //r or l
char pos[5];
unsigned long msgTimer = 0;

/*
 * location:
 * 0 = loading table
 * 1 = rolling table
 * 2 = roll
 * 3 = quality table
 * 4 = unloading table
 */

/*
 * motors:
 * 0 = loading table
 * 1 = rolling table
 * 2 = roll
 * 3 = quality table
 * 4 = unloading table
 */

byte loading_table = 0;
byte rolling_table = 1;
byte roll = 2;
byte quality_table = 3;
byte unloading_table = 4;

char * topicCat = (char * ) malloc(45);
char loading_t[] = "/loading\0";
char rolling_t[] = "/rolling\0";
char quality_t[] = "/quality\0";
char unloading_t[] = "/unloading\0";
char vibration_s[] = "/arduinoVibrationSensor\0";

void setup(void) {

  memset(topicCat, '\0', sizeof(topicCat));
  memset(pos, '0', sizeof(pos));
  memset(motorInfoReceived, 0, sizeof(motorInfoReceived));
  memset(motor_state, 0, sizeof(motor_state));
  memset(motor_speed, 0, sizeof(motor_speed));
  memset(motor_direction, '\0', sizeof(motor_direction));
  Serial.begin(115200);

  if (!lis.begin(0x18)) { // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1);
  }

  lis.setRange(LIS3DH_RANGE_2_G); // 2, 4, 8 or 16 G

  Serial.print("Range = ");
  Serial.print(2 << lis.getRange());
  Serial.println("G");

  client.setServer(server, 1883);
  client.setCallback(callback);

  Ethernet.begin(mac, ip);

  // Allow the hardware to sort itself out
  delay(1500);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {

    memset(topicCat, '\0', sizeof(topicCat));
    strcpy(topicCat, vibration_s);
    memmove( & topicCat[0], & topicCat[1], strlen(topicCat)); //del "/"

    if (client.connect(topicCat, "futurelab", "FutureLab123")) { // "arduinoVibrationSensor"

      strcat(topicCat, "|connected"); // "arduinoVibrationSensor|connected"
      client.publish("/tables/connection", topicCat);

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, vibration_s);
      strcat(topicCat, "/control");
      client.subscribe(topicCat); // "/arduinoVibrationSensor/control"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, vibration_s);
      strcat(topicCat, "/data");
      client.subscribe(topicCat); // "/arduinoVibrationSensor/data"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, loading_t);
      strcat(topicCat, "/motorInfo");
      client.subscribe(topicCat); // "/loading/motorInfo"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, rolling_t);
      strcat(topicCat, "/motorInfo");
      client.subscribe(topicCat); // "/rolling/motorInfo"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, quality_t);
      strcat(topicCat, "/motorInfo");
      client.subscribe(topicCat); // "/quality/motorInfo"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, unloading_t);
      strcat(topicCat, "/motorInfo");
      client.subscribe(topicCat); // "/unloading/motorInfo"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, loading_t);
      strcat(topicCat, "/MatPosition");
      client.subscribe(topicCat); // "/loading/MatPosition"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, rolling_t);
      strcat(topicCat, "/MatPosition");
      client.subscribe(topicCat); // "/rolling/MatPosition"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, quality_t);
      strcat(topicCat, "/MatPosition");
      client.subscribe(topicCat); // "/quality/MatPosition"

      memset(topicCat, '\0', sizeof(topicCat));
      strcpy(topicCat, unloading_t);
      strcat(topicCat, "/MatPosition");
      client.subscribe(topicCat); // "/unloading/MatPosition"

    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char * topic, byte * payload, unsigned int length) {

  // Allocate the correct amount of memory for the payload copy
  byte * p = (byte * ) malloc(length);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, vibration_s);
  strcat(topicCat, "/control");

  if (strcmp(topic, topicCat) == 0) // "/arduinoVibrationSensor/control"
  {
    for (int i = 0; i < length; i++) {
      //Serial.print((char)p[i]);

      if ((char) p[i] == 's') //start
      {
        isWorking = true;
        memset(topicCat, '\0', sizeof(topicCat));
        strcpy(topicCat, vibration_s);
        strcat(topicCat, "/info");
        client.publish(topicCat, "started");
      } else if ((char) p[i] == 'p') //pause
      {
        isWorking = false;
        memset(topicCat, '\0', sizeof(topicCat));
        strcpy(topicCat, vibration_s);
        strcat(topicCat, "/info");
        client.publish(topicCat, "stopped");
        memset(pos, '0', sizeof(pos));
        memset(motorInfoReceived, 0, sizeof(motorInfoReceived));
        memset(motor_state, 0, sizeof(motor_state));
        memset(motor_speed, 0, sizeof(motor_speed));
        memset(motor_direction, '\0', sizeof(motor_direction));
      }
    }

    free(p);
    return;
  }

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, vibration_s);
  strcat(topicCat, "/data");

  if (strcmp(topic, topicCat) == 0) //1 byte = location, 2 byte = sensor_id, topic =  "/arduinoVibrationSensor/data"
  {
    isWorking = true;
    location = p[0] - 48; //ascii transform
    sensor_id = p[1] - 48;

    memset(pos, '0', sizeof(pos));
    memset(motorInfoReceived, 0, sizeof(motorInfoReceived));
    memset(motor_state, 0, sizeof(motor_state));
    memset(motor_speed, 0, sizeof(motor_speed));
    memset(motor_direction, '\0', sizeof(motor_direction));

    free(p);
    return;
  }

  //MOTOR__INFO

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, loading_t);
  strcat(topicCat, "/motorInfo");

  if (strcmp(topic, topicCat) == 0) // "/loading/motorInfo"
  {
    if (strstr(p, "isStarted|") != NULL) {
      motor_state[0] = p[10] - 48;
      motorInfoReceived[0]++;
    } else if (strstr(p, "direction|") != NULL) {
      motor_direction[0] = (char) p[10];
      motorInfoReceived[0]++;
    } else if (strstr(p, "speed|") != NULL) {
      int power = 1;
      int speed = 0;
      char temp;
      for (int i = length - 1; i >= 6; i--) {
        temp = ((char) p[i]);
        speed += atoi( & temp) * power;
        power *= 10;
      }
      motor_speed[0] = (byte) speed;
      motorInfoReceived[0]++;
    }

    free(p);
    return;
  }

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, rolling_t);
  strcat(topicCat, "/motorInfo");

  if (strcmp(topic, topicCat) == 0) //"/rolling/motorInfo"
  {

    if (strstr(p, "isStarted|") != NULL) {
      motor_state[1] = p[10] - 48;
      motorInfoReceived[1]++;
    } else if (strstr(p, "direction|") != NULL) {
      motor_direction[1] = (char) p[10];
      motorInfoReceived[1]++;
    } else if (strstr(p, "speed|") != NULL) {
      int power = 1;
      int speed = 0;
      char temp;
      for (int i = length - 1; i >= 6; i--) {
        temp = ((char) p[i]);
        speed += atoi( & temp) * power;
        power *= 10;
      }
      motor_speed[1] = (byte) speed;
      motorInfoReceived[1]++;
    } else if (strstr(p, "isRollingStarted|") != NULL) {
      motor_state[2] = p[17] - 48;
      motorInfoReceived[2]++;
    } else if (strstr(p, "rollingDirection|") != NULL) {
      motor_direction[2] = (char) p[17];
      motorInfoReceived[2]++;
    } else if (strstr(p, "rollingSpeed|") != NULL) {
      int power = 1;
      int speed = 0;
      char temp;
      for (int i = length - 1; i >= 13; i--) {
        temp = ((char) p[i]);
        speed += atoi( & temp) * power;
        power *= 10;
      }
      motor_speed[2] = (byte) speed;
      motorInfoReceived[2]++;
    }

    free(p);
    return;
  }

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, quality_t);
  strcat(topicCat, "/motorInfo");

  if (strcmp(topic, topicCat) == 0) // "/quality/motorInfo"
  {
    if (strstr(p, "isStarted|") != NULL) {
      motor_state[3] = p[10] - 48;
      motorInfoReceived[3]++;
    } else if (strstr(p, "direction|") != NULL) {
      motor_direction[3] = (char) p[10];
      motorInfoReceived[3]++;
    } else if (strstr(p, "speed|") != NULL) {
      int power = 1;
      int speed = 0;
      char temp;
      for (int i = length - 1; i >= 6; i--) {
        temp = ((char) p[i]);
        speed += atoi( & temp) * power;
        power *= 10;
      }
      motor_speed[3] = (byte) speed;
      motorInfoReceived[3]++;
    }

    free(p);
    return;
  }

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, unloading_t);
  strcat(topicCat, "/motorInfo");

  if (strcmp(topic, topicCat) == 0) // "/unloading/motorInfo"
  {
    if (strstr(p, "isStarted|") != NULL) {
      motor_state[4] = p[10] - 48;
      motorInfoReceived[4]++;
    } else if (strstr(p, "direction|") != NULL) {
      motor_direction[4] = (char) p[10];
      motorInfoReceived[4]++;
    } else if (strstr(p, "speed|") != NULL) {
      int power = 1;
      int speed = 0;
      char temp;
      for (int i = length - 1; i >= 6; i--) {
        temp = ((char) p[i]);
        speed += atoi( & temp) * power;
        power *= 10;
      }
      motor_speed[4] = (byte) speed;
      motorInfoReceived[4]++;
    }

    free(p);
    return;
  }

  //MAT__POSITION

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, unloading_t);
  strcat(topicCat, "/MatPosition");

  if (strcmp(topic, topicCat) == 0) // "/unloading/MatPosition"
  {
    int numbOfNulls = 5 - (length - 9);

    for (int i = 0; i < numbOfNulls; i++) {
      pos[i] = '0';
    }

    int startIndex = numbOfNulls;
    for (int i = 9; i < length; i++) {
      pos[startIndex] = (char) p[i];
      startIndex++;
    }

    //posReseived = true;
    free(p);
    return;
  }

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, quality_t);
  strcat(topicCat, "/MatPosition");

  if (strcmp(topic, topicCat) == 0) //"/quality/MatPosition"
  {
    int numbOfNulls = 5 - (length - 9);

    for (int i = 0; i < numbOfNulls; i++) {
      pos[i] = '0';
    }

    int startIndex = numbOfNulls;
    for (int i = 9; i < length; i++) {
      pos[startIndex] = (char) p[i];
      startIndex++;
    }

    //posReseived = true;
    free(p);
    return;
  }

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, rolling_t);
  strcat(topicCat, "/MatPosition");

  if (strcmp(topic, topicCat) == 0) //"/rolling/MatPosition"
  {
    int numbOfNulls = 5 - (length - 9);

    for (int i = 0; i < numbOfNulls; i++) {
      pos[i] = '0';
    }

    int startIndex = numbOfNulls;
    for (int i = 9; i < length; i++) {
      pos[startIndex] = (char) p[i];
      startIndex++;
    }

    //posReseived = true;

    free(p);
    return;
  }

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, loading_t);
  strcat(topicCat, "/MatPosition");

  if (strcmp(topic, topicCat) == 0) // "/loading/MatPosition"
  {
    int numbOfNulls = 5 - (length - 9);

    for (int i = 0; i < numbOfNulls; i++) {
      pos[i] = '0';
    }

    int startIndex = numbOfNulls;
    for (int i = 9; i < length; i++) {
      pos[startIndex] = (char) p[i];
      startIndex++;
    }

    //posReseived = true;

    free(p);
    return;
  }

  free(p);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  if (isWorking)
    checkVibration();

}

void checkVibration() {
  Serial.println("here1");
  int arrSize = 52; //location + sensor_id + motors_state + motor_direction + motor_speed + pos + 3*5*2
  byte arr[arrSize];

  memset(arr, 0, sizeof(arr));

  arr[0] = location;
  arr[1] = sensor_id;

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, loading_t);
  strcat(topicCat, "/control");
  client.publish(topicCat, "i\0");

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, rolling_t);
  strcat(topicCat, "/control");
  client.publish(topicCat, "i\0");

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, quality_t);
  strcat(topicCat, "/control");
  client.publish(topicCat, "i\0");

  memset(topicCat, '\0', sizeof(topicCat));
  strcpy(topicCat, unloading_t);
  strcat(topicCat, "/control");
  client.publish(topicCat, "i\0");

  Serial.println("here2");
  /*
  //posReseived = false;
  msgTimer = millis();
  while(!checkMotorInfoReceived() && millis() - msgTimer < 2000)
  {
    client.loop();
  }

  if(!checkMotorInfoReceived())
  {
      memset(motorInfoReceived, 0, sizeof(motorInfoReceived));
      return;
  }
  
  memset(motorInfoReceived, 0, sizeof(motorInfoReceived));
  //posReseived = false;
  */

  arr[2] = motor_state[0];
  arr[3] = motor_state[1];
  arr[4] = motor_state[2];
  arr[5] = motor_state[3];
  arr[6] = motor_state[4];

  arr[7] = (byte) motor_direction[0];
  arr[8] = (byte) motor_direction[1];
  arr[9] = (byte) motor_direction[2];
  arr[10] = (byte) motor_direction[3];
  arr[11] = (byte) motor_direction[4];

  arr[12] = motor_speed[0];
  arr[13] = motor_speed[1];
  arr[14] = motor_speed[2];
  arr[15] = motor_speed[3];
  arr[16] = motor_speed[4];

  arr[17] = (byte) pos[0];
  arr[18] = (byte) pos[1];
  arr[19] = (byte) pos[2];
  arr[20] = (byte) pos[3];
  arr[21] = (byte) pos[4];

  //memset(pos, '0' , sizeof(pos));

  Serial.println("here3");
  for (int i = 22; i < arrSize; i = i + 6) {
    lis.read();
    /*
    Serial.print("x=");
    Serial.println(lis.x);
    Serial.print("y=");
    Serial.println(lis.y);
    Serial.print("z=");
    Serial.println(lis.z);
    */
    arr[i] = (byte)((lis.x >> 8) & 0x00ff);
    arr[i + 1] = (byte)(lis.x & 0x00ff);
    arr[i + 2] = (byte)((lis.y >> 8) & 0x00ff);
    arr[i + 3] = (byte)(lis.y & 0x00ff);
    arr[i + 4] = (byte)((lis.z >> 8) & 0x00ff);
    arr[i + 5] = (byte)(lis.z & 0x00ff);
    /*
    Serial.print(arr[i]);
    Serial.print(",");
    Serial.print(arr[i+1]);
    Serial.print(",");
    Serial.print(arr[i+2]);
    Serial.print(",");
    Serial.print(arr[i+3]);
    Serial.print(",");
    Serial.print(arr[i+4]);
    Serial.print(",");
    Serial.print(arr[i+5]);
    Serial.println();
    Serial.println();
    */
    delay(120);
  }
  for (int i = 0; i < arrSize; i++) {
    Serial.print(arr[i]);
  }
  Serial.println();

  client.publish("/vibrationSensor/vibration", arr, arrSize);

}

boolean checkMotorInfoReceived() {
  for (int i = 0; i < sizeof(motorInfoReceived); i++) {
    if (motorInfoReceived[i] != 3)
      return false;
  }

  return true;
}

/*
 sensors_event_t event; 
 lis.getEvent(&event);
 //Display the results (acceleration is measured in m/s^2) 
 Serial.print("\t\tX: "); Serial.print(event.acceleration.x);
 Serial.print(" \tY: "); Serial.print(event.acceleration.y); 
 Serial.print(" \tZ: "); Serial.print(event.acceleration.z); 
 Serial.println(" m/s^2 ");
 Serial.println();
 */

/*
  if(location == loading_table)
  {
    client.publish("/loading/control", "i\0");
  }
  else if(location == rolling_table || location == roll)
  {
    client.publish("/rolling/control", "i\0");  
  }
  else if(location == quality_table)
  {
    client.publish("/quality/control", "i\0");  
  }
  else if(location == unloading_table)
  {
    client.publish("/unloading/control", "i\0");  
  }
 */

/*
    if(location==rolling_table){
   if (strstr(p, "isStarted|") != NULL) {
     motor_state[1] = p[10] - 48;
     motorInfoReceived[1] = 1;
   }
   else
   {
     //ignore 
   }
   }
   else if (location==roll)
   {
     if (strstr(p, "isRollingStarted|") != NULL) {
       motor_state[2] = p[17] - 48;
       motorInfoReceived[2] = 1;
   }
   else
   {
     //ignore 
   }
   }
 */