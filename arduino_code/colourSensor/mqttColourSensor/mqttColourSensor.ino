#include <SPI.h>

#include <Ethernet.h>

#include <PubSubClientColour.h>


#define S0 4
#define S1 5
#define S2 6
#define S3 7
#define sensorOut 8

// Stores frequency read by the photodiodes
int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

// Stores the red. green and blue colors
int redColor = 0;
int greenColor = 0;
int blueColor = 0;

int rCounter = 0;

bool isReadyToCheck = false;
bool isInProcess = false;
bool isDisplayProblemSent = false;
bool isFeedBackRecieved = false;
// Update these with values suitable for your network.
byte mac[] = {
  0xAE,
  0x27,
  0xE4,
  0x5E,
  0x7D,
  0x66
};
//IPAddress ip(192,168,0,112);
//IPAddress server(192,168,0,102);

IPAddress ip(192, 168, 0, 15);
IPAddress server(192, 168, 0, 21);

//IPAddress ip (169,254,166,104);
//IPAddress server (169,254,166,232);
EthernetClient ethClient;
PubSubClientColour client(ethClient);

char * receivedMsg = (char * ) malloc(sizeof(char) * 2);

//vars
char currRfid[9]; //use

void setup() {
  //memset(positionPointer, '\0', sizeof(positionPointer));
  memset(receivedMsg, '\0', sizeof(receivedMsg));

  Serial.begin(115200);
  Serial.setTimeout(100); //communication with nano(display)

  client.setServer(server, 1883);
  client.setCallback(callback);

  // Setting the outputs
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  // Setting frequency scaling to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Setting the sensorOut as an input
  pinMode(sensorOut, INPUT);

  Ethernet.begin(mac, ip);

  // Allow the hardware to sort itself out
  delay(1500);
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    //if (client.connect("arduinoColourSensor")) {
    if (client.connect("arduinoColourSensor", "futurelab", "FutureLab123")) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/tables/connection", "arduinoColourSensor|connected");
      // ... and resubscribe

      client.subscribe("/colourSensor/MatReadyToQCheck");
      client.subscribe("/colourSensor/StartAutoQCheck");
      client.subscribe("/colourSensor/MatEndQCheck");
      client.subscribe("/quality/MatReadyToQCheck");

    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
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
  if (strcmp(topic, "/colourSensor/MatReadyToQCheck") == 0) {
    char warningLine[30] = "Received id is not correct.\0";
    if (length != 8)
      client.publish("/colourSensor/warning", warningLine);

    for (int i = 0; i < length; i++) {
      currRfid[i] = (char) p[i];
    }
    isFeedBackRecieved = true;
    currRfid[9] = '\0';
    delay(1000);
    waitFeedback("rcd\0", 4);
    delay(500);
    waitFeedback(currRfid, 9);
    delay(1000);
    char posMsg[11];
    memset(posMsg, '\0', sizeof(posMsg));
    strcat(posMsg, currRfid);
    strcat(posMsg, "|s\0");
    client.publish("/quality/MatMove", posMsg);
    waitFeedback("qtc\0", 4);
  } else if (strcmp(topic, "/colourSensor/StartAutoQCheck") == 0) {
    char posMsg[11];
    memset(posMsg, '\0', sizeof(posMsg));
    strcat(posMsg, currRfid);
    strcat(posMsg, "|e\0");
    client.publish("/quality/MatMove", posMsg);
    delay(300); //warning delay(4) should be changed too
    isReadyToCheck = true;
  } else if (strcmp(topic, "/colourSensor/MatEndQCheck") == 0) {
    char warningLine[30] = "Received id is not correct.\0";
    if (length != 8)
      client.publish("/colourSensor/warning", warningLine);
    /* impossible to check
     for (int i=0;i<8;i++) {
       if(currRfid[i]!=(char)p[i])
       {
         client.publish("/colourSensor/warning", warningLine);  
         break;
       }
     }
     */
    isInProcess = false;
  } else if (strcmp(topic, "/quality/MatReadyToQCheck") == 0) {
    client.publish("/quality/MatCheckAuto", "CS is ready");
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
      client.publish("/colourSensor/warning", "No display connected");
      waitFeedback(msg, msgSize);
    }
  }
  isDisplayProblemSent = false;

}

