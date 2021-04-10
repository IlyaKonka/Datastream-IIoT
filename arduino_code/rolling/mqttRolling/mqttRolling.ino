#include <SPI.h>

#include <Ethernet.h>

#include <PubSubClient.h>


// Update these with values suitable for your network.
byte mac[] = {
  0x9E,
  0x71,
  0x61,
  0x2A,
  0x8E,
  0xE9
};
//IPAddress ip(192,168,0,110);
//IPAddress server(192,168,0,102);
//IPAddress ip (169,254,166,101);
//IPAddress server (169,254,166,232);
//IPAddress ip (169,254,65,101);
//IPAddress server (169,254,65,51); 
IPAddress ip(192, 168, 0, 12);
IPAddress server(192, 168, 0, 21);

EthernetClient ethClient;
PubSubClient client(ethClient);

char speedArr[4]; // speed counts from 0 to   
char * receivedMsg = (char * ) malloc(sizeof(char) * 2);
char * positionPointer = (char * ) malloc(sizeof(char) * 3);
char * button = (char * ) malloc(sizeof(char) * 2);

char command = 'n'; //nothing
boolean isManual = false;

//motor
int motor1_A = 2;
int motor1_B = 4;
int motor1_Speed = 3;

int motor2_A = 6;
int motor2_B = 7;
int motor2_Speed = 9;

//default
int speed = 170;
char direction = 'r';
char started = '0';
int rSpeed = 200;
char rDirection = 'r';
char rStarted = '0';
bool writeInterval = false;

//vars
unsigned long startTime;
int interval;
char thickness[3];
char currRfid[9]; //use
char matPosition[9];
char currMatPosition[9];
unsigned long msgTimer = 0;

bool isFirstOnTheTbl = false;
bool isQualityTblReady = false;
bool isAfterRolling = false;
bool isDisplayProblemSent = false;
//bool stopBeforeQuality =false;
bool isMovingToQuality = false;

int dCounter = 0;
int bCounter = 0;
int mCounter = 0;

void setup() {
  //dCounter = 0;
  //bCounter = 0;
  //mCounter = 0;
  //memset(positionPointer, '\0', sizeof(positionPointer));

  strcpy(positionPointer, "ff\0");
  memset(matPosition, '\0', sizeof(matPosition));
  memset(receivedMsg, '\0', sizeof(receivedMsg));
  memset(button, '\0', sizeof(button));
  Serial.begin(115200);
  Serial.setTimeout(100); //communication with nano(display)

  //ir sensors
  pinMode(A0, OUTPUT); //1
  pinMode(A1, OUTPUT); //2
  pinMode(A2, OUTPUT); //3
  pinMode(A3, INPUT); //4
  //pinMode(A4,INPUT);//5
  //pinMode(A5,INPUT);//6
  //pinMode(5,INPUT);//7
  //pinMode(8,INPUT);//8

  client.setServer(server, 1883);
  client.setCallback(callback);

  //Motor
  pinMode(motor1_A, OUTPUT);
  pinMode(motor1_B, OUTPUT);
  pinMode(motor2_A, OUTPUT);
  pinMode(motor2_B, OUTPUT);

  Ethernet.begin(mac, ip);

  // Allow the hardware to sort itself out
  delay(1500);

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected() && !isManual) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect

    if (client.connect("arduinoRollingTable", "futurelab", "FutureLab123")) {
      //if (client.connect("arduinoRollingTable")) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/tables/connection", "arduinoRollingTable|connected");
      // ... and resubscribe

      client.subscribe("/rolling/control");
      client.subscribe("/rolling/RollThickness");
      client.subscribe("/rolling/MatLoadingInfo");
      client.subscribe("/rolling/MatReadyToQCheck");

    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      //delay(5000);
      for (int i = 0; i < 50; i++) {
        checkButtonInfo();
        if (isManual)
          break;
      }
    }
  }
}

