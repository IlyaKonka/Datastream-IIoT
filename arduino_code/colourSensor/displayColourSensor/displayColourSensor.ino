#include "ssd1306.h"
#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);  // I2C / TWI 

const uint8_t psiLit [] PROGMEM {
0x06, 0x06, 0x06, 0x0E, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E,
0x0E, 0x1E, 0x1E, 0x7C, 0x7C, 0xF8, 0xF0, 0xF0, 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x80, 0xE0, 0xF0, 0x38, 0x1C, 0x0C, 0x06, 0x06, 0x07, 0x03, 0x03, 0x03, 0x03, 0x07, 0x07,
0x06, 0x0E, 0x1E, 0x3E, 0x7C, 0xF8, 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x02, 0x06, 0x06, 0x0E, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x0E, 0x06, 0x02, 0x02, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xFF, 0x7F, 0x3F, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x07, 0x1F, 0x3F, 0x7F, 0xFE, 0xFE, 0xFC, 0xF8, 0xF8, 0xF8, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0,
0xC0, 0xC0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
0x06, 0x06, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x03, 0x07, 0x07, 0x07, 0x0F, 0x0F,
0x0F, 0x1F, 0x3F, 0x3F, 0x7F, 0xFE, 0xF8, 0xF8, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8,
0xF0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x80, 0xC0, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x1F, 0x7F, 0x7E, 0xFC, 0xF0, 0xF0, 0xE0, 0xC0, 0xC0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xC0,
0xC0, 0xC0, 0xE0, 0x70, 0x38, 0x1F, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x80, 0xC0, 0xC0, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0xC0, 0xC0, 0x80, 0x00,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00
};

const uint8_t logoLit [] PROGMEM {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0xA0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x10, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02,
0x00, 0x10, 0x00, 0x81, 0x00, 0x00, 0x44, 0x60, 0x00, 0x40, 0x00, 0x44, 0x0C, 0x00, 0x04, 0x40,
0x80, 0x51, 0xF1, 0xF0, 0xE0, 0xC0, 0x61, 0x83, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 0x81,
0x10, 0x10, 0x00, 0x22, 0x02, 0x20, 0x22, 0x00, 0x08, 0x40, 0x04, 0x20, 0x00, 0x80, 0x48, 0x00,
0x00, 0x10, 0x00, 0x20, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xE0, 0x88, 0xC8, 0xC5, 0xE4, 0xC2, 0xF3, 0xE0, 0xFD, 0xFF,
0xDD, 0xF0, 0xFA, 0xD8, 0xD9, 0xF9, 0xD4, 0xC8, 0xE0, 0xF0, 0xF2, 0x90, 0xE1, 0xC1, 0x80, 0xC0,
0x84, 0x81, 0x81, 0x81, 0x41, 0x81, 0x83, 0x82, 0xC2, 0x87, 0x8F, 0x8F, 0x8F, 0x0F, 0xCF, 0x8F,
0x8F, 0x1F, 0x8E, 0x8E, 0x8E, 0x0C, 0x9B, 0x0C, 0x8C, 0x3C, 0x0A, 0x0C, 0x0C, 0x88, 0xC0, 0x82,
0x80, 0x80, 0x0C, 0x84, 0x00, 0x50, 0x02, 0x20, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x21, 0x09, 0x41, 0x63, 0x21, 0x23, 0x03, 0x11, 0x23, 0x05,
0x39, 0xD1, 0xF4, 0xF9, 0xFC, 0xDD, 0xBC, 0xF9, 0xD5, 0x77, 0xBF, 0xC7, 0xCD, 0x97, 0xCF, 0xD3,
0xF7, 0xFB, 0xFB, 0xF3, 0xFB, 0xF7, 0xF7, 0xFF, 0xFF, 0xF7, 0xF7, 0xF7, 0x77, 0xF7, 0xF3, 0xFF,
0xFB, 0xFB, 0xFF, 0xF3, 0x6B, 0xF3, 0xC3, 0xB3, 0x43, 0x59, 0xF3, 0xF9, 0x7B, 0x79, 0xB9, 0x5D,
0x53, 0x19, 0x4B, 0x59, 0x13, 0x85, 0x01, 0x09, 0x21, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x10, 0x10, 0x24, 0x64, 0xAC, 0x00, 0xA4, 0x24, 0x4A, 0x2A,
0xCF, 0x7F, 0x67, 0xDA, 0x7F, 0xB6, 0x37, 0x5F, 0x7F, 0x7F, 0xAF, 0x3F, 0x1F, 0xFF, 0x5F, 0x47,
0xE7, 0xF7, 0xF9, 0xFB, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0x9F, 0xFF,
0xDF, 0xFF, 0xFF, 0xC7, 0xFF, 0xFA, 0xFD, 0xB7, 0x42, 0x93, 0x19, 0xD2, 0xF3, 0x2D, 0xA3, 0x7A,
0x51, 0x1C, 0x98, 0x8B, 0x0B, 0x21, 0x00, 0x09, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x08, 0x1E, 0x18,
0x3E, 0x38, 0x3C, 0x7C, 0xFF, 0x7C, 0xFD, 0x7D, 0xFD, 0xFD, 0xFF, 0xF9, 0xFF, 0xFF, 0xF5, 0xFF,
0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
0x7F, 0x7F, 0x7E, 0x1F, 0x3F, 0x3C, 0x3F, 0x1D, 0x0D, 0x17, 0x0E, 0x0B, 0x0F, 0x03, 0x02, 0x00,
0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00
};

