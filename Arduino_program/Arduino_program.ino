#include <SPI.h>
#include <Wire.h>

// Adafruit TFT knihovna (místo LCDWIKI)
#include "Adafruit_GFX.h"
#include "Adafruit_ST7796S_kbv.h"

#include <DHT.h>
#define DHTPIN 42
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temp;
float hum;

//led paske
#include <FastLED.h>
#define NUM_LEDS 24
#define DATA_PIN_LED 5
CRGB leds[NUM_LEDS];

#include <Servo.h>
Servo myServo;

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12 
#define CLK_PIN   52
#define DATA_PIN  51
#define CS_PIN    40
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#include <MQ135.h>
#define pinAMQ A8
#define pinDMQ 32
float ppm;
MQ135 senzorMQ = MQ135(pinAMQ);

float reading;
#define LIGHTSENSORPIN A9

// Nastavení pinů TFT displeje pro Adafruit knihovnu
#define TFT_CS A5    
#define TFT_DC A3    // DC = CD (Data/Command)
#define TFT_RST A4
// LED pin (A0) se u Adafruit knihovny nepoužívá přímo v konstruktoru

// Konstruktor Adafruit TFT displeje
Adafruit_ST7796S_kbv tft = Adafruit_ST7796S_kbv(TFT_CS, TFT_DC, TFT_RST);

// Definice barev (RGB565 formát)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0

// oled displej
#include <U8glib.h>
#define SCK 13
#define SDA 11
#define RES 10
#define DC 9
#define CS 8
// konstruktor OLED displeje
U8GLIB_SH1106_128X64 oled(SCK, SDA, CS, DC, RES);

// tlacitka up down left right
#define buttonDn 36
#define buttonUp 38
#define buttonLeft 41
#define buttonRight 39

// ultrazvukove cidlo
#define trig 22
#define echo 23
long odezva;
long vzdalenost;

#define IRsenzor 24
#define PIRsenzor 25

//rgb led
#define redLed 4
#define greenLed 6
#define blueLed 7

//promenne pro "debouncing tlacitek"
bool buttonUpClicked = true;
bool buttonDnClicked = true;
bool buttonLeftClicked = true;
bool buttonRightClicked = true;

//souradnice pohybliveho ramecku
int xCorSelectBoxStart = 30;
int yCorSelectBoxStart = 320/2 - 130;
int xCorSelectBoxEnd = 130;
int yCorSelectBoxEnd = 320/2 - 10;

//minuly stav souradnic pohybliveho ramecku na premazani
int xCorSelectBoxStartLast;
int yCorSelectBoxStartLast;
int xCorSelectBoxEndLast;
int yCorSelectBoxEndLast;

bool displayChanged = false;
bool menuInitialized = false; // flag pro prvotní vykreslení menu
// index který slouží k orientaci v navigačním menu
int menuIndex = 10; 

// promenne pro funkci millis(), misto delay
int period = 1000;
unsigned long time_now = 0;

String datumcas = "";
String espMessage = "";           // uložená zpráva z ESP
unsigned long espMessageTime = 0; // čas kdy přišla zpráva
bool showEspMessage = false;      // flag jestli zobrazovat ESP zprávu
String lastDisplayedText = "";    // poslední zobrazený text
String localIP = "";
String RGBvalueServer = "";
String motorValueServer = "";
int motorValue = 0;

int hodnota;

// promenne pro IR senzor
bool lastIRstate = HIGH;

// Upravená funkce pro Adafruit TFT displej
void show_string(const char *string, int16_t x, int16_t y, uint8_t textSize, uint16_t textColor, uint16_t backgroundColor)
{
    tft.setTextSize(textSize);
    tft.setTextColor(textColor, backgroundColor);
    tft.setCursor(x, y);
    tft.print(string);
}

void clear_screen(void)
{
   tft.fillScreen(COLOR_BLACK);
}

// funkce vykreslující OLED
void clearOLED() {
  oled.firstPage();
  do {
  } while ( oled.nextPage() );
}

