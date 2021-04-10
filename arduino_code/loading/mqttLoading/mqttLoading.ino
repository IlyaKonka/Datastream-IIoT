#include <SPI.h>

#include <Ethernet.h>

#include <PubSubClient.h>

#include <MFRC522.h>

#include <avr/wdt.h>

#define RST_PIN 9 //rfid
#define SS_PIN 8 //rfid

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

// Update these with values suitable for your network.
byte mac[] = {
  0x02,
  0x2B,
  0xF4,
  0x6C,
  0xBD,
  0x59
};
//IPAddress ip(192,168,0,109);
//IPAddress server(192,168,0,102);
//IPAddress ip (169,254,166,100);
//IPAddress server (169,254,166,232);

//IPAddress ip (169,254,65,100);
//IPAddress server (169,254,65,51); 

IPAddress ip(192, 168, 0, 11);
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
int motor_Speed = 3;

//default
int speed = 170;
char direction = 'r';
char started = '0';
bool writeInterval = false;

//vars
unsigned long startTime;
int interval;
char rfid[9];
char currRfid[9];
char matPosition[7];
char currMatPosition[7];
unsigned long msgTimer = 0;
unsigned long detectTimer = -1;
unsigned long dropTimer = 0;

char action = NULL;
bool firstPosInfo = false;
bool isReady = true;
bool isRollingTblReady = false;
bool isLastPos = false;
bool isDropping = false;
bool noDisplayPos = false;

bool isLeftInfoSended = false;
bool isRightInfoSended = false;
bool isSentToDisplay = false;
bool isTblReadySent = false;
bool isDisplayProblemSent = false;

int dCounter = 0;
int fCounter = 0;
int rCounter = 0;
int tCounter = 0;

