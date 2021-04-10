#include <SPI.h>

#include <Ethernet.h>

#include <PubSubClient.h>


// Update these with values suitable for your network.
byte mac[] = {
  0x72,
  0xB2,
  0xD0,
  0x0B,
  0x11,
  0x0D
};
//IPAddress ip(192,168,0,111);
//IPAddress server(192,168,0,102);
//IPAddress ip (169,254,166,102);
//IPAddress server (169,254,166,232);

//IPAddress ip (169,254,65,102);
//IPAddress server (169,254,65,51); 

IPAddress ip(192, 168, 0, 13);
IPAddress server(192, 168, 0, 21);

EthernetClient ethClient;
PubSubClient client(ethClient);

char speedArr[4]; // speed counts from 0 to 255
char * receivedMsg = (char * ) malloc(sizeof(char) * 2);
char * positionPointer = (char * ) malloc(sizeof(char) * 3);
char * button = (char * ) malloc(sizeof(char) * 2);

char command = 'n'; //nothing
boolean isManual = false;

//motor
int motor1_A = 2;
int motor1_B = 4;
int motor1_Speed = 3;

//default
int speed = 120;
char direction = 'r';
char started = '0';
bool writeInterval = false;

//vars
unsigned long startTime;
int interval;
char currRfid[9]; //use
char matPosition[8];
char currMatPosition[8];
unsigned long msgTimer = 0;
char quality[2];

bool isFirstOnTheTbl = false;
bool isAfterQCheck = false;
bool isUnloadingTblReady = false;
bool isDisplayProblemSent = false;
bool stopBeforeUnloading = false;
bool isMovingToUnload = false;
bool isCollectData = false;

int dCounter = 0;
int bCounter = 0;
int mCounter = 0;

