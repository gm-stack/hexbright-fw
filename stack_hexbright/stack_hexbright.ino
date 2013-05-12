#include <math.h>
#include <Wire.h>

////////////////////////////////////////////////////////////////////////////
// misc constants
////////////////////////////////////////////////////////////////////////////
#define OVERTEMP  340

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

byte usbconn = 0;

void loop() {
  static unsigned long lastTempTime;
  unsigned long time = millis();
  
  int chargeState = analogRead(A_CHARGE);
  if (chargeState < 128) { // charging
    pinMode(D_RLED_SW, OUTPUT);
    byte analogVal = (time / 16) % 255; // TODO HERE: software PWM
    Serial.println(analogVal);
    analogWrite(D_RLED_SW, analogVal);
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
      //mode = MODE_LOW;
    }
  }
  
  if (!usbconn) {
    pinMode(D_RLED_SW, OUTPUT);
    digitalWrite(D_RLED_SW, LOW);
    pinMode(D_RLED_SW, INPUT);
    
    if (digitalRead(D_RLED_SW)) {
      digitalWrite(D_DRV_EN, HIGH);
    } else {
      digitalWrite(D_DRV_EN, LOW);
    }
  }

}
