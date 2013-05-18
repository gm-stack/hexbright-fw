#include <math.h>
#include <Wire.h>

////////////////////////////////////////////////////////////////////////////
// misc constants
////////////////////////////////////////////////////////////////////////////
#define OVERTEMP  340
#define FADE_TIME 100
//#define CHG_RED

////////////////////////////////////////////////////////////////////////////
// pin definitions
////////////////////////////////////////////////////////////////////////////



#define D_RLED_SW   2
// red LED in end button *and* button switch. Seriously, they couldn't put these on different pins?

#define D_GLED      5
// green LED in end button

// datasheet for LED controller http://www.ti.com/general/docs/lit/getliterature.tsp?genericPartNumber=tps63020&fileType=pdf

#define D_PWR       8
// enable pin for voltage regulator

#define D_DRV_MODE  9
// seems to be low on low/med power, and high on full power. Maybe HIGH -> direct drive

#define D_DRV_EN    10
// enable pin for LED driver - PWM this to dim the LED

#define A_TEMP      0
// analog temp sensor, datasheet http://ww1.microchip.com/downloads/en/DeviceDoc/21942e.pdf

#define A_CHARGE    3
// charge status: low = charging, high = fully charged, HiZ = DC power not connected


#define MODE_OFF    0
#define MODE_LOW    1
#define MODE_MED    2
#define MODE_HIGH   3
byte mode = 0;

void setup() {
  pinMode(D_PWR, INPUT);
  digitalWrite(D_PWR, LOW);
    
  pinMode(D_DRV_MODE, OUTPUT);
  pinMode(D_DRV_EN, OUTPUT);
  digitalWrite(D_DRV_MODE, LOW);
  digitalWrite(D_DRV_EN, LOW);
  
  pinMode(D_RLED_SW, INPUT);
  pinMode(D_GLED, OUTPUT);
  
  Serial.begin(9600);
  Wire.begin();
  mode = MODE_OFF;
}

void checkPower() {
  pinMode(D_PWR, OUTPUT);
  digitalWrite(D_PWR, HIGH);
  
  while(digitalRead(D_RLED_SW)) {};
  
  delay(100);
  analogRead(6);
  bitSet(ADMUX, 3);
  delayMicroseconds(250);
  bitSet(ADCSRA, ADSC);
  while (bit_is_set(ADCSRA, ADSC)) {};
  word x = ADC;
  long int a = (1100L * 1023) / x;
  
  byte v = a / 1000;
  byte dv = (a / 100) % 10;
  
  Serial.print("a=");
  Serial.print(a);
  Serial.print(" v=");
  Serial.print(v);
  Serial.print(" dv=");
  Serial.println(dv);
  
  delay(400);
  pinMode(D_RLED_SW, OUTPUT);
  for (int i=0; i<v; i++) {
    digitalWrite(D_RLED_SW, HIGH);
    delay(200);
    digitalWrite(D_RLED_SW, LOW);
    delay(200);
  }
  delay(500);
    for (int i=0; i<dv; i++) {
    digitalWrite(D_RLED_SW, HIGH);
    delay(200);
    digitalWrite(D_RLED_SW, LOW);
    delay(200);
  }
  pinMode(D_RLED_SW, INPUT);
  digitalWrite(D_PWR, LOW);
  delay(10);
}

byte usbconn = 0;
byte lastBtnDown = 0;
byte btnDownLatch = 0;
unsigned long lastBtnDownTime = 0;
unsigned long btnUpTime = 0;
unsigned long mode_switch_time = 0;
byte targetBrightness = 0;
byte prevBrightness = 0;
byte nowBrightness = 0;