void setup() {
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

  client.setServer(server, 1883);
  client.setCallback(callback);

  //Motor
  pinMode(motor1_A, OUTPUT);
  pinMode(motor1_B, OUTPUT);

  Ethernet.begin(mac, ip);

  // Allow the hardware to sort itself out
  delay(1500);
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected() && !isManual) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoQualityTable", "futurelab", "FutureLab123")) {
      //if (client.connect("arduinoQualityTable")) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/tables/connection", "arduinoQualityTable|connected");
      // ... and resubscribe

      client.subscribe("/quality/control");
      client.subscribe("/quality/MatRollInfo");
      client.subscribe("/quality/MatCheck");
      client.subscribe("/quality/MatReadyToUnloading");
      client.subscribe("/quality/MatMove");
      client.subscribe("/quality/MatCheckAuto");

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
  if (strcmp(topic, "/quality/control") == 0) {
    bool isSpeed = false;
    for (int i = 0; i < length; i++) {
      //Serial.print((char)p[i]);

      if ((char) p[i] == 'l') //left
      {
        direction = 'l';
        client.publish("/quality/info", "direction|l");
      } else if ((char) p[i] == 'r') //right
      {
        direction = 'r';
        client.publish("/quality/info", "direction|r");
      } else if ((char) p[i] == 's') //start
      {
        motor(speed, direction);
        //started='1';
        client.publish("/quality/info", "isStarted|1");
      } else if ((char) p[i] == 'p') //pause
      {
        stopMotor();
        //started='0';
        client.publish("/quality/info", "isStarted|0");
      } else if ((char) p[i] == 't') //loopTime info
      {
        if (writeInterval == true)
          writeInterval = false;
        else
          writeInterval = true;

        //writeInterval = !writeInterval; //does not work

        if (writeInterval)
          client.publish("/quality/info", "loopTime|1");
        else
          client.publish("/quality/info", "loopTime|0");
      } else if ((char) p[i] == 'i') {
        writeInfo();
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

      if (speed < 100) //min
        speed = 100;
      if (speed > 255) //max
        speed = 255;

      //Serial.println(freeRam());
      //sprintf(speedArr, "%ld", speed); 
      //char* speedArrPointer = &speedArr[0]; 
      memset(speedArr, '\0', sizeof(speedArr));
      itoa(speed, speedArr, 10);
      char speedMsg[7] = "speed|\0";
      char * concSpeed = (char * ) malloc(20);
      memset(concSpeed, '\0', sizeof(concSpeed));
      strncpy(concSpeed, & speedMsg[0], sizeof(speedMsg)); /* copy name into the new var */
      strcat(concSpeed, & speedArr[0]); /* add the extension */
      client.publish("/quality/info", concSpeed);

      free(concSpeed);
    }
    //Serial.println();
  } else if (strcmp(topic, "/quality/MatRollInfo") == 0) {
    char warningLine[30] = "Received id is not correct.\0";
    if (length != 8) {
      if ('c' == (char) p[8]) //collect
      {
        isCollectData = true;
        for (int i = 0; i < 8; i++) {
          currRfid[i] = (char) p[i];
        }
        currRfid[9] = '\0';
      } else
        client.publish("/quality/warning", warningLine);
    }

    if (!isCollectData) {

      for (int i = 0; i < length; i++) {
        currRfid[i] = (char) p[i];
      }
      currRfid[9] = '\0';
      delay(1000);
      waitFeedback("rcd\0", 4);
      delay(500);
      waitFeedback(currRfid, 9);
      delay(1500);
      client.publish("/rolling/MatReadyToQCheck", currRfid);
      isFirstOnTheTbl = true;
      motor(speed, 'r');
      waitFeedback("pfq\0", 4);
      //go to center and wfq... then "/quality/MatCheck" received
    } else {
      waitFeedback("cdt\0", 4);
      isFirstOnTheTbl = true;
      motor(speed, 'r');
    }
  } else if (strcmp(topic, "/quality/MatCheck") == 0) {
    char warningLine[33] = "Received info is not correct.\0";
    char warningLineId[30] = "Received id is not correct.\0";
    bool isWritten = false;
    if (length != 10)
      client.publish("/quality/warning", warningLine);

    for (int i = 0; i < 8; i++) {
      if (currRfid[i] != (char) p[i]) {
        client.publish("/quality/warning", warningLineId);
        break;
      }
    }
    quality[0] = (char) p[9];
    quality[1] = '\0';
    char tmpQ[4];
    if (quality[0] == '0')
      tmpQ[0] = '1';
    else if (quality[0] == '1')
      tmpQ[0] = '0';
    else if (quality[0] == '2')
      tmpQ[0] = '0';

    tmpQ[1] = '0';
    tmpQ[2] = '0';
    tmpQ[3] = '\0';
    waitFeedback(tmpQ, 4);
    delay(4000);
    isAfterQCheck = true;
    //move to the end of the table
  } else if (strcmp(topic, "/quality/MatReadyToUnloading") == 0) {
    char warningLine[49] = "Received id is not correct. Expected: \0";
    bool isWritten = false;
    for (int i = 0; i < length; i++) {
      if ((char) p[i] != currRfid[i]) {
        if (!isWritten) {
          //strcpy(warningLine, "Received id is not correct. Expected: "); // copy name into the new var 
          strcat(warningLine, & currRfid[0]); // add the extension 
          client.publish("/quality/warning", warningLine);
          isWritten = true;
        }
      }
    }
    isUnloadingTblReady = true;
  } else if (strcmp(topic, "/quality/MatMove") == 0) {
    char warningLine[30] = "Received id is not correct.\0";
    if (length != 10)
      client.publish("/quality/warning", warningLine);

    if (!isCollectData) {
      for (int i = 0; i < 8; i++) {
        if (currRfid[i] != (char) p[i]) {
          client.publish("/quality/warning", warningLine);
          break;
        }
      }
    }

    if ((char) p[9] == 's') //to start pos
      movingStart();
    else if ((char) p[9] == 'e') //to end pos
      movingEnd();
  } else if (strcmp(topic, "/quality/MatCheckAuto") == 0) {
    client.publish("/colourSensor/MatReadyToQCheck", currRfid);
  }

  free(p);
}