void callback(char * topic, byte * payload, unsigned int length) {

  // Allocate the correct amount of memory for the payload copy
  byte * p = (byte * ) malloc(length);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);

  if (strcmp(topic, "/loading/control") == 0) {
    bool isSpeed = false;
    for (int i = 0; i < length; i++) {

      if ((char) p[i] == 'l') //left
      {
        direction = 'l';
        motor(speed, direction);
        client.publish("/loading/info", "direction|l");
      } else if ((char) p[i] == 'r') //right
      {
        direction = 'r';
        motor(speed, direction);
        client.publish("/loading/info", "direction|r");
      } else if ((char) p[i] == 's') //start
      {
        motor(speed, direction);
        client.publish("/loading/info", "isStarted|1");
      } else if ((char) p[i] == 'p') //pause
      {
        stopMotor();
        client.publish("/loading/info", "isStarted|0");
      } else if ((char) p[i] == 't') //loopTime info
      {
        if (writeInterval == true)
          writeInterval = false;
        else
          writeInterval = true;

        //writeInterval = !writeInterval; //does not work

        if (writeInterval)
          client.publish("/loading/info", "loopTime|1");
        else
          client.publish("/loading/info", "loopTime|0");
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

      if (speed < 0) //min
        speed = 100;
      if (speed > 255) //max
        speed = 255;

      //Serial.println(freeRam());
      //sprintf(speedArr, "%ld", speed); 
      //char* speedArrPointer = &speedArr[0]; 
      memset(speedArr, '\0', sizeof(speedArr));
      itoa(speed, speedArr, 10);
      char speedMsg[7] = "speed|\0";
      char * concSpeed = (char * ) malloc(strlen(speedMsg) + strlen( & speedArr[0]));
      memset(concSpeed, '\0', sizeof(concSpeed));
      strncpy(concSpeed, & speedMsg[0], sizeof(speedMsg)); /* copy name into the new var */
      strcat(concSpeed, & speedArr[0]); /* add the extension */
      client.publish("/loading/info", concSpeed);
      //Serial.println(freeRam());
      free(concSpeed);
      //Serial.println(freeRam());
      //Serial.println();
    }

  } else if (strcmp(topic, "/loading/MatProcess") == 0) {
    char warningLine[49] = "Received id is not correct. Expected: \0";
    bool isWritten = false;
    for (int i = 0; i < length; i++) {
      if ((char) p[i] != '|') {
        if ((char) p[i] != currRfid[i]) {
          if (!isWritten) {
            strcat(warningLine, & currRfid[0]); // add the extension 
            client.publish("/loading/warning", warningLine);
            isWritten = true;
          }

        }
      } else {
        action = (char) p[i + 1];
        char actionMsg[9] = "action|";
        actionMsg[7] = action;
        actionMsg[8] = '\0';
        client.publish("/loading/info", actionMsg);
        if (action == 'F') {
          delay(1000);
          waitFeedback("vid\0", 4);
          delay(1000);
        } else if (action == 'D') {
          delay(1000);
          waitFeedback("iid\0", 4);
          delay(1000);
        }
        break;
      }
    }
  } else if (strcmp(topic, "/loading/MatReadyToRoll") == 0) {
    char warningLine[49] = "Received id is not correct. Expected: \0";
    bool isWritten = false;
    for (int i = 0; i < length; i++) {
      if ((char) p[i] != currRfid[i]) {
        if (!isWritten) {
          //strcpy(warningLine, "Received id is not correct. Expected: "); // copy name into the new var 
          strcat(warningLine, & currRfid[0]); // add the extension 
          client.publish("/loading/warning", warningLine);
          isWritten = true;
        }
      }
    }
    isRollingTblReady = true;
  }

  free(p);
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
      client.publish("/loading/warning", "No display connected");
      waitFeedback(msg, msgSize);
    }
  }
  isDisplayProblemSent = false;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected() && !isManual) {
    //Serial.println("Attempting MQTT connection...");
    // Attempt to connect

    if (client.connect("arduinoLoadingTable", "futurelab", "FutureLab123")) {
      //if (client.connect("arduinoLoadingTable")) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/tables/connection", "arduinoLoadingTable|connected");
      // ... and resubscribe

      client.subscribe("/loading/control");
      client.subscribe("/loading/MatProcess");
      client.subscribe("/loading/MatReadyToRoll");

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

void setup() {
  //memset(positionPointer, '\0', sizeof(positionPointer));

  strcpy(positionPointer, "ff\0");
  memset(receivedMsg, '\0', sizeof(receivedMsg));
  memset(currRfid, '\0', sizeof(currRfid));
  memset(rfid, '\0', sizeof(rfid));
  memset(button, '\0', sizeof(button));
  Serial.begin(115200);
  Serial.setTimeout(100); //communication with nano(display)
  SPI.begin(); // инициализация SPI

  //manual control
  //pinMode(6, INPUT); //right
  //pinMode(7,INPUT);  //left

  //ir sensors
  pinMode(A0, OUTPUT); //1
  pinMode(A1, OUTPUT); //2
  pinMode(A2, OUTPUT); //3
  pinMode(A3, INPUT); //4

  //pinMode(A4,OUTPUT);//5
  //pinMode(A5,INPUT);//6

  //attachPinChangeInterrupt(B_PIN, checkButtons, RISING );

  //RFID Reader
  mfrc522.PCD_Init();
  mfrc522.PCD_WriteRegister(mfrc522.RFCfgReg, (0x07 << 4));
  mfrc522.PCD_AntennaOn();
  client.setServer(server, 1883);
  client.setCallback(callback);

  //Motor
  pinMode(motor1_A, OUTPUT);
  pinMode(motor1_B, OUTPUT);

  Ethernet.begin(mac, ip);

  // Allow the hardware to sort itself out
  delay(1500);
}

/*
void checkButtons()
{
  if(digitalRead(A5)==1)
  {
    digitalWrite(A4,1);
    //isManual=true;  
  }
}
*/

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
  //wdt_reset();
  startTime = millis();

  checkButtonInfo();

  //server
  if (!client.connected() && !isManual) {
    reconnect();
  }

  client.loop();

  if (dropTimer != 0 && millis() - dropTimer > 3000) {
    wdt_enable(WDTO_15MS);
  }
  //main action (sorting)
  if (action == 'F' && started == '0') {
    waitFeedback("frw\0", 4);
    delay(1000);
    motor(speed, 'r');
  } else if (action == 'D' && started == '0') {
    waitFeedback("dpg\0", 4);
    delay(500);
    motor(speed, 'l');
    dropTimer = millis();
    //delay(5000);
    isDropping = true;
    isLastPos = true;
    while ((millis() - dropTimer) < 3000) {
      checkPosition();
      delay(50);
    }
    dropTimer = 0;
    isDropping = false;
  }

  //table ready msgs
  if (isReady)
    rCounter++;

  if (rCounter == 20) {
    client.publish("/loading/ReadyToNewMat", "ready");
    rCounter = 0;
  }

  //transport to rolling table
  if (isRollingTblReady) {
    isSentToDisplay = false;
    if (!isTblReadySent) {
      isLastPos = true;
      isTblReadySent = true;
      motor(speed, 'r');
      waitFeedback("rtr\0", 4);
    }
  }

  //rfid
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    dumpByteArray(mfrc522.uid.uidByte, mfrc522.uid.size);
  }

  //ir sensors
  checkPosition();

  //write info loop time
  if (writeInterval) {
    interval = millis() - startTime;
    char intervalArr[7];
    itoa(interval, intervalArr, 10); // here 10 means decimal
    client.publish("/loading/loopTime", intervalArr);
    delay(100);
  }

}

