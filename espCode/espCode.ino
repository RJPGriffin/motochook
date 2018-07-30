/**
 * Display:
 *
 * Following hardware connections are assumed
 *
 *   OLED                   Arduino 101
 *   ---------------------+------------------------------------------------------------------
 *   #1 Vss/GND             GND
 *   #2 Vdd                 3V3 (up to 271 mA, use external power supply to feed Arduino 101)
 *   #4 D/!C                D9
 *   #7 SCLK                D13 (hardware SPI SCLK)
 *   #8 SDIN                D11 (hardware SPI MOSI)
 *   #16 !RESET             !RESET
 *   #17 !CS                D10
 *   #5,#6,#10-14,#19,#20   GND
 *   #3,#9,#15,#18          not connected
 *
 * Based on Adafruit SSD1306 driver (https://github.com/adafruit/Adafruit_SSD1306)
 */

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <ESP8266_SSD1322.h>

//ESP8266 Pins
#define OLED_CS     15  // D8, CS - Chip select
#define OLED_DC     2   // D4 - DC digital signal
#define OLED_RESET  16  // D0 -RESET digital signal

//hardware SPI - only way to go. Can get 110 FPS
ESP8266_SSD1322 display(OLED_DC, OLED_RESET, OLED_CS);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1322_LCDHEIGHT != 64)
#error ("Height incorrect, please fix ESP8266_SSD1322.h!");
#endif

//SD Card Variables
const int SD_CS = 3; // D3
int cardWorking = 0;

//Serial Variables
char inBuff[5] = {};
struct data {
        float coolantTemp = 0;
        float tps = 0;
        float lambda = 0;
        float oilPressure = 0;
        float oilTemp = 0;
        int motorRPM = 0;
};

void setup()   {
        pinMode(OLED_CS, OUTPUT);
        pinMode(SD_CS, OUTPUT);
        Serial.begin(9600);
        displaySetup();
        sdSetup();

}


void loop() {

}

void sdSetup(){
        digitalWrite(SD_CS, HIGH);
        Serial.print("Initializing SD card...");

        // This is for a non standard version of the library.
        int sdInit = SD.begin(SD_CS, SPI_HALF_SPEED);
        // see if the card is present and can be initialized:
        if (sdInit != 111) {
                Serial.print("SD INIT = ");
                Serial.println(sdInit);
                Serial.println("Card failed, or not present");
                cardWorking = 0;
                // TODO Write error to display
                return;
        }
        cardWorking = 1;
        Serial.println("card initialized.");
}

void receiveSerial(){
        if(Serial.available()) {
                buffPush(Serial.read());
                if(inBuff[4] == '}') { // Latest char is the end of a packet
                        if(inBuff[0] =='{') { // First char of packet is in expected location - we should have data between them
                                float value = dataDecode(inbuff[2],inbuff[3]);
                                assignValue(inBuff[1], value);
                        }
                }
        }
}

void buffPush(char in){
        //hard coded for buffer length of 5
        for(int i = 0; i < 4; i++) { //leave last value untouched, hence -1
                //Shifts the array left
                inBuff[i] = inBuff[i+1];
        }
        //Add new value to end
        inBuff[inBuff .4] = in;
}

float dataDecode(char b1, char b2){
        float val = 0;
        //First calculate the value in b1 and b2
        //Check for 0
        if( b1 == 0xFF && b2 = 0xFF) {
                val = 0;
        } else if (b1 && 0x10000000) { // check flag for integer
                val = ((int)b1 - 128)*100 + (int)b2;
                Serial.print("integer value decoded as: ");
                Serial.println(val);
        } else{ //value is a float
                val = (float)((int)b1 + ((int)b2/100));
                Serial.print("Float value decoded as: ");
                Serial.println(val);
        }

        return val;
}

void assignValue(char id, float val){
        switch (id) {
        case 'v':
                data.coolantTemp = val;
                break;
        case 't':
                data.tps = val;
                break;
        case 'w':
                data.lambda = val;
                break;
        case 'i':
                data.oilPressure = val;
                break;
        case 'm':
                data.motorRPM = val;
                break;
        case 'o':
                data.oilTemp = val;
                break;
        default:
                break;
        }
}



void displaySetup(){
        // Initialize and perform reset
        display.begin(true);
        // init done

        // Show image buffer on the display hardware.
        // Since the buffer is intialized with an Adafruit splashscreen
        // internally, this will display the splashscreen.
        // TODO Splashscreen here
        // display.display();

        // Clear the buffer.
        display.clearDisplay();

        display.setTextColor(WHITE);


        display.drawRightString("Size 2",0,0,2);
        display.drawRightString("Size 4",0,10,4);
        display.drawRightString("Size 6",0,20,6);

        display.display();
        delay(2000);
}


void testdrawchar(void) {
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,0);

        for (uint8_t i=0; i < 168; i++) {
                if (i == '\n') continue;
                display.write(i);
                if ((i > 0) && (i % 21 == 0))
                        display.println();
        }
        display.display();
}


void testdrawrect(void) {
        for (int16_t i=0; i<display.height()/2; i+=2) {
                display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
                display.display();
        }
}

void testdrawline() {
        for (int16_t i=0; i<display.width(); i+=4) {
                display.drawLine(0, 0, i, display.height()-1, WHITE);
                display.display();
        }
        for (int16_t i=0; i<display.height(); i+=4) {
                display.drawLine(0, 0, display.width()-1, i, WHITE);
                display.display();
        }
        delay(250);

        display.clearDisplay();
        for (int16_t i=0; i<display.width(); i+=4) {
                display.drawLine(0, display.height()-1, i, 0, WHITE);
                display.display();
        }
        for (int16_t i=display.height()-1; i>=0; i-=4) {
                display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
                display.display();
        }
        delay(250);

        display.clearDisplay();
        for (int16_t i=display.width()-1; i>=0; i-=4) {
                display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
                display.display();
        }
        for (int16_t i=display.height()-1; i>=0; i-=4) {
                display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
                display.display();
        }
        delay(250);

        display.clearDisplay();
        for (int16_t i=0; i<display.height(); i+=4) {
                display.drawLine(display.width()-1, 0, 0, i, WHITE);
                display.display();
        }
        for (int16_t i=0; i<display.width(); i+=4) {
                display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE);
                display.display();
        }
        delay(250);
}

void testscrolltext(void) {
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(10,0);
        display.clearDisplay();
        display.println("scroll");
        display.display();

        display.startscrollright(0x00, 0x0F);
        delay(2000);
        display.stopscroll();
        delay(1000);
        display.startscrollleft(0x00, 0x0F);
        delay(2000);
        display.stopscroll();
        delay(1000);
        display.startscrolldiagright(0x00, 0x07);
        delay(2000);
        display.startscrolldiagleft(0x00, 0x07);
        delay(2000);
        display.stopscroll();
}