void draw() {
  //predchozi
  oled.drawStr(5, 10, "*");
  switch(menuIndex){
    case 10:
      oled.drawStr(18, 15, "vzdalenost [cm]");
      oled.setFont(u8g_font_fub30);
      oled.drawStr(44, 60, String(vzdalenost).c_str());
      oled.setFont(u8g_font_7x14);
      
      break;
    case 11:
      oled.drawStr(10, 15, "passive infrared");
      if(digitalRead(PIRsenzor) == HIGH){
        oled.setFont(u8g_font_fub30);
        oled.drawStr(5,60,"pohyb!");
        oled.setFont(u8g_font_7x14);
      }
      break;
    case 12:
      oled.drawStr(10, 15, "infrared senzor");
      if(digitalRead(IRsenzor) == LOW){  // změň HIGH na LOW
        oled.setFont(u8g_font_fub20);
        oled.drawStr(5,40,"prekazka!");
        oled.setFont(u8g_font_7x14);
      }
      break;
    case 13:
      oled.drawStr(10,15,"IP adresa serveru");
      oled.setFont(u8g_font_fub11);
      oled.drawStr(10,50,localIP.c_str());
      oled.setFont(u8g_font_7x14);

      break;
    case 20:
      oled.drawStr(24, 15, "teplota [C]");
      oled.setFont(u8g_font_fub30);
      oled.drawStr(14,60,String(temp).c_str());
      oled.setFont(u8g_font_7x14);
      
      break;
    case 21:
     
      oled.drawStr(24, 15, "vlhkost [%]");
      oled.setFont(u8g_font_fub30);
      oled.drawStr(14,60,String(hum).c_str());
      oled.setFont(u8g_font_7x14);
      break;
    case 22:

      if(millis() >= time_now + period){
        time_now += period;
        reading = analogRead(LIGHTSENSORPIN); //Read light level
      }
      oled.drawStr(24, 15, "osvit");
      oled.setFont(u8g_font_fub30);
      oled.drawStr(14,60,String(reading).c_str());
      oled.setFont(u8g_font_7x14);

      break;
    case 23:
      
      oled.drawStr(10, 15, "koncentrace [ppm]");
      if(millis() >= time_now + period){
        time_now += period;
        ppm = senzorMQ.getPPM();
      }
      oled.setFont(u8g_font_fub30);
      oled.drawStr(30,60,String(ppm).c_str());
      oled.setFont(u8g_font_7x14);
      break;
  }
}

void setRGBLEDcolor(int Red,int Green,int Blue){
  analogWrite(redLed,Red);
  analogWrite(greenLed,Green);
  analogWrite(blueLed,Blue);
}


void setup() {

  // sériový monitor
  Serial.begin(9600);
  //komunikace s ESP8266
  Serial3.begin(115200);

  SPI.begin();

  //led páske
  FastLED.addLeds<NEOPIXEL, DATA_PIN_LED>(leds, NUM_LEDS);
  FastLED.setBrightness(100);

  P.begin();
  P.setIntensity(10);

  // Inicializace matice s výchozím textem
  datumcas = "Cekam na cas...";
  P.displayText(datumcas.c_str(), PA_CENTER, 1, 100, PA_NO_EFFECT, PA_NO_EFFECT);
  lastDisplayedText = datumcas;

  myServo.attach(3);
  myServo.write(40);

  dht.begin();

  // nastavení OLED displeje
  oled.setFont(u8g_font_7x14);
  clearOLED();

  // Inicializace Adafruit TFT displeje
  tft.begin();
  tft.fillScreen(COLOR_BLACK);
  tft.setRotation(1);  // Landscape orientace
  
  // Volitelné: nastavení podsvícení TFT (pin A0)
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);  // Zapnutí podsvícení

  // čidlo vzdálenosti HC-SR04
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  // ostatní senzory
  pinMode(IRsenzor, INPUT);
  pinMode(PIRsenzor, INPUT);
  
  //tlacitka
  pinMode(buttonDn,INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);

  //dopsat co to je za piny
  pinMode(CS_PIN, OUTPUT);
  pinMode(DHTPIN, OUTPUT);

  pinMode(LIGHTSENSORPIN, INPUT); 
  
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);

  pinMode(24, INPUT);
}
static unsigned long lastTempSend = 0;

