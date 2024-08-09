// Including libraries for OLED display and Radio module
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_ADDRESS 0x3C   //Displays I2C address
#define OLED_SDA 4
#define OLED_SCL 5
#define OLED_WIDTH 128      //Display pixel width
#define OLED_HEIGHT 64      //Display pixel height
#define OLED_RESET -1

#define RADIO_ADDRESS 0x60  //FM-modules I2C address
#define RADIO_SDA 4
#define RADIO_SCL 5

#define ENCODER_CLK 7
#define ENCODER_DT 8
#define ENCODER_SW 9

#define TUNE_SLOW 0.05
#define TUNE_MEDIUM 0.1
#define TUNE_FAST 1

enum TuneOption {SLOW = 0, MEDIUM = 1, FAST = 2};

//Global variables
unsigned int currentCLK;
unsigned int lastCLK;
unsigned int buttonPushed = LOW;
TuneOption tuneOption = MEDIUM;
float fstep = TUNE_MEDIUM;

unsigned int f_B;
unsigned int f_H = 0;
unsigned int f_L = 0;
float f;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

void setFreq() {
  //Calculating the tuning frequency
  f_B = 4* (f * 1000000 + 225000) / 32768;
  int f_H = f_B >> 8;       //Shift bits 8 times to right
  int f_L = f_B & 0XFF;     //Isolate last 8 bits

  //Communicate with TEA5767 to receive
  Wire.beginTransmission(RADIO_ADDRESS);
  Wire.write(f_H);
  Wire.write(f_L);
  Wire.write(0xB0);
  Wire.write(0x10);
  Wire.write((byte)0x00);
  Wire.endTransmission();

  switch (tuneOption) {
    case SLOW: fstep = TUNE_SLOW; break;
    case MEDIUM: fstep = TUNE_MEDIUM; break;
    case FAST: fstep = TUNE_FAST; break;
    default: fstep = TUNE_MEDIUM; break;
  }
}

void setDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  //Write listened FM frequency
  display.setCursor(0, 0);
  display.println("FM:");
  display.println(f);
  display.println("Fstep:");
  display.println(fstep);
  display.display();
}

void setState() {
  //Do not allow frequencies below 87.5M or above 108M 
  if (f<87.5) f=87.5;
  if (f>108) f=108;
  //Set Radio & Display
  setFreq();
  setDisplay();
}

void setup() {
  //Setup
  pinMode(ENCODER_CLK,INPUT);
  pinMode(ENCODER_DT,INPUT);
  pinMode(ENCODER_SW,INPUT_PULLUP);

  lastCLK = digitalRead(ENCODER_CLK);
  
  Wire.begin();
  
  pinMode(RADIO_SCL,INPUT);
  pinMode(RADIO_SDA,INPUT);

  f = 96.8;     //Start from 96.8MHz

  //Set parameters to display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);

  setState();
}

void loop() {
  //Read rotary encoder data, if rotated -> shift frequency
  currentCLK = digitalRead(ENCODER_CLK);
  if (currentCLK != lastCLK && currentCLK == 1){
		if (digitalRead(ENCODER_DT) == currentCLK) {
			f+=fstep;
      setState();
		} 
    if (digitalRead(ENCODER_DT) != currentCLK) {
			f-=fstep;
      setState();
		}
  }
  lastCLK = currentCLK;

  //Button selects frequency step from tuneOptions
  if (digitalRead(ENCODER_SW) == LOW) {
    tuneOption = (tuneOption + 1) % 3;
    delay(200);
    setState();
  }
  delay(1);
}