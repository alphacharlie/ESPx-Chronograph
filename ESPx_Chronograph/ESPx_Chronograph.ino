#include <Wire.h> 
#include <LCD.h>
#include <LiquidCrystal_I2C.h>



#define BACKLIGHT_PIN     3

//Pins here are ESP 8266 specific and are unused pins on the UEXT connector
#define GATE_1_ENABLE_PIN 12
#define GATE_2_ENABLE_PIN 13

#define GATE_1_PIN 14
#define GATE_2_PIN 15

#define LED_1_PIN 16
#define LED_2_PIN 3

#define RELAY_PIN 5

//LiquidCrystal_I2C lcd(0x38);  // Set the LCD I2C address

LiquidCrystal_I2C lcd(0x27, BACKLIGHT_PIN, POSITIVE);  // Set the LCD I2C address

int loopCount = 0;
long upTime = 0; 
static char outBuf[12];

static bool gate1Bang = false;
static bool gate2Bang = false;
volatile bool gatesReady = false;
volatile unsigned long bang1Clicks = 0;
volatile unsigned long bang2Clicks = 0;
static unsigned long shotClicks = 0;
static unsigned long lastShotClicks = 0;
volatile unsigned long tmpVal = 0;


// Creat a set of new characters
const uint8_t charBitmap[][8] = {
   { 0xc, 0x12, 0x12, 0xc, 0, 0, 0, 0 },
   { 0x6, 0x9, 0x9, 0x6, 0, 0, 0, 0 },
   { 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0, 0x0 },
   { 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0, 0x0 },
   { 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0x0 },
   { 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0x0 },
   { 0x0, 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0x0 },
   { 0x0, 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0x0 }
   
};

void shotAnimate(int _rate)
{
  lcd.home();
  for ( int j = 0; j < 12; j++ )
  {
    lcd.setCursor( 0, 0);
    lcd.print("                ");
    lcd.setCursor ( j, 0 );        // go to the next spot
    lcd.print(">>>---->");
    delay ( 100 / _rate );  
    
  }
  lcd.setCursor( 0, 0);
  lcd.print("               *");
  delay(1000);
};

void displaySpeed(int delayClicks, int rate)
{
  /*
   *    The distance between gates is 30.48cm (1ft)
   *    at 80000000 ticks per second on the CCOUNT register
   *    
   *      fps = 80000000 / count in ticks 
   * 
   */
   
  float feetSec = 80000000.00 / (float)delayClicks;  
  
  //get meters / sec
  float metersSec = feetSec * 0.3048;  
  
  
    lcd.setCursor( 0, 1);
    lcd.print("                ");
    lcd.setCursor ( 3, 1);        // go to the next spot
    dtostrf(metersSec,8, 2, outBuf);
    lcd.print(outBuf);
    lcd.setCursor( 11, 1);
    lcd.print(" m/s");
    delay ( 3000 / rate );  
    lcd.setCursor( 0, 1);
    lcd.print("                ");
    lcd.setCursor ( 3, 1);        // go to the next spot
    dtostrf(feetSec,8, 2, outBuf);
    lcd.print(outBuf);
    lcd.setCursor( 11, 1);
    lcd.print(" ft/s");
    delay ( 3000 / rate );  
}

void gate1Tripped(){
  //get the CCOUNT first!!!
  tmpVal = ESP.getCycleCount();
  if(gatesReady)
  {
    if(!gate1Bang)
    {
      bang1Clicks = tmpVal;
      gate1Bang = true;

    }
  }
  digitalWrite(LED_1_PIN, HIGH);
  tmpVal = 0;
}

void gate2Tripped(){
    //get the CCOUNT first!!!
  tmpVal = ESP.getCycleCount();
  if(gatesReady && gate1Bang)
  {
    bang2Clicks = tmpVal;
    gate2Bang = true;
  }
  digitalWrite(LED_2_PIN, HIGH);
  tmpVal = 0;
}