int freeRam() {
  extern int __heap_start, * __brkval;
  int v;
  return (int) & v - (__brkval == 0 ? (int) & __heap_start : (int) __brkval);
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

  if (!isDropping)
    checkAction();

  bool cond1 = ((currMatPosition[0] == '1' || currMatPosition[1] == '1') &&
    currMatPosition[2] == '0' && currMatPosition[3] == '0' && currMatPosition[4] == '0' &&
    currMatPosition[5] == '0');

  if (action == NULL && currRfid[0] == '\0' && !isManual) {
    if (detectTimer == -1 && millis() - detectTimer > 1000) {
      detectTimer = millis();
    } else {
      if (millis() - detectTimer > 500) {
        detectTimer = -1;
        if (cond1 && !isRightInfoSended) {
          client.publish("/loading/rfidReadProblem", "Moving material right.");
          waitFeedback("mvr\0", 4);
          isLeftInfoSended = true;
          motor(speed, 'r');
          dropTimer = millis();
        } else if (!isLeftInfoSended && strstr(currMatPosition, "1") != NULL && !cond1) {
          client.publish("/loading/rfidReadProblem", "Moving material left.");
          waitFeedback("mvl\0", 4);
          isLeftInfoSended = true;
          motor(speed, 'l');
        }
      }
    }
  }

  //Serial.println(millis() - msgTimer);
  if ((strcmp(matPosition, currMatPosition) != 0 && currRfid[0] != '\0' && millis() - msgTimer > 200) || firstPosInfo) //not eq
  {

    msgTimer = millis();
    char posMsg[30];
    memset(posMsg, '\0', sizeof(posMsg));
    strncpy(matPosition, currMatPosition, 7);
    strcat(posMsg, & currRfid[0]);
    strcat(posMsg, "|");
    char * currPositionPointer = getPositionInM(currMatPosition);

    if (currPositionPointer != NULL) {
      if (strcmp(currPositionPointer, positionPointer) != 0) {
        char * posMqtt = getPosMqtt(currPositionPointer);
        strncpy(positionPointer, currPositionPointer, 3);
        strcat(posMsg, posMqtt);
        strcat(posMsg, "\0");
        client.publish("/loading/MatPosition", posMsg);
        free(posMqtt);
        char tmpPos[4];
        if (currPositionPointer[1] == '\0') //x
        {
          tmpPos[0] = '0';
          tmpPos[1] = '0';
          tmpPos[2] = currPositionPointer[0];
          tmpPos[3] = '\0';
        } else if (currPositionPointer[2] == '\0') //xx
        {
          tmpPos[0] = '0';
          tmpPos[1] = currPositionPointer[0];
          tmpPos[2] = currPositionPointer[1];
          tmpPos[3] = '\0';
        }
        if (action != 'D' && !noDisplayPos && (tmpPos[1] != '1' || tmpPos[2] != '0')) {
          waitFeedback(tmpPos, 4);
        }
        isLeftInfoSended = false;
        isRightInfoSended = false;
      }
    } else if (!firstPosInfo && !isLastPos) {
      strcpy(positionPointer, "ff\0");
      strcat(posMsg, "-");
      strcat(posMsg, "\0");
      client.publish("/loading/MatPosition", posMsg);
      char none[4];
      none[0] = '9';
      none[1] = '9';
      none[2] = '9';
      none[3] = '\0';
      if (action != 'D') {
        waitFeedback(none, 4);
      }
      isLeftInfoSended = false;
      isRightInfoSended = false;
    }
    firstPosInfo = false;
    free(currPositionPointer);

  }
}

void dumpByteArray(byte * buffer, byte bufferSize) {
  //Note there needs to be 1 extra space for this to work as snprintf null terminates.
  char * myPtr = & currRfid[0]; //or just myPtr=charArr; but the former described it better.

  for (byte i = 0; i < bufferSize; i++) {
    snprintf(myPtr, 3, "%02x", buffer[i]); //convert a byte to character string, and save 2 characters (+null) to charArr;
    myPtr += 2; //increment the pointer by two characters in charArr so that next time the null from the previous go is overwritten.
  }

  if (strcmp(rfid, currRfid) != 0) //not eq
  {
    stopMotor();
    strncpy(rfid, currRfid, 9);
    client.publish("/loading/MatLoaded", currRfid);
    char tempPos[20];
    memset(tempPos, '\0', sizeof(tempPos));
    strcpy(tempPos, & currRfid[0]);
    strcat(tempPos, "|7560\0");
    client.publish("/loading/MatPosition", tempPos);
    waitFeedback("rid\0", 4);
    delay(500);
    waitFeedback(currRfid, 9);
    firstPosInfo = true;
    isReady = false;
  }

}