void movingStart() {
  int beforeSensor = 1; //changeable
  motor(200, 'l');
  while (currMatPosition[beforeSensor] != '1') {
    checkPosition();
  }
  stopMotor();
  client.publish("/colourSensor/StartAutoQCheck", currRfid);
}

void movingEnd() {
  int afterSensor = 4; //changeable
  motor(200, 'r');
  while (currMatPosition[afterSensor] != '1') {
    checkPosition();
  }
  stopMotor();
  if (!isCollectData)
    client.publish("/colourSensor/MatEndQCheck", currRfid);
  else {
    client.publish("/colourSensor/MatEndQCheck", currRfid);
    waitFeedback("res\0", 4);
    waitFeedback("rdy\0", 4);
    isCollectData = false;
    memset(currRfid, '\0', sizeof(currRfid));
  }
}

void waitFeedback(char msg[], int msgSize) {
  long tmr = millis();
  for (int i = 0; i < 1; i++) //only 1 time
  {
    Serial.write(msg, msgSize);
    Serial.flush();
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
      client.publish("/quality/warning", "No display connected");
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
    if (command != 'n' && command != 'o') //command=f,b,s
    {
      command = 'o';
      stopMotor();
    } else //command=n,o
    {
      command = 'f';
      motor(speed, 'r');
    }
  } else if (strcmp(button, "b\0") == 0) {
    if (client.connected())
      client.disconnect();

    isManual = true;
    if (command != 'n' && command != 'o') //command=f,b,s
    {
      command = 'o';
      stopMotor();
    } else //command=n,o
    {
      command = 'b';
      motor(speed, 'l');
    }
  } else if (strcmp(button, "s\0") == 0) {
    if (client.connected())
      client.disconnect();

    isManual = true;
    if (command != 'n' && command != 'o') //command=f,b,s
    {
      command = 'o';
      stopMotor();
    } else //command=n,o
    {

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
      //stopSpecial()
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
  checkButtonInfo();
  //Serial.println(freeRam());
  //server
  if (!client.connected() && !isManual) {
    reconnect();
  }
  client.loop();

  //after qcheck
  if (isAfterQCheck) {
    isAfterQCheck = false;
    waitFeedback("fwd\0", 4);
    delay(1000);
    speed = 100; //changeable (should be default)
    motor(speed, 'r');
    stopBeforeUnloading = true;
  }

  //transport to quality table
  if (isUnloadingTblReady) {
    isUnloadingTblReady = false;
    waitFeedback("rtu\0", 4);
    motor(speed, 'r');
    isMovingToUnload = true;
  }

  //ir sensors
  checkPosition();

  //write info loop time
  if (writeInterval) {
    interval = millis() - startTime;
    char intervalArr[7];
    itoa(interval, intervalArr, 10); // here 10 means decimal
    client.publish("/quality/loopTime", intervalArr);
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
    if (currMatPosition[3] == '1')
      dCounter++;

    if (dCounter == 3) {
      stopMotor();
      dCounter = 0;
      isFirstOnTheTbl = false;
      if (!isCollectData) {
        waitFeedback("wfq\0", 4);
        delay(2000);
        client.publish("/quality/MatReadyToQCheck", currRfid);
      } else
        client.publish("/quality/MatReadyToQCheck", currRfid);
    }
  }

  if (stopBeforeUnloading) {
    if (currMatPosition[6] == '1') {
      bCounter++;
    }

    if (bCounter == 3) {
      stopMotor();
      bCounter = 0;
      stopBeforeUnloading = false;
      waitFeedback("wfu\0", 4);
      delay(1200);
      client.publish("/unloading/MatQualityInfo", currRfid);

    }
  }
  if (isMovingToUnload) {
    if (strstr(currMatPosition, "1") == NULL) {
      mCounter++;
    }

    if (mCounter == 3) {
      stopMotor();
      mCounter = 0;
      isMovingToUnload = false;
      client.publish("/quality/info", "QualityCheck is done.");
      delay(2000);
      waitFeedback("rdy\0", 4);
      memset(currRfid, '\0', sizeof(currRfid));
      delay(1000);
    }
  }

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

  currMatPosition[7] = '\0';

  checkAction();

  //Serial.println(millis() - msgTimer);
  if (strcmp(matPosition, currMatPosition) != 0 && currRfid[0] != '\0' && millis() - msgTimer > 200) //not eq
  {

    msgTimer = millis();
    char posMsg[19];
    memset(posMsg, '\0', sizeof(posMsg));
    strncpy(matPosition, currMatPosition, 8);
    strcat(posMsg, currRfid);
    strcat(posMsg, "|");
    char * currPositionPointer = getPositionInM(currMatPosition);

    if (currPositionPointer != NULL) {
      if (strcmp(currPositionPointer, positionPointer) != 0) {
        char * posMqtt = getPosMqtt(currPositionPointer);
        strncpy(positionPointer, currPositionPointer, 3);
        strcat(posMsg, posMqtt);
        strcat(posMsg, "\0");
        client.publish("/quality/MatPosition", posMsg);
        free(posMqtt);
      }
    } else {
      strcpy(positionPointer, "ff\0");
      strcat(posMsg, "-");
      strcat(posMsg, "\0");
      client.publish("/quality/MatPosition", posMsg);
    }
    free(currPositionPointer);
  }
}

char * getPosMqtt(char * currMatPosition) {
  char * arrMqttPos = (char * ) malloc(sizeof(char) * 10);
  if (strcmp(currMatPosition, "33\0") == 0) {
    strcpy(arrMqttPos, "33390\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "36\0") == 0) {
    strcpy(arrMqttPos, "35910\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "38\0") == 0) {
    strcpy(arrMqttPos, "37800\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "42\0") == 0) {
    strcpy(arrMqttPos, "41580\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "43\0") == 0) {
    strcpy(arrMqttPos, "43470\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "46\0") == 0) {
    strcpy(arrMqttPos, "45990\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "48\0") == 0) {
    strcpy(arrMqttPos, "47880\0");
    return arrMqttPos;
  }
}

char * getM(int i) {
  char * arrSm = (char * ) malloc(sizeof(char) * 3);
  if (i == 0) {
    arrSm[0] = '3';
    arrSm[1] = '3';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 1) {
    arrSm[0] = '3';
    arrSm[1] = '6';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 2) {
    arrSm[0] = '3';
    arrSm[1] = '8';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 3) {
    arrSm[0] = '4';
    arrSm[1] = '2';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 4) {
    arrSm[0] = '4';
    arrSm[1] = '3';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 5) {
    arrSm[0] = '4';
    arrSm[1] = '6';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 6) {
    arrSm[0] = '4';
    arrSm[1] = '8';
    arrSm[2] = '\0';
    return arrSm;
  }

}

char * getPositionInM(char currMatPosition[]) {
  for (int i = 6; i >= 0; i--) {
    if (currMatPosition[i] == '1') {
      return getM(i);
    }
  }
  return NULL;
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

void writeInfo() {
  char * line = (char * ) malloc(sizeof(char) * 40);

  memset(line, '\0', sizeof(line));
  strcpy(line, "isStarted|"); // copy name into the new var 
  strcat(line, & started); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/quality/motorInfo", line);

  memset(line, '\0', sizeof(line));
  strcpy(line, "direction|"); // copy name into the new var 
  strcat(line, & direction); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/quality/motorInfo", line);

  //sprintf(speedArr, "%ld", speed);  
  memset(speedArr, '\0', sizeof(speedArr));
  itoa(speed, speedArr, 10); // here 10 means decimal
  memset(line, '\0', sizeof(line));
  strcpy(line, "speed|"); // copy name into the new var 
  strcat(line, & speedArr[0]); // add the extension 
  client.publish("/quality/motorInfo", line);

  free(line);

}