void loop() {
  static unsigned long lastTempTime;
  unsigned long time = millis();
  
  int chargeState = analogRead(A_CHARGE);
  if (chargeState < 128) { // charging
    byte analogVal = (time / 16) % 255; // TODO HERE: software PWM
    //Serial.println(analogVal);
    #ifdef CHG_RED
    pinMode(D_RLED_SW, OUTPUT);
    digitalWrite(D_RLED_SW, HIGH);
    #endif
    digitalWrite(D_GLED, LOW);
    usbconn = 1;
  } else if (chargeState > 768) { // full charged
    pinMode(D_RLED_SW, INPUT);
    digitalWrite(D_GLED, HIGH);
    usbconn = 1;
  } else {
    pinMode(D_RLED_SW, INPUT);
    digitalWrite(D_GLED, LOW);
    usbconn = 0;
  }
  
  
  // TODO here: dim the light in this case
  if (time-lastTempTime > 1000)
  {
    lastTempTime = time;
    int temperature = analogRead(A_TEMP);
    Serial.print("Temp: ");
    Serial.println(temperature);
    if (temperature > OVERTEMP && mode != MODE_OFF)
    {
      Serial.println("Overheating!");
      digitalWrite(D_DRV_MODE, LOW);
      mode = MODE_LOW;
    }
  }
  
  if (!usbconn) {
    pinMode(D_RLED_SW, OUTPUT);
    digitalWrite(D_RLED_SW, LOW);
    pinMode(D_RLED_SW, INPUT);
  }
 
 byte btnDown = digitalRead(D_RLED_SW);

 if ((!lastBtnDown) && (btnDown)) {
   lastBtnDownTime = millis();
   btnDownLatch = 0;
 }
 if (((lastBtnDown) && (!btnDown)) || (((millis() - lastBtnDownTime) > 500) && btnDown)) {
   if (!btnDownLatch) {
     btnUpTime = millis() - lastBtnDownTime;
     Serial.print("button down for ");
     Serial.println(btnUpTime);
     btnDownLatch = 1;
   } else {
     btnUpTime = 0;
     delay(10);
   }
 }
 lastBtnDown = btnDown;
 
 if (btnUpTime && btnUpTime <= 500) {
   btnUpTime = 0;
   switch (mode) {
     case MODE_OFF:
       mode = MODE_LOW;
       mode_switch_time = millis();
       prevBrightness = 0;
       targetBrightness = 64;
       break;
     case MODE_LOW:
       mode = MODE_MED;
       mode_switch_time = millis();
       prevBrightness = 64;
       targetBrightness = 255;
       break;
     case MODE_MED:
       mode = MODE_HIGH;
       mode_switch_time = millis();
       prevBrightness = 64;
       targetBrightness = 255;
       break;
     case MODE_HIGH:
       mode = MODE_OFF;
       prevBrightness = 255;
       targetBrightness = 0;
       break;
   }
   Serial.print("mode is ");
   Serial.println(mode);
 }
 
 if (btnUpTime > 500) {
   btnUpTime = 0;
   switch (mode) {
     case MODE_OFF:
       mode = MODE_OFF;
       checkPower();
       break;
     case MODE_LOW:
     case MODE_MED:
     case MODE_HIGH:
       prevBrightness = 0;
       targetBrightness = 0;
       mode = MODE_OFF;
       break;
   }
   Serial.print("mode is ");
   Serial.println(mode);
 }
 
 
 
 switch(mode) {
   case MODE_OFF:
     pinMode(D_PWR, OUTPUT);
     digitalWrite(D_PWR, LOW);
     digitalWrite(D_DRV_MODE, LOW);
     digitalWrite(D_DRV_EN, LOW);
     while(digitalRead(D_RLED_SW)) {};
     break;
   case MODE_LOW:
     pinMode(D_PWR, OUTPUT);
     digitalWrite(D_PWR, HIGH);
     digitalWrite(D_DRV_MODE, LOW);
     break;
    case MODE_MED:
     pinMode(D_PWR, OUTPUT);
     digitalWrite(D_PWR, HIGH);
     digitalWrite(D_DRV_MODE, LOW);
     break;
    case MODE_HIGH:
     pinMode(D_PWR, OUTPUT);
     digitalWrite(D_PWR, HIGH);
     digitalWrite(D_DRV_MODE, HIGH);
     break;
 }
 
 unsigned long currTime = millis() - mode_switch_time;
 //Serial.print("currTime = ");
 //Serial.println(currTime);
 if (currTime <= FADE_TIME) {
   nowBrightness = map(currTime, 0, FADE_TIME, prevBrightness, targetBrightness);
   analogWrite(D_DRV_EN, nowBrightness);
   //Serial.print("brightness = ");
   //Serial.println(nowBrightness);
 } else {
   nowBrightness = targetBrightness;
   analogWrite(D_DRV_EN, targetBrightness);
 }
     
     
     /*  if (digitalRead(D_RLED_SW)) {
      digitalWrite(D_DRV_EN, HIGH);
    } else {
      digitalWrite(D_DRV_EN, LOW);
    }
  }*/
  
  

}