void loop() {
  //Serial.println(freeRam());
  //server

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  if (isReadyToCheck) {
    isReadyToCheck = false;
    isInProcess = true;
    checkColour();

    //writeRes(colour);   
  }

  /*
    digitalWrite(S2,LOW);
    digitalWrite(S3,LOW);
    redFrequency = pulseIn(sensorOut, LOW);
    //Serial.println(redFrequency);
    //redColor = map(redFrequency, 100, 140, 255,0);
    delay(4);
    digitalWrite(S2,HIGH);
    digitalWrite(S3,HIGH);
    greenFrequency = pulseIn(sensorOut, LOW);
    //Serial.println(greenFrequency);
    //greenColor = map(greenFrequency, 130, 200, 255, 0);
    delay(4);
    digitalWrite(S2,LOW);
    digitalWrite(S3,HIGH);
    blueFrequency = pulseIn(sensorOut, LOW);
    Serial.println(scaleBlue(blueFrequency));
    //blueColor = map(blueFrequency, 140, 200, 255, 0);
    delay(4);

    //Serial.println();
    delay(500);
    */

}

byte scaleRed(int red) {
  if (red < 100)
    red = 100;
  else if (red > 1300)
    red = 1300;

  return (byte) map(red, 100, 1300, 0, 255);
}

byte scaleGreen(int green) {
  if (green < 190)
    green = 190;
  else if (green > 1300)
    green = 1300;

  return (byte) map(green, 190, 1300, 0, 255);
}

byte scaleBlue(int blue) {
  if (blue < 250)
    blue = 250;
  else if (blue > 1200)
    blue = 1200;

  return (byte) map(blue, 250, 1200, 0, 255);
}

void checkColour() {

  unsigned int arrSize = 400;
  byte arr[arrSize];
  int counter = 8; //rfid at first
  memset(arr, 0, sizeof(arr));
  while (isInProcess) {
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);
    redFrequency = pulseIn(sensorOut, LOW);
    arr[counter] = scaleRed(redFrequency);
    counter++;
    delay(4);
    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);
    greenFrequency = pulseIn(sensorOut, LOW);
    arr[counter] = scaleGreen(greenFrequency);
    counter++;
    delay(4);
    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH);
    blueFrequency = pulseIn(sensorOut, LOW);
    arr[counter] = scaleBlue(blueFrequency);
    counter++;
    delay(4);

    client.loop();

    if (counter == arrSize - 5) {
      break;
    }
  }

  for (int i = 0; i < 8; i++) {
    arr[i] = (byte) currRfid[i];
  }
  client.publish("/colourSensor/MatCheck", arr, arrSize);

  waitFeedback("qif\0", 4); //Quality check is finished (TODO)
  delay(2000);
  waitFeedback("rdy\0", 4);
  isFeedBackRecieved = false;
  memset(currRfid, '\0', sizeof(currRfid));
  delay(1000);
  /*
  unsigned int byteArrSize = 450;
  byte bsize = 450 ;
  byte arrByte[byteArrSize];
  for(int i = 0 ; i < byteArrSize;i++)
  {
      arrByte[i]=1;
  }
  
  
  client.publish("/colourSensor/MatCheck", arrByte, byteArrSize);
  //waitFeedback("qif\0",4);//Quality check is finished (TODO)
  delay(2000);
  waitFeedback("rdy\0",4);
  memset(currRfid, '\0', sizeof(currRfid));
  */

  /*
  int red=0;
  int blue=0;
  int white=0;
  for(int i =0; i<counter;i++)
  {
    if(arr[i]=='r')
      red++;
    else if(arr[i]=='w')
      white++;
    else if(arr[i]=='b')
      blue++;
  }

  if(white>blue && white>red)
  {
    if(red<10)
      return '0';
    else
      return '1';
  }

  if(blue>white && blue>red)
  {
    if(red<10)
      return '0';
    else
      return '1';
  }
  if(red>blue && red>white)
  {
    return '2';
  }
  else
  {
    return 'e';//error
  }

*/
}

/*
void writeRes(char res)
{
  
  char* concMsg = (char*) malloc(20); 
  memset(concMsg, '\0', sizeof(concMsg));
  strncpy(concMsg, &currRfid[0], sizeof(currRfid)); 
  if(res=='0')
  {
    strcat(concMsg, "|0\0"); 
    waitFeedback("000\0",4);
  }
  else if(res=='1')
  {
    strcat(concMsg, "|1\0"); 
    waitFeedback("100\0",4);
  }
  else if(res=='2')
  {
    strcat(concMsg, "|2\0"); 
    waitFeedback("200\0",4);
  }
  else if(res=='e') //error
  {
    client.publish("/colourSensor/warning","Problem with quality check. Try again.");
    waitFeedback("e00\0",4);
    free(concMsg);
    colour='d';//default
    return;
  }
  
  client.publish("/colourSensor/MatCheck", concMsg);
  delay(2000);
  waitFeedback("rdy\0",4);
  free(concMsg);
  colour='d';//default
  memset(currRfid, '\0', sizeof(currRfid));
  
}
*/
int freeRam() {
  extern int __heap_start, * __brkval;
  int v;
  return (int) & v - (__brkval == 0 ? (int) & __heap_start : (int) __brkval);
}