void loop() {
  
  // načtení hodnot z DHT11 pro zobrazení na web serveru
  temp = dht.readTemperature();
  hum = dht.readHumidity();
  if(millis() - lastTempSend > 500){
    Serial3.print(temp);
    lastTempSend = millis();
  }
  
  //reset LEDek
  for(int i = 0; i< 24;i++){
    leds[i] = CRGB::Black;
  }
  
  

  // Kontrola timeoutu (30 sekund)
  if(showEspMessage && (millis() - espMessageTime > 30000)){
      showEspMessage = false;
  }

  // Zobraz správný text
  String textToDisplay = showEspMessage ? espMessage : datumcas;

  // Aktualizuj JEN když se změní
  if(textToDisplay != lastDisplayedText && textToDisplay != ""){
    P.print(textToDisplay.c_str());  // ZMĚNA z displayText na print
    lastDisplayedText = textToDisplay;
  }

  // příjem dat z onboard ESP8266 / web serveru
  String ESPdata = "";
  while (Serial3.available() > 0) {  //když je dostupný serial3 (esp)
    char inByte = Serial3.read(); 

    if(inByte != ""){
      ESPdata += String(inByte);
    }
  }
  
  if(ESPdata != ""){
    Serial.println("ESP8266 DATA:");                  
    Serial.println(ESPdata + "*");
  }
  
  if(ESPdata.indexOf("+") >= 0){
    localIP = ESPdata.substring(2,15);
    Serial.print("lokal IP je: ");
    Serial.println(localIP);
  }
  else if(ESPdata.indexOf("tlacOFF") >= 0){
    Serial.println("tlacoff test");
  }
  else if(ESPdata.indexOf("tlacON") >= 0){
    Serial.println("tlacontest");
  }
  else if(ESPdata.indexOf("*") >= 0){
    espMessage = ESPdata.substring(1);
    espMessageTime = millis();
    showEspMessage = true;
    // Odstraněn delay(1000)!
  } 
  else if(ESPdata.indexOf("!") >= 0){
    RGBvalueServer = ESPdata.substring(1);

    int firstIndex = RGBvalueServer.indexOf("-");
    int secondIndex = RGBvalueServer.indexOf(":");

    int redValueServer = RGBvalueServer.substring(0,firstIndex).toInt();
    int greenValueServer = RGBvalueServer.substring(firstIndex + 1,secondIndex).toInt();
    int blueValueServer = RGBvalueServer.substring(secondIndex + 1).toInt();

    Serial.print("RGB value: ");
    Serial.print(redValueServer);
    Serial.print(greenValueServer);
    Serial.println(blueValueServer);

    setRGBLEDcolor(redValueServer,greenValueServer,blueValueServer);
  }
  else if(ESPdata.indexOf(";") >= 0){
    motorValueServer = ESPdata.substring(1);
    motorValue = motorValueServer.toInt();
    myServo.write(motorValue);
  }
  else if(ESPdata.indexOf("_") >= 0){
    datumcas = ESPdata.substring(1,16);
    Serial.print("Prijat cas z ESP: [");
    Serial.print(datumcas);
    Serial.println("]");
  }
  ESPdata = "";

  xCorSelectBoxStartLast = xCorSelectBoxStart;
  yCorSelectBoxStartLast = yCorSelectBoxStart;
  xCorSelectBoxEndLast = xCorSelectBoxEnd;
  yCorSelectBoxEndLast = yCorSelectBoxEnd;

  //handlery tlacitek
  if((digitalRead(buttonUp) == LOW) && (buttonUpClicked == true)){
    if((menuIndex % 100)/10 == 2){
      menuIndex -=10;
      yCorSelectBoxStart -= 140;
      yCorSelectBoxEnd -=140;
      displayChanged = true;
      lastIRstate = HIGH; // reset při opuštění IR mode
    }
    buttonUpClicked = false;
  }
  if((digitalRead(buttonDn) == LOW) && (buttonDnClicked == true)){
    if((menuIndex % 100)/10 == 1){
      menuIndex +=10;
      yCorSelectBoxStart += 140;
      yCorSelectBoxEnd +=140;
      displayChanged = true;
      lastIRstate = HIGH; // reset při opuštění IR mode
    }
    buttonDnClicked = false;
  }
  if((digitalRead(buttonLeft) == LOW) && (buttonLeftClicked == true)){
    if((menuIndex % 10)/1 == 3 || (menuIndex % 10)/1 == 2 || (menuIndex % 10)/1 == 1){
      menuIndex -= 1;
      xCorSelectBoxStart -= 100;
      xCorSelectBoxEnd -= 100;
      displayChanged = true;
      lastIRstate = HIGH; // reset při opuštění IR mode
    }
    buttonLeftClicked = false;
  }
  if((digitalRead(buttonRight) == LOW) && (buttonRightClicked == true)){
    if((menuIndex % 10)/1 == 0 || (menuIndex % 10)/1 == 1 || (menuIndex % 10)/1 == 2){
      menuIndex += 1;
      xCorSelectBoxStart += 100;
      xCorSelectBoxEnd += 100;
      displayChanged = true;
      lastIRstate = HIGH; // reset při opuštění IR mode
    }
    buttonRightClicked = false;
  }
  if ((digitalRead(buttonUp) == HIGH) && (buttonUpClicked == false)) {
    buttonUpClicked = true;
  }
  if ((digitalRead(buttonDn) == HIGH) && (buttonDnClicked == false)) {
    buttonDnClicked = true;
  }
  if ((digitalRead(buttonLeft) == HIGH) && (buttonLeftClicked == false)) {
    buttonLeftClicked = true;
  }
  if ((digitalRead(buttonRight) == HIGH) && (buttonRightClicked == false)) {
    buttonRightClicked = true;
  }

  //switch statement na vykonani vybrane akce
  switch(menuIndex){
    case 10:
      digitalWrite(trig, LOW);
      delayMicroseconds(2);
      digitalWrite(trig, HIGH);
      delayMicroseconds(5);
      digitalWrite(trig, LOW);
      odezva = pulseIn(echo, HIGH, 30000); // timeout 30ms místo default 1s
      vzdalenost = odezva / 58.31;
      
      // Omezení vzdálenosti na rozumné hodnoty
      if(vzdalenost > 400 || vzdalenost < 2) {
        vzdalenost = 0;
      }
      
      Serial.println(vzdalenost);
      setRGBLEDcolor(0,250,0);
      
      hodnota = map(vzdalenost,0,70,0,24);
      hodnota = constrain(hodnota, 0, 24); // zajištění že hodnota je v rozsahu
      for(int i = 0; i< hodnota;i++){
        leds[i] = CRGB::Blue;
      }
      break;
      
    case 11:
      Serial.print("PIR stav: ");
      Serial.println(digitalRead(PIRsenzor));
      if(digitalRead(PIRsenzor) == HIGH){
        Serial.println("detekován pohyb");
        for(int i = 0;i<24;i++){
          leds[i] = CRGB::Red;
        }
      }
      else{
        for(int i = 0;i<24;i++){
          leds[i] = CRGB::Black;
        }
      }
      break;
      
    case 12:
      {
      bool currentIRstate = digitalRead(IRsenzor);
      
      if(currentIRstate != lastIRstate) {
        if(currentIRstate == LOW){  // změň HIGH na LOW
          Serial.println("detekována překážka");
          myServo.write(90);
        }
        else{
          myServo.write(180);
        }
        lastIRstate = currentIRstate;
      }
    }
    break;
      
    case 13:
      Serial.println("Server");
      setRGBLEDcolor(250,0,0);
      break;
      
    case 20:
      temp = dht.readTemperature();
      Serial.print("Teplota: ");
      Serial.println(temp);
      break;
      
    case 21:
      hum = dht.readHumidity();
      break;
      
    case 22:
      reading = analogRead(LIGHTSENSORPIN); 
      Serial.println(reading);
      break;
      
    case 23:
      ppm = senzorMQ.getPPM();
      Serial.print("Koncentrace plynů: ");
      Serial.println(ppm);
      break;
      
    default:
      Serial.println(menuIndex);
      break;
  }

  // Vykreslení GUI na TFT pomocí Adafruit knihovny
  
  // Prvotní vykreslení menu (jen jednou)
  if(!menuInitialized) {
    // Cara přes půl obrazovky (horizontální čára)
    tft.drawFastHLine(30, 320/2, 420, COLOR_WHITE);
    
    // Názvy políček - horní řada
    show_string("dist", 40, 320/2 - 80, 3, COLOR_WHITE, COLOR_BLACK);
    show_string("PIR", 140, 320/2 - 80, 3, COLOR_WHITE, COLOR_BLACK);
    show_string("IR", 240, 320/2 - 80, 3, COLOR_WHITE, COLOR_BLACK);
    show_string("Server", 340, 320/2 - 80, 3, COLOR_WHITE, COLOR_BLACK);
    
    // Názvy políček - spodní řada
    show_string("temp", 40, 320/2 + 60, 3, COLOR_WHITE, COLOR_BLACK);
    show_string("hum", 140, 320/2 + 60, 3, COLOR_WHITE, COLOR_BLACK);
    show_string("Lx", 240, 320/2 + 60, 3, COLOR_WHITE, COLOR_BLACK);
    show_string("AirQ", 340, 320/2 + 60, 3, COLOR_WHITE, COLOR_BLACK);
    
    menuInitialized = true;
  }
  
  // Premazani puvodniho ramecku (jen pokud nastala nejaka zmena)
  if(displayChanged){
    tft.drawRect(xCorSelectBoxStartLast, yCorSelectBoxStartLast, 
                 xCorSelectBoxEndLast - xCorSelectBoxStartLast, 
                 yCorSelectBoxEndLast - yCorSelectBoxStartLast, COLOR_BLACK);
    displayChanged = false;
  }
  
  // Vykresleni noveho ramecku
  tft.drawRect(xCorSelectBoxStart, yCorSelectBoxStart, 
               xCorSelectBoxEnd - xCorSelectBoxStart, 
               yCorSelectBoxEnd - yCorSelectBoxStart, COLOR_WHITE);

  // OLED vykreslení (musí být na konci programu)
  oled.firstPage();
  do {
    draw();
  } while (oled.nextPage());
  FastLED.show();
}