char * getPosMqtt(char * currMatPosition) {
  char * arrMqttPos = (char * ) malloc(sizeof(char) * 10);
  if (strcmp(currMatPosition, "4\0") == 0) {
    strcpy(arrMqttPos, "3780\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "5\0") == 0) {
    strcpy(arrMqttPos, "5040\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "10\0") == 0) {
    strcpy(arrMqttPos, "10080\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "11\0") == 0) {
    strcpy(arrMqttPos, "11340\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "13\0") == 0) {
    strcpy(arrMqttPos, "13230\0");
    return arrMqttPos;
  } else if (strcmp(currMatPosition, "16\0") == 0) {
    strcpy(arrMqttPos, "15750\0");
    return arrMqttPos;
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

char * getM(int i) {
  char * arrSm = (char * ) malloc(sizeof(char) * 3);
  if (i == 0) {
    arrSm[0] = '4';
    arrSm[1] = '\0';
    return arrSm;
  } else if (i == 1) {
    arrSm[0] = '5';
    arrSm[1] = '\0';
    return arrSm;
  } else if (i == 2) {
    arrSm[0] = '1';
    arrSm[1] = '0';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 3) {
    arrSm[0] = '1';
    arrSm[1] = '1';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 4) {
    arrSm[0] = '1';
    arrSm[1] = '3';
    arrSm[2] = '\0';
    return arrSm;
  } else if (i == 5) {
    arrSm[0] = '1';
    arrSm[1] = '6';
    arrSm[2] = '\0';
    return arrSm;
  }

}
void checkAction() {
  if (action == 'D') {
    if (strstr(currMatPosition, "1") == NULL)
      dCounter++;

    if (dCounter == 3) {
      stopMotor();
      dCounter = 0;
      action = NULL;
      isReady = true;
      isLastPos = false;
      client.publish("/loading/info", "Action D is done.");
      waitFeedback("dpd\0", 4);
      delay(2000);
      client.publish("/loading/MatDropped", currRfid);
      waitFeedback("rdy\0", 4);
      delay(1000);
      memset(currRfid, '\0', sizeof(currRfid));
      memset(rfid, '\0', sizeof(rfid));
    }
  } else if (action == 'F') {
    if (currMatPosition[5] == '1')
      fCounter++;

    if (fCounter == 3) {
      stopMotor();
      fCounter = 0;
      action = 'W'; //waiting for rolling table
      waitFeedback("wfr\0", 4);
      delay(1200);
      client.publish("/rolling/MatLoadingInfo", currRfid);
      noDisplayPos = true;
    }
  }

  if (isRollingTblReady) {
    if (strstr(currMatPosition, "1") == NULL)
      tCounter++;

    if (tCounter == 3) {
      stopMotor();
      tCounter = 0;
      isRollingTblReady = false;
      isTblReadySent = false;
      isReady = true;
      isLastPos = false;
      memset(currRfid, '\0', sizeof(currRfid));
      memset(rfid, '\0', sizeof(rfid));
      noDisplayPos = false;
      client.publish("/loading/info", "Action F is done.");
      action = NULL;
      delay(2000);
      waitFeedback("rdy\0", 4);
      delay(1000);
    }
  }
}

void writeInfo() {
  char * line = (char * ) malloc(sizeof(char) * 16);

  memset(line, '\0', sizeof(line));
  strcpy(line, "isStarted|"); // copy name into the new var 
  strcat(line, & started); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/loading/motorInfo", line);

  memset(line, '\0', sizeof(line));
  strcpy(line, "direction|"); // copy name into the new var 
  strcat(line, & direction); // add the extension 
  line[11] = '\0'; //fix
  client.publish("/loading/motorInfo", line);

  //sprintf(speedArr, "%ld", speed);  
  memset(speedArr, '\0', sizeof(speedArr));
  itoa(speed, speedArr, 10); // here 10 means decimal
  memset(line, '\0', sizeof(line));
  strcpy(line, "speed|"); // copy name into the new var 
  strcat(line, & speedArr[0]); // add the extension 
  client.publish("/loading/motorInfo", line);

  free(line);

}

void stopMotor() {
  analogWrite(motor_Speed, 0);
  digitalWrite(motor1_A, 0);
  digitalWrite(motor1_B, 0);
  started = '0';
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
  analogWrite(motor_Speed, speed); // speed counts from 0 to 255
  delay(20);
  started = '1';
}