char * receivedRfid = (char * ) malloc(sizeof(char) * 9);
char * receivedMsg = (char * ) malloc(sizeof(char) * 4);
unsigned long startTime = 0;
bool isActionHappened = false;
bool screenSaver = false;

bool isChecked = false;

void setup() {
  memset(receivedMsg, '\0', sizeof(receivedMsg));
  memset(receivedRfid, '\0', sizeof(receivedRfid));
  Serial.begin(115200);
  Serial.setTimeout(100);
  ssd1306_128x64_i2c_init();
  ssd1306_fillScreen(0x00);
  u8g.setFont(u8g_font_ncenB12);
  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on. 
  u8g.firstPage();
  delay(1000);
}

void wait(int time) {
  time /= 100;
  for (int i = 0; i < time; i++) {
    Serial.readBytes(receivedMsg, 4);

    if (receivedMsg[0] != '\0') {
      //warning only 1 msg
      Serial.write("r\0", 2); //received
      Serial.flush();

      if (strcmp(receivedMsg, "rcd\0") == 0) {
        while (1) {
          Serial.readBytes(receivedRfid, 9);
          delay(20);
          if (receivedRfid[0] != '\0') {
            //warning only 1 msg
            Serial.write("r\0", 2); //received
            Serial.flush();
            memset(receivedMsg, '\0', sizeof(receivedMsg));
            while (Serial.available())
              Serial.read();
            id(receivedRfid);
            isActionHappened = true;
            return;
          }
        }
      }

      //warning maybe timer 
      while (Serial.available())
        Serial.read();

      if (strcmp(receivedMsg, "qtc\0") == 0) {
        memset(receivedMsg, '\0', sizeof(receivedMsg));
        qualityCheck(); //...
        isActionHappened = true;
        return;
      } else if (strcmp(receivedMsg, "qif\0") == 0) {
        isChecked = true;
        memset(receivedMsg, '\0', sizeof(receivedMsg));
        qualityCheckIsFinished();
        isActionHappened = true;
        return;
      } else if (strcmp(receivedMsg, "rdy\0") == 0) {
        memset(receivedMsg, '\0', sizeof(receivedMsg));
        isActionHappened = false;
        screenSaver = true;
        return;
      } else {
        qualityRes(receivedMsg);
        isActionHappened = true;
        return;
      }
    }
  }
}

bool stopAnim() {
  if (isChecked) {
    isChecked = false;
    return true;
  } else
    return false;
}

void animation() {
  while (1) {
    for (int i = 48; i < 55; i++) {
      ssd1306_putPixels(i, 49, 0xFF);
    }
    wait(200);
    if (stopAnim()) {
      break;
    }
    for (int i = 60; i < 67; i++) {
      ssd1306_putPixels(i, 49, 0xFF);
    }
    wait(200);
    if (stopAnim()) {
      break;
    }
    for (int i = 72; i < 79; i++) {
      ssd1306_putPixels(i, 49, 0xFF);
    }
    wait(200);
    if (stopAnim()) {
      break;
    }
    ssd1306_clearBlock(48, 49 / 8, 7, 8);
    ssd1306_clearBlock(60, 49 / 8, 7, 8);
    ssd1306_clearBlock(72, 49 / 8, 7, 8);
    wait(200);
    if (stopAnim())
      break;
  }
}