void callback(char * topic, byte * payload, unsigned int length) {

  // Allocate the correct amount of memory for the payload copy
  byte * p = (byte * ) malloc(length);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);

  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");

  if (strcmp(topic, "/rolling/control") == 0) {
    bool isSpeed = false;

    for (int i = 0; i < length; i++) {
      //Serial.print((char)p[i]);

      if ((char) p[i] == 'l') //left
      {
        direction = 'l';
        rDirection = 'l';
        client.publish("/rolling/info", "direction|l");
        client.publish("/rolling/info", "rollingDirection|l");
      } else if ((char) p[i] == 'r') //right
      {
        direction = 'r';
        rDirection = 'r';
        client.publish("/rolling/info", "direction|r");
        client.publish("/rolling/info", "rollingDirection|r");
      } else if ((char) p[i] == 's') //start
      {
        motor(speed, direction);
        //started='1';
        client.publish("/rolling/info", "isStarted|1");
      } else if ((char) p[i] == 'p') //pause
      {
        stopMotor();
        //started='0';
        client.publish("/rolling/info", "isStarted|0");
      } else if ((char) p[i] == 't') //loopTime info
      {
        if (writeInterval == true)
          writeInterval = false;
        else
          writeInterval = true;

        //writeInterval = !writeInterval; //does not work

        if (writeInterval)
          client.publish("/rolling/info", "loopTime|1");
        else
          client.publish("/rolling/info", "loopTime|0");
      } else if ((char) p[i] == 'i') {
        writeInfo();
      } else if ((char) p[i] == 'm') {
        client.publish("/rolling/info", "isRollingStarted|1");
        rolling(rSpeed, rDirection);
      } else if ((char) p[i] == 'z') {
        client.publish("/rolling/info", "isRollingStarted|0");
        stopRolling();
      } else {
        isSpeed = true;
      }
    }
    //Serial.println();

    if (isSpeed) {
      int power = 1;
      speed = 0;
      char temp;
      for (int i = length - 1; i >= 0; i--) {
        temp = ((char) p[i]);
        speed += atoi( & temp) * power;
        power *= 10;
      }

      if (speed < 0) //min
        speed = 100;
      if (speed > 255) //max
        speed = 255;

      rSpeed = speed - 20; //changeable 
      //Serial.println(freeRam());
      //sprintf(speedArr, "%ld", speed); 
      //char* speedArrPointer = &speedArr[0]; 
      memset(speedArr, '\0', sizeof(speedArr));
      itoa(speed, speedArr, 10);
      char speedMsg[7] = "speed|\0";
      char rSpeedMsg[14] = "rollingSpeed|\0";
      char * concSpeed = (char * ) malloc(strlen(rSpeedMsg) + strlen( & speedArr[0]));
      memset(concSpeed, '\0', sizeof(concSpeed));
      strncpy(concSpeed, & speedMsg[0], sizeof(speedMsg)); // copy name into the new var 
      strcat(concSpeed, & speedArr[0]); // add the extension 
      client.publish("/rolling/info", concSpeed);

      memset(speedArr, '\0', sizeof(speedArr));
      itoa(rSpeed, speedArr, 10);
      memset(concSpeed, '\0', sizeof(concSpeed));
      strncpy(concSpeed, & rSpeedMsg[0], sizeof(rSpeedMsg)); // copy name into the new var 
      strcat(concSpeed, & speedArr[0]); // add the extension 
      client.publish("/rolling/info", concSpeed);

      free(concSpeed);
    }
    //Serial.println();
  } else if (strcmp(topic, "/rolling/MatLoadingInfo") == 0) {
    char warningLine[30] = "Id is't correct\0";
    if (length != 8)
      client.publish("/rolling/warning", warningLine);

    for (int i = 0; i < length; i++) {
      currRfid[i] = (char) p[i];
    }
    currRfid[9] = '\0';
    delay(1000);
    waitFeedback("rcd\0", 4);
    delay(500);
    waitFeedback(currRfid, 9);
    delay(1500); //extra
    client.publish("/loading/MatReadyToRoll", currRfid);
    isFirstOnTheTbl = true;
    motor(speed, 'r');
    waitFeedback("pfr", 4);
  } else if (strcmp(topic, "/rolling/RollThickness") == 0) {
    char warningLine[33] = "Info is't correct.\0";
    char warningLineId[30] = "Id is't correct\0";
    bool isWritten = false;
    if (length != 10 && length != 11)
      client.publish("/rolling/warning", warningLine);

    for (int i = 0; i < 8; i++) {
      if (currRfid[i] != (char) p[i]) {
        client.publish("/rolling/warning", warningLineId);
        break;
      }
    }
    memset(thickness, '\0', sizeof(thickness));
    int tCounter = 0;
    for (int i = 9; i < length; i++) {
      thickness[tCounter] = (char) p[i];
      tCounter++;
    }
    char tmpThickness[4];
    if (tCounter == 1) {
      tmpThickness[0] = '0';
      tmpThickness[1] = '0';
      tmpThickness[2] = thickness[0];
      tmpThickness[3] = '\0';
    } else if (tCounter == 2) {
      tmpThickness[0] = '0';
      tmpThickness[1] = thickness[0];
      tmpThickness[2] = thickness[1];
      tmpThickness[3] = '\0';
    }
    delay(1500);
    waitFeedback("tns\0", 4);
    delay(500);
    waitFeedback(tmpThickness, 4);
    delay(2000);
    rollingMode(thickness);
  } else if (strcmp(topic, "/rolling/MatReadyToQCheck") == 0) {
    char warningLine[49] = "Id is't correct.Expected: \0";
    bool isWritten = false;
    for (int i = 0; i < length; i++) {
      if ((char) p[i] != currRfid[i]) {
        if (!isWritten) {
          //strcpy(warningLine, "Id is't correct.Expected: \0"); // copy name into the new var 
          strcat(warningLine, & currRfid[0]); // add the extension 
          client.publish("/rolling/warning", warningLine);
          isWritten = true;
        }
      }
    }
    isQualityTblReady = true;
  }

  free(p);

}

