
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <OneButton.h>

#define VREF              3.3
#define SAMPLES           2048
#define ADC_RESOL         12
#define DAC_RESOL         12
#define DELTA             10
#define ADC_MAX           ((1 << ADC_RESOL) - 1)
#define DAC_MAX           ((1 << DAC_RESOL) - 1)
#define PWM_FREQ          1000000                       // 1 MHz
#define BAUD_DATE         115200
#define LED_DELAY         100
#define BACKLIGHT_LEVEL   1024
#define SETTLE_TIME       60

#define INTERNAL_LED      25
#define TFT_BACKLIGHT     4
#define BUTTON_0          6
#define BUTTON_1          7
#define LED_OUT           21
#define LED_VOLTAGE       A1
#define POT_VOLTAGE       A2

//---------------------------------------------------------------------

TFT_eSPI tft;

OneButton up = OneButton(BUTTON_0, true, true);
OneButton dn = OneButton(BUTTON_1, true, true);

//---------------------------------------------------------------------

unsigned long analogReadSampled(int channel, int samples) {
  unsigned long total = 0;
  for (int i=0; i<samples; i++) {
    total+= analogRead(channel);
  }
  return total/samples;
}

float voltage(unsigned value) {
    return VREF * value / ADC_MAX;
}

//---------------------------------------------------------------------

int pot_value, pot_last;
int out_value;
bool led_on;
unsigned long lastTime;

void setup() {
  pinMode(INTERNAL_LED, OUTPUT);
  pinMode(LED_OUT, OUTPUT);
  
  pinMode(POT_VOLTAGE, INPUT);
  pinMode(LED_VOLTAGE, INPUT);

  analogWriteFreq(PWM_FREQ);
  analogWriteResolution(DAC_RESOL);
  analogReadResolution(ADC_RESOL);

  Serial.begin(BAUD_DATE);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  analogWrite(TFT_BACKLIGHT, BACKLIGHT_LEVEL);   

  pot_value = pot_last = analogReadSampled(POT_VOLTAGE, SAMPLES);
  out_value = pot_value;
  analogWrite(LED_OUT, out_value);    
  led_on = true;
  lastTime = millis();
}

void loop() {  
  // The potentiometer is used for coarse adjustments of the LED light level 
  pot_value = analogReadSampled(POT_VOLTAGE, SAMPLES);
  if ((pot_value >=  pot_last + DELTA) || (pot_value <=  pot_last - DELTA)) {    
    pot_last = pot_value;
    out_value = pot_value;
    analogWrite(LED_OUT, out_value);
    delay(SETTLE_TIME);
  }

  // Make the LED blink every LED_DELAY us, using the correct light intensity
  if (millis() - lastTime > LED_DELAY) {
    led_on = !led_on;
    if (led_on) {
      analogWrite(LED_OUT, out_value);
      delay(SETTLE_TIME);
      
      unsigned int voltage_read = analogReadSampled(LED_VOLTAGE, SAMPLES);

      tft.setCursor(0, 0);
      tft.printf("POT = %04d of %04d \n\n", out_value, ADC_MAX);
      tft.printf("LED = %1.3fV \n(%04d of %04d) \n", voltage(voltage_read), voltage_read, DAC_MAX);
    } else {
      digitalWrite(LED_OUT, LOW);
    }
    
    lastTime = millis();
  }
}

//---------------------------------------------------------------------

static void clickUp() {
  out_value = (out_value + 1) % (DAC_MAX + 1);
}

static void clickDown() {
  out_value--;
  if (out_value < 0)
    out_value = 0;
}

static void doubleClickUp() {
  out_value = (out_value + 10) % (DAC_MAX + 1);
}

static void doubleClickDown() {
  out_value-= 10;
  if (out_value < 0)
    out_value = 0;
}

// The push buttons are managed on the second core of the processor
void setup1() {
  up.attachClick(clickUp);
  up.attachDoubleClick(doubleClickUp);
  dn.attachClick(clickDown);
  dn.attachDoubleClick(doubleClickDown);
}

void loop1()
{
    up.tick();
    dn.tick();
    delay(10);
}