void qualityRes(char res[]) {
  ssd1306_fillScreen(0x00);
  u8g.firstPage();
  u8g.setFont(u8g_font_ncenB14);
  if (res[0] == '0') {
    do {
      u8g.drawStr(18, 40, "No defect"); //14
    } while (u8g.nextPage());
  } else if (res[0] == '1') {
    do {
      u8g.drawStr(32, 25, "Defect"); //14
      u8g.drawStr(27, 50, "type #1");
    } while (u8g.nextPage());
  } else if (res[0] == '2') {

    do {
      u8g.drawStr(32, 25, "Defect"); //14
      u8g.drawStr(27, 50, "type #2");
    } while (u8g.nextPage());
  } else if (res[0] == 'e') {
    u8g.setFont(u8g_font_ncenB12);
    do {
      u8g.drawStr(3, 16, "Problem with"); //12
      u8g.drawStr(5, 36, "quality check.");
      u8g.drawStr(5, 56, "Pls, try again.");
    } while (u8g.nextPage());
  }

  memset(receivedMsg, '\0', sizeof(receivedMsg));
}

void qualityCheck() {
  ssd1306_fillScreen(0x00);
  u8g.setFont(u8g_font_ncenB12);
  u8g.firstPage();
  do {
    u8g.drawStr(5, 30, "Quality check");
  } while (u8g.nextPage());
  animation();
}
void qualityCheckIsFinished() {
  ssd1306_fillScreen(0x00);
  u8g.setFont(u8g_font_ncenB12);
  u8g.firstPage();
  do {
    u8g.drawStr(6, 25, "Quality check"); //12
    u8g.drawStr(20, 50, "is finished");
  } while (u8g.nextPage());
}
void id(char rfidArr[]) {
  ssd1306_fillScreen(0x00);
  u8g.setFont(u8g_font_ncenB12);
  u8g.firstPage();
  do {
    u8g.drawStr(52, 16, "ID"); //12
    u8g.drawStr(23, 36, rfidArr);
    u8g.drawStr(25, 56, "received");
  } while (u8g.nextPage());
  memset(receivedRfid, '\0', sizeof(receivedRfid));
}

void mainCheck() {
  if (isActionHappened) {
    screenSaver = false;
    while (1) {
      isActionHappened = false;
      startTime = millis();
      while (millis() - startTime < 300000 && isActionHappened == false && screenSaver == false)
      //5 min waiting then screensaver
      {
        wait(10000);
      }
      if (!isActionHappened) {
        ssd1306_fillScreen(0x00);
        break;
      }

    }
  }
}

void loop() {
  //Serial.println();
  //Serial.println(freeRam());
  //Serial.println();

  ssd1306_fillScreen(0x00);
  ssd1306_drawBitmap(25, 1, 80, 50, psiLit);
  wait(2000);
  mainCheck();
  ssd1306_fillScreen(0x00);
  ssd1306_drawBitmap(25, 1, 80, 50, logoLit);
  wait(2000);
  mainCheck();
  ssd1306_fillScreen(0x00);
  u8g.setFont(u8g_font_ncenB12);
  u8g.firstPage();
  do {
    u8g.drawStr(17, 25, "Waiting for"); //12
    u8g.drawStr(9, 50, "new material");
  } while (u8g.nextPage());
  wait(2000);
  mainCheck();

}

int freeRam() {
  extern int __heap_start, * __brkval;
  int v;
  return (int) & v - (__brkval == 0 ? (int) & __heap_start : (int) __brkval);
}

//u8g.drawStr( 17, 25, "Waiting for");//12
//u8g.drawStr( 9, 50, "new material");

//u8g.drawStr( 25, 25, "Material");//14
//u8g.drawStr( 23, 50, "detected");

//u8g.drawStr( 8, 16, "Please, move");//12
//u8g.drawStr( 25, 36, "material");
//u8g.drawStr( 30, 56, "right -->");

//u8g.drawStr( 8, 16, "Moving to");//12
//u8g.drawStr( 5, 36, "RFID Reader");
//u8g.drawStr( 33, 56, "<-- left");

//u8g.drawStr( 40, 25, "RFID");//14
//u8g.drawStr( 23, 50, "674516f2");

//u8g.drawStr( 40, 25, "Valid");//14
//u8g.drawStr( 39, 50, "RFID");

//u8g.drawStr( 32, 25, "Invalid");//14
//u8g.drawStr( 39, 50, "RFID");

//u8g.drawStr( 8, 40, "Forwarding");//14

//u8g.drawStr( 20, 30, "Dropping");//14

//u8g.drawStr( 23, 40, "Dropped");//14

//u8g.drawStr( 25, 25, "Position:");//14
//u8g.drawStr( 39, 50, "20 cm");
//u8g.drawStr( 46, 50, "2 cm");

//u8g.drawStr( 18, 13, "Waiting for");//12
//u8g.drawStr( 35, 33, "rolling");

//u8g.drawStr( 22, 25, "Ready to");//14
//u8g.drawStr( 48, 50, "roll");

//u8g.drawStr( 7, 25, "Manual mode");//12
//u8g.drawStr( 47, 50, "OFF");