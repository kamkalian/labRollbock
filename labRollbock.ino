
#include <Adafruit_NeoPixel.h>


/*
 * LED Ring definieren
 */
#define NUMPIXELS   12
#define LED_PIN      2
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);



/*
 * Pins für den Steppermotor definieren
 */
#define EN_PIN    2 //enable (CFG6)
#define DIR_PIN   3 //direction
#define STEP_PIN  4 //step

/*
 * Variabeln
 */
bool high;
bool runStepper;
int i = 64;

void setup() {

  /*
   * Pins für den Motor setzen
   */
  cli();
  
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH); //deactivate driver (LOW active)
  
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW); //LOW or HIGH
  
  pinMode(STEP_PIN, OUTPUT);
  
  digitalWrite(EN_PIN, LOW); //activate driver
  
  /*
   * Timer1 einrichten. Dieser wird dann für den betrieb des Steppermotors verwendet
   */
  TCCR1A = 0;
  //TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = i;

  //TCCR1B |= (1 << CS12) | (1 << CS10); //prescaler 1:1024
  //TCCR1B |= (1 << CS12); //prescaler 1:256
  TCCR1B |= (1 << CS11) | (1 << CS10); //prescaler 1:64
  TCCR1B |= (1 << WGM12);
  TIMSK1 |= (1 << OCIE1A);

  sei();

  /*
   * LED Ring Startsequenz
   */
  pixels.begin(); // This initializes the NeoPixel library.

  for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,0,255)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(150); // Delay for a period of time (in milliseconds).

  }

  runStepper = true;

}

void loop() {
  
  i--;
  if(i<14)i=14;
  delay(50);
  
}


/*
 * Timer1 Overflow Routine
 * bei jedem Aufruf wird hier abwechselnd ein high oder low gesetzen, womit dann der Steppermotor läuft.
 */
ISR(TIMER1_COMPA_vect){

  if(runStepper){
    if(high){
      digitalWrite(STEP_PIN, HIGH);
      high = false;
    }
    else {
      digitalWrite(STEP_PIN, LOW);
      high = true;
    }
    OCR1A = i;
    //Serial.println(high);
  }
  
}