void rollingMode(char tnss[]) {
  int thcknss;
  thcknss = atoi( & tnss[0]);
  int times = 0;
  if (thcknss <= 19)
    times = 9;
  else if (thcknss <= 39)
    times = 7;
  else if (thcknss <= 59)
    times = 5;
  else if (thcknss <= 79)
    times = 3;
  else if (thcknss <= 99)
    times = 1;

  char tms[4];
  itoa(times, tms, 10);
  tms[1] = '0';
  tms[2] = '0';
  tms[3] = '\0';
  waitFeedback(tms, 4);
  char dir = 'r';
  for (int i = 1; i <= times; i++) {
    rollProcess(dir);

    if (dir == 'r')
      dir = 'l';
    else
      dir = 'r';

    if (i != times) {
      itoa((times - i), tms, 10);
      tms[1] = '0';
      tms[2] = '0';
      tms[3] = '\0';
      waitFeedback(tms, 4);
    }
  }
  isAfterRolling = true;
}

void rollProcess(char dir) {
  speed = getMotorSpeed(170);
  rSpeed = getRollSpeed(200);
  int beforeRollSensor = 1; //changeable
  int afterRollSensor = 6; //changeable

  if (dir == 'r') {
    motor(speed, 'r');
    rolling(rSpeed, 'r');
    while (currMatPosition[afterRollSensor] != '1') {
      checkPosition();
      client.loop();
    }
  } else if (dir == 'l') {
    motor(speed, 'l');
    rolling(rSpeed, 'l');
    while (currMatPosition[beforeRollSensor] != '1') {
      checkPosition();
      client.loop();
    }
  }

  stopMotor();
  stopRolling();
}

int getMotorSpeed(int time) //TODO
{
  return 170;
}
int getRollSpeed(int time) //TODO
{
  return 200;
}

void waitFeedback(char msg[], int msgSize) {
  long tmr = millis();
  for (int i = 0; i < 1; i++) //only 1 time
  {
    Serial.write(msg, msgSize);
    Serial.flush();
    Serial.readBytes(receivedMsg, 2);

    while (receivedMsg[0] == '\0' && millis() - tmr < 10000)
      Serial.readBytes(receivedMsg, 2);

    if (receivedMsg[0] != '\0') {
      if (strcmp(receivedMsg, "r\0") == 0)
        memset(receivedMsg, '\0', sizeof(receivedMsg));

      while (Serial.available())
        Serial.read();
      return;
    } else if (!isDisplayProblemSent) {
      isDisplayProblemSent = true;
      client.publish("/rolling/warning", "No display connected");
      waitFeedback(msg, msgSize);
    }
  }
  isDisplayProblemSent = false;

}

