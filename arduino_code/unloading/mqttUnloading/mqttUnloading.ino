#include <SPI.h>

#include <Ethernet.h>

#include <PubSubClient.h>


// Update these with values suitable for your network.
byte mac[] = {
  0x9E,
  0xC8,
  0xDC,
  0x0F,
  0xE6,
  0x3D
};
//IPAddress ip(192,168,0,113);
//IPAddress server(192,168,0,102);
//IPAddress ip (169,254,166,103);
//IPAddress server (169,254,166,232);

IPAddress ip(192, 168, 0, 14);
IPAddress server(192, 168, 0, 21);

//IPAddress ip (169,254,65,103);
//IPAddress server (169,254,65,51); 

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

int motor2_A = 7;
int motor2_B = 6;
int motor2_Speed = 9;

//default
int speed = 200;
int turnOutSpeed = 170;
char turnedOut = '0';
char direction = 'r';
char started = '0';
bool writeInterval = false;

//vars
unsigned long startTime;
int interval;
char currRfid[9]; //use
char matPosition[7];
char currMatPosition[7];
unsigned long msgTimer = 0;
char quality[2];
char action = NULL;

int dCounter = 0;
int mCounter = 0;

bool isFirstOnTheTbl = false;
bool isDisplayProblemSent = false;
bool isForwarding = false;
bool isFinished = false;

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
    if (client.connect("arduinoUnloadingTable", "futurelab", "FutureLab123")) {
      //if (client.connect("arduinoUnloadingTable")) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/tables/connection", "arduinoUnloadingTable|connected");
      // ... and resubscribe

      client.subscribe("/unloading/control");
      client.subscribe("/unloading/MatQualityInfo");
      client.subscribe("/unloading/MatDrop");

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
  if (strcmp(topic, "/unloading/control") == 0) {
    bool isSpeed = false;
    for (int i = 0; i < length; i++) {
      //Serial.print((char)p[i]);

      if ((char) p[i] == 'l') //left
      {
        direction = 'l';
        client.publish("/unloading/info", "direction|l");
      } else if ((char) p[i] == 'r') //right
      {
        direction = 'r';
        client.publish("/unloading/info", "direction|r");
      } else if ((char) p[i] == 's') //start
      {
        motor(speed, direction);
        //started='1';
        client.publish("/unloading/info", "isStarted|1");
      } else if ((char) p[i] == 'p') //pause
      {
        stopMotor();
        //started='0';
        client.publish("/unloading/info", "isStarted|0");
      } else if ((char) p[i] == 't') //loopTime info
      {
        if (writeInterval == true)
          writeInterval = false;
        else
          writeInterval = true;

        //writeInterval = !writeInterval; //does not work

        if (writeInterval)
          client.publish("/unloading/info", "loopTime|1");
        else
          client.publish("/unloading/info", "loopTime|0");
      } else if ((char) p[i] == 'i') {
        writeInfo();
      } else if ((char) p[i] == 'f') {
        client.publish("/unloading/info", "TurnedOut|1");
        turnOut('f'); //forward
      } else if ((char) p[i] == 'b') {
        client.publish("/unloading/info", "TurnedOut|0");
        turnOut('b'); //back
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
      client.publish("/unloading/info", concSpeed);

      free(concSpeed);
    }
    //Serial.println();
  } else if (strcmp(topic, "/unloading/MatQualityInfo") == 0) {
    char warningLine[30] = "Received id is not correct.\0";
    if (length != 8)
      client.publish("/unloading/warning", warningLine);

    for (int i = 0; i < length; i++) {
      currRfid[i] = (char) p[i];
    }

    currRfid[9] = '\0';
    delay(1000);
    waitFeedback("rcd\0", 4);
    delay(500);
    waitFeedback(currRfid, 9);
    delay(1500);
    client.publish("/quality/MatReadyToUnloading", currRfid);
    isFirstOnTheTbl = true;
    motor(speed, 'r');
    waitFeedback("pfu\0", 4);
    //go to turn out
  } else if (strcmp(topic, "/unloading/MatDrop") == 0) {
    char warningLine[33] = "Received info is not correct.\0";
    char warningLineId[30] = "Received id is not correct.\0";
    bool isWritten = false;
    if (length != 10)
      client.publish("/unloading/warning", warningLine);

    for (int i = 0; i < 8; i++) {
      if (currRfid[i] != (char) p[i]) {
        client.publish("/unloading/warning", warningLineId);
        break;
      }
    }
    quality[0] = (char) p[9];
    quality[1] = '\0';
    char tmpQ[4];
    tmpQ[0] = quality[0];
    tmpQ[1] = '0';
    tmpQ[2] = '0';
    tmpQ[3] = '\0';
    delay(1000);
    waitFeedback(tmpQ, 4);
    delay(2000);
    if (quality[0] == '0')
      action = 'F'; //forwarding
    else if (quality[0] == '1')
      action = 'T'; //turning out
    else if (quality[0] == '2')
      action = 'T'; //turning out
  }

  free(p);
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
      client.publish("/unloading/warning", "No display connected");
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
    command = 'o';
    turnOut('f');
    turnOut('b');

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

  if (action != NULL) {
    if (action == 'F') {
      waitFeedback("fwd\0", 4);
      delay(2000);
      motor(speed, 'r');
      isForwarding = true;
      action = NULL;
    } else if (action == 'T') {
      waitFeedback("tro\0", 4);
      delay(2000);
      turnOut('f');
      turnOut('b');
      isFinished = true;
      action = NULL;
    }
  }

  //ir sensors
  checkPosition();

  //write info loop time
  if (writeInterval) {
    interval = millis() - startTime;
    char intervalArr[7];
    itoa(interval, intervalArr, 10); // here 10 means decimal
    client.publish("/unloading/loopTime", intervalArr);
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
    if (currMatPosition[2] == '1')
      dCounter++;

    if (dCounter == 3) {
      stopMotor();
      dCounter = 0;
      isFirstOnTheTbl = false;
      client.publish("/unloading/MatQuery", currRfid);
      //waitFeedback("wfq\0",4);
    }
  }

  if (isFinished) {
    isFinished = false;
    client.publish("/unloading/info", "Unloading is done.");
    client.publish("/unloading/MatFinal", currRfid);
    memset(currRfid, '\0', sizeof(currRfid));
    waitFeedback("fnd\0", 4);
    delay(2000);
    waitFeedback("rdy\0", 4);
    delay(1000);
  }
  if (isForwarding) {
    if (strstr(currMatPosition, "1") == NULL) {
      mCounter++;
    }

    if (mCounter == 3) {
      delay(700);
      stopMotor();
      mCounter = 0;
      isForwarding = false;
      isFinished = true;
    }
  }
}

char * getPositionInM(char currMatPosition[]) {
  for (int i = 5; i >= 0; i--) {
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

  currMatPosition[6] = '\0';

  checkAction();

  //Serial.println(millis() - msgTimer);
  if (strcmp(matPosition, currMatPosition) != 0 && currRfid[0] != '\0' && millis() - msgTimer > 200) //not eq
  {

    msgTimer = millis();
    char posMsg[19];
    memset(posMsg, '\0', sizeof(posMsg));
    strncpy(matPosition, currMatPosition, 7);
    strcat(posMsg, currRfid);
    strcat(posMsg, "|");
    char * currPositionPointer = getPositionInM(currMatPosition);

    if (currPositionPointer != NULL) {
      if (strcmp(currPositionPointer, positionPointer) != 0) {
        char * posMqtt = getPosMqtt(currPositionPointer);
        strncpy(positionPointer, currPositionPointer, 3);
        strcat(posMsg, posMqtt);
        strcat(posMsg, "\0");
        client.publish("/unloading/MatPosition", posMsg);
        free(posMqtt);
      }
    } else {
      strcpy(positionPointer, "ff\0");
      strcat(posMsg, "-");
      strcat(posMsg, "\0");
      client.publish("/unloading/MatPosition", posMsg);
    }
    free(currPositionPointer);
  }
}

char * getM(int i) {
  char * arrSm = (char * ) malloc(sizeof(char) * 3);
  if (i == 0) {
    arrSm[0] = '5';
    arrSm[1] = '0';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 1) {
    arrSm[0] = '5';
    arrSm[1] = '2';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 2) {
    arrSm[0] = '5';
    arrSm[1] = '4';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 3) {
    arrSm[0] = '5';
    arrSm[1] = '7';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 4) {
    arrSm[0] = '5';
    arrSm[1] = '9';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 5) {
    arrSm[0] = '6';
    arrSm[1] = '0';
    arrSm[2] = '\0';
    return arrSm;
  }
}

char * getPosMqtt(char * currMatPosition) {
  char * arrMqttPos = (char * ) malloc(sizeof(char) * 10);
  if (strcmp(currMatPosition, "50\0") == 0) {
    strcpy(arrMqttPos, "50400\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "52\0") == 0) {
    strcpy(arrMqttPos, "52290\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "54\0") == 0) {
    strcpy(arrMqttPos, "54180\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "57\0") == 0) {
    strcpy(arrMqttPos, "56700\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "59\0") == 0) {
    strcpy(arrMqttPos, "58590\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "60\0") == 0) {
    strcpy(arrMqttPos, "60480\0");
    return arrMqttPos;
  }
}

void turnOut(char dir) {
  if (dir == 'f') //forward
  {
    digitalWrite(motor2_A, 0);
    digitalWrite(motor2_B, 1);
    turnedOut = '1';
  } else if (dir == 'b') //back
  {
    digitalWrite(motor2_A, 1);
    digitalWrite(motor2_B, 0);
    turnedOut = '0';
  }
  analogWrite(motor2_Speed, turnOutSpeed); // speed counts from 0 to 255
  delay(1000);

  analogWrite(motor2_Speed, 0);
  digitalWrite(motor2_A, 0);
  digitalWrite(motor2_B, 0);
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
  client.publish("/unloading/motorInfo", line);

  memset(line, '\0', sizeof(line));
  strcpy(line, "direction|"); // copy name into the new var 
  strcat(line, & direction); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/unloading/motorInfo", line);

  //sprintf(speedArr, "%ld", speed);  
  memset(speedArr, '\0', sizeof(speedArr));
  itoa(speed, speedArr, 10); // here 10 means decimal
  memset(line, '\0', sizeof(line));
  strcpy(line, "speed|"); // copy name into the new var 
  strcat(line, & speedArr[0]); // add the extension 
  client.publish("/unloading/motorInfo", line);

  memset(line, '\0', sizeof(line));
  strcpy(line, "TurnedOut|"); // copy name into the new var 
  strcat(line, & turnedOut); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/unloading/motorInfo", line);

  free(line);

}