void resetGates(){
  gatesReady = false;
 // lcd.home ();                   // go home
//  lcd.print("       Resetting");  
  yield();

  digitalWrite(GATE_1_ENABLE_PIN, LOW);
  digitalWrite(GATE_2_ENABLE_PIN, LOW);
  delay(100);
  
  gate1Bang = false;
  gate2Bang = false;
  shotClicks = 0;
  bang1Clicks = 0;
  bang2Clicks = 0;
  
  digitalWrite(GATE_1_ENABLE_PIN, HIGH);
  digitalWrite(GATE_2_ENABLE_PIN, HIGH);
  digitalWrite(LED_1_PIN, LOW);
  digitalWrite(LED_2_PIN, LOW);
  gatesReady = true;

}
void setup()
{
  int charBitmapSize = (sizeof(charBitmap ) / sizeof (charBitmap[0]));
  // Switch on the backlight
  pinMode ( BACKLIGHT_PIN, OUTPUT );
  digitalWrite ( BACKLIGHT_PIN, HIGH );
  
  lcd.begin(16,2);               // initialize the lcd 

   for ( int i = 0; i < charBitmapSize; i++ )
   {
      lcd.createChar ( i, (uint8_t *)charBitmap[i] );
   }

  lcd.home ();                   // go home
  lcd.print("CHRONOGRAPH ");  
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print (" Initializing... "); 

  //set gate pin modes. the TRIGGER ENABLE PINs provide the A voltage to the SCR and are cycled to reset the triggers
   
  pinMode(GATE_1_ENABLE_PIN, OUTPUT);
  pinMode(GATE_2_ENABLE_PIN, OUTPUT);
  
  pinMode(LED_1_PIN, OUTPUT);
  pinMode(LED_2_PIN, OUTPUT);
  
  pinMode(RELAY_PIN, OUTPUT); 

   
  digitalWrite(GATE_1_ENABLE_PIN, HIGH);
  digitalWrite(GATE_2_ENABLE_PIN, HIGH);
  digitalWrite(RELAY_PIN, LOW);
    
  pinMode(GATE_1_PIN, INPUT);
  pinMode(GATE_2_PIN, INPUT); 
  attachInterrupt(digitalPinToInterrupt(GATE_1_PIN), gate1Tripped, CHANGE);
  attachInterrupt(digitalPinToInterrupt(GATE_2_PIN), gate2Tripped, CHANGE);
  
  upTime = millis();
  digitalWrite(LED_1_PIN, LOW);
  digitalWrite(LED_2_PIN, HIGH);
  delay(250);
  digitalWrite(LED_1_PIN, HIGH);
  digitalWrite(LED_2_PIN, LOW);
  delay(250); 
  digitalWrite(LED_1_PIN, LOW);
  digitalWrite(LED_2_PIN, HIGH);
  delay(250);
  digitalWrite(LED_1_PIN, HIGH);
  digitalWrite(LED_2_PIN, LOW);
  delay(250); 
  digitalWrite(LED_1_PIN, LOW);
  gatesReady = true;
}

void loop()
{
  if(gatesReady)
  {
    while((bang1Clicks == 0) || (bang2Clicks == 0))
    {
      lcd.home ();                   // go home
      lcd.print("Ready!          ");  
      lcd.setCursor ( 0, 1 );        // go to the next line


      //this shows the speed of the ast shot and is pretty much a screensaver
      if(lastShotClicks <= 0)
      {
        lcd.print (" watching gate  "); 
        delay(200);
      }
      else
      {
        displaySpeed(lastShotClicks, 4);
      }   
      
      //clean up (sometimes the SCR gates get stuck...)
      if((!digitalRead(GATE_1_PIN) && !digitalRead(GATE_2_PIN)) && ((bang1Clicks == 0) || (bang2Clicks == 0)))
      {
        resetGates();
      }
        
    }
  
    if(bang2Clicks > bang1Clicks)
    {
     shotClicks = bang2Clicks - bang1Clicks;
    }
    else
    {
      //CCOUNT has reset...
      unsigned long tmpClick1 = 4294967295 - bang1Clicks; //subtract the clicks from the max value
      shotClicks = bang2Clicks + tmpClick1; //then add the second value...
    }
    lastShotClicks = shotClicks;
    lcd.setCursor(0,1);
    lcd.print("Shot Detected!  ");
    resetGates();
        
    lcd.noBacklight();
    delay(50);
    lcd.backlight();
    shotAnimate(4);
    displaySpeed(lastShotClicks, 3);

  }
  else delay(50);
}