void checkButtonInfo() {
  Serial.readBytes(button, 2);

  if (strcmp(button, "f\0") == 0) {
    if (client.connected())
      client.disconnect();

    isManual = true;
    if (command == 'f' || command == 'b') {
      command = 'o';
      stopMotor();
    } else //command=n,o,s
    {
      command = 'f';
      motor(speed, 'r');
    }
  } else if (strcmp(button, "b\0") == 0) {
    if (client.connected())
      client.disconnect();

    isManual = true;
    if (command == 'f' || command == 'b') {
      command = 'o';
      stopMotor();
    } else //command=n,o,s
    {
      command = 'b';
      motor(speed, 'l');
    }
  } else if (strcmp(button, "s\0") == 0) {
    if (client.connected())
      client.disconnect();

    isManual = true;

    if (command == 'f') {
      command = 's';
      rolling(rSpeed, 'r');
    } else if (command == 'b') {
      command = 's';
      rolling(rSpeed, 'l');
    } else if (command == 's') {
      command = 'o';
      stopRolling();
    } else //command=n,o
    {
      command = 's';
      rolling(rSpeed, 'r');
    }
  } else if (strcmp(button, "o\0") == 0) {
    if (client.connected())
      client.disconnect();

    if (command == 'n') {
      isManual = true;
      command = 'o';
    } else //command=f,b,s,o
    {
      isManual = false;
      command = 'n';
      stopMotor();
      stopRolling();
      Ethernet.begin(mac, ip);
      client.setServer(server, 1883);
      client.setCallback(callback);
      delay(1500);
    }
  }

  memset(button, '\0', sizeof(button));
}

void loop() {

  startTime = millis();
  //Serial.println(freeRam());
  //server

  checkButtonInfo();

  if (!client.connected() && !isManual) {
    reconnect();
  }
  client.loop();

  //after rolling
  if (isAfterRolling) {
    client.publish("/rolling/MatAfterRoll", currRfid);
    isAfterRolling = false;
    /*
    delay(500);
    waitFeedback("fwd\0",4);
    delay(2000);
    speed = 170; //changeable (should be default)
    motor(speed,'r');
    stopBeforeQuality=true;
    */
    delay(500);
    waitFeedback("wfq\0", 4);
    delay(1200);
    client.publish("/quality/MatRollInfo", currRfid);
  }

  //transport to quality table
  if (isQualityTblReady) {
    isQualityTblReady = false;
    waitFeedback("rtq\0", 4);
    motor(speed, 'r');
    isMovingToQuality = true;
  }

  //ir sensors
  checkPosition();

  //write info loop time
  if (writeInterval) {
    interval = millis() - startTime;
    char intervalArr[7];
    itoa(interval, intervalArr, 10); // here 10 means decimal
    client.publish("/rolling/loopTime", intervalArr);
    delay(100);
  }

}

int freeRam() {
  extern int __heap_start, * __brkval;
  int v;
  return (int) & v - (__brkval == 0 ? (int) & __heap_start : (int) __brkval);
}

void checkAction() {
  if (isFirstOnTheTbl) {
    if (currMatPosition[1] == '1')
      dCounter++;

    if (dCounter == 3) {
      stopMotor();
      dCounter = 0;
      isFirstOnTheTbl = false;
      client.publish("/rolling/RollQuery", currRfid);
    }
  }
  /*
  if(stopBeforeQuality && currMatPosition[6]=='1' ) //changeable
  {
    stopRolling();
  }
  
  if(stopBeforeQuality )
  {
     if(currMatPosition[7]=='1'){
        bCounter++;
     }
        

      if(bCounter==3)
      {
        stopMotor();
        bCounter = 0;
        stopBeforeQuality = false;
        waitFeedback("wfq\0",4);
        delay(1200);
        client.publish("/quality/MatRollInfo",currRfid);
      }
  }
  */
  if (isMovingToQuality) {
    if (strstr(currMatPosition, "1") == NULL)
      mCounter++;

    if (mCounter == 3) {
      stopMotor();
      mCounter = 0;
      isMovingToQuality = false;
      client.publish("/rolling/info", "Rolling is done.");
      delay(2000);
      waitFeedback("rdy\0", 4);
      delay(1000);
      memset(currRfid, '\0', sizeof(currRfid));
    }
  }
}

char * getPosMqtt(char * currMatPosition) {
  char * arrMqttPos = (char * ) malloc(sizeof(char) * 10);
  if (strcmp(currMatPosition, "18\0") == 0) {
    strcpy(arrMqttPos, "17640\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "20\0") == 0) {
    strcpy(arrMqttPos, "19530\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "21\0") == 0) {
    strcpy(arrMqttPos, "21420\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "23\0") == 0) {
    strcpy(arrMqttPos, "23310\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "26\0") == 0) {
    strcpy(arrMqttPos, "26460\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "28\0") == 0) {
    strcpy(arrMqttPos, "27720\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "30\0") == 0) {
    strcpy(arrMqttPos, "29610\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "32\0") == 0) {
    strcpy(arrMqttPos, "31500\0");
    return arrMqttPos;
  }
}

char * getPositionInM(char currMatPosition[]) {
  for (int i = 7; i >= 0; i--) {
    if (currMatPosition[i] == '1') {
      return getM(i);
    }
  }
  return NULL;
}

void checkPosition() {
  digitalWrite(A0, 0);
  digitalWrite(A1, 0);
  digitalWrite(A2, 0);
  if (digitalRead(A3) == 0)
    currMatPosition[0] = '1';
  else
    currMatPosition[0] = '0';

  digitalWrite(A0, 1);
  digitalWrite(A1, 0);
  digitalWrite(A2, 0);
  if (digitalRead(A3) == 0)
    currMatPosition[1] = '1';
  else
    currMatPosition[1] = '0';

  digitalWrite(A0, 0);
  digitalWrite(A1, 1);
  digitalWrite(A2, 0);
  if (digitalRead(A3) == 0)
    currMatPosition[2] = '1';
  else
    currMatPosition[2] = '0';

  digitalWrite(A0, 1);
  digitalWrite(A1, 1);
  digitalWrite(A2, 0);
  if (digitalRead(A3) == 0)
    currMatPosition[3] = '1';
  else
    currMatPosition[3] = '0';

  digitalWrite(A0, 0);
  digitalWrite(A1, 0);
  digitalWrite(A2, 1);
  if (digitalRead(A3) == 0)
    currMatPosition[4] = '1';
  else
    currMatPosition[4] = '0';

  digitalWrite(A0, 1);
  digitalWrite(A1, 0);
  digitalWrite(A2, 1);
  if (digitalRead(A3) == 0)
    currMatPosition[5] = '1';
  else
    currMatPosition[5] = '0';

  digitalWrite(A0, 0);
  digitalWrite(A1, 1);
  digitalWrite(A2, 1);
  if (digitalRead(A3) == 0)
    currMatPosition[6] = '1';
  else
    currMatPosition[6] = '0';

  digitalWrite(A0, 1);
  digitalWrite(A1, 1);
  digitalWrite(A2, 1);
  if (digitalRead(A3) == 0)
    currMatPosition[7] = '1';
  else
    currMatPosition[7] = '0';

  currMatPosition[8] = '\0';

  checkAction();

  //Serial.println(millis() - msgTimer);
  if (strcmp(matPosition, currMatPosition) != 0 && currRfid[0] != '\0' && millis() - msgTimer > 200) //not eq
  {
    msgTimer = millis();
    char posMsg[19];
    memset(posMsg, '\0', sizeof(posMsg));
    strncpy(matPosition, currMatPosition, 9);
    strcat(posMsg, currRfid);
    strcat(posMsg, "|");
    char * currPositionPointer = getPositionInM(currMatPosition);

    if (currPositionPointer != NULL) {
      if (strcmp(currPositionPointer, positionPointer) != 0) {
        char * posMqtt = getPosMqtt(currPositionPointer);
        strncpy(positionPointer, currPositionPointer, 3);
        strcat(posMsg, posMqtt);
        strcat(posMsg, "\0");
        client.publish("/rolling/MatPosition", posMsg);
        free(posMqtt);
      }
    } else {
      strcpy(positionPointer, "ff\0");
      strcat(posMsg, "-");
      strcat(posMsg, "\0");
      client.publish("/rolling/MatPosition", posMsg);
    }
    free(currPositionPointer);
  }
}

char * getM(int i) {
  char * arrSm = (char * ) malloc(sizeof(char) * 3);
  if (i == 0) {
    arrSm[0] = '1';
    arrSm[1] = '8';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 1) {
    arrSm[0] = '2';
    arrSm[1] = '0';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 2) {
    arrSm[0] = '2';
    arrSm[1] = '1';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 3) {
    arrSm[0] = '2';
    arrSm[1] = '3';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 4) {
    arrSm[0] = '2';
    arrSm[1] = '6';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 5) {
    arrSm[0] = '2';
    arrSm[1] = '8';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 6) {
    arrSm[0] = '3';
    arrSm[1] = '0';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 7) {
    arrSm[0] = '3';
    arrSm[1] = '2';
    arrSm[2] = '\0';
    return arrSm;
  }
}

void motor(int speed, char direction) {
  if (direction == 'l') //left
  {
    digitalWrite(motor1_A, 1);
    digitalWrite(motor1_B, 0);
  } else if (direction == 'r') //right
  {
    digitalWrite(motor1_A, 0);
    digitalWrite(motor1_B, 1);
  }
  analogWrite(motor1_Speed, speed); // speed counts from 0 to 255
  delay(20);
  started = '1';
}
void stopMotor() {
  analogWrite(motor1_Speed, 0);
  digitalWrite(motor1_A, 0);
  digitalWrite(motor1_B, 0);
  started = '0';
}

void rolling(int rSpeed, char rDirection) {
  if (rDirection == 'l') //left
  {
    digitalWrite(motor2_A, 1);
    digitalWrite(motor2_B, 0);
  } else if (rDirection == 'r') //right
  {
    digitalWrite(motor2_A, 0);
    digitalWrite(motor2_B, 1);
  }
  analogWrite(motor2_Speed, rSpeed); // speed counts from 0 to 255
  delay(20);
  rStarted = '1';
}

void stopRolling() {
  analogWrite(motor2_Speed, 0);
  digitalWrite(motor2_A, 0);
  digitalWrite(motor2_B, 0);
  rStarted = '0';
}

void writeInfo() {
  char * line = (char * ) malloc(sizeof(char) * 40);

  memset(line, '\0', sizeof(line));
  strcpy(line, "isStarted|"); // copy name into the new var 
  strcat(line, & started); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/rolling/motorInfo", line);

  memset(line, '\0', sizeof(line));
  strcpy(line, "isRollingStarted|"); // copy name into the new var 
  strcat(line, & rStarted); // add the extension 
  line[18] = '\0'; //fix
  client.publish("/rolling/motorInfo", line);

  memset(line, '\0', sizeof(line));
  strcpy(line, "direction|"); // copy name into the new var 
  strcat(line, & direction); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/rolling/motorInfo", line);

  memset(line, '\0', sizeof(line));
  strcpy(line, "rollingDirection|"); // copy name into the new var 
  strcat(line, & rDirection); // add the extension 
  line[18] = '\0'; //fix
  client.publish("/rolling/motorInfo", line);

  //sprintf(speedArr, "%ld", speed);  
  memset(speedArr, '\0', sizeof(speedArr));
  itoa(speed, speedArr, 10); // here 10 means decimal
  memset(line, '\0', sizeof(line));
  strcpy(line, "speed|"); // copy name into the new var 
  strcat(line, & speedArr[0]); // add the extension 
  client.publish("/rolling/motorInfo", line);

  memset(speedArr, '\0', sizeof(speedArr));
  itoa(rSpeed, speedArr, 10); // here 10 means decimal
  memset(line, '\0', sizeof(line));
  strcpy(line, "rollingSpeed|"); // copy name into the new var 
  strcat(line, & speedArr[0]); // add the extension 
  client.publish("/rolling/motorInfo", line);

  free(line);

}