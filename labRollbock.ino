
#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>


/*
 * LED Ring definieren
 */
#define NUMPIXELS   12
#define LED_PIN      6
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

/*
 * Button Pin
 */
#define btn_Pin 7

/*
 * Pins für den Steppermotor definieren
 */
#define EN_PIN    5 //enable (CFG6)
#define DIR_PIN   3 //direction
#define STEP_PIN  4 //step

/*
 * Pins für Reflexlichtschranke
 */
 #define REFLEX_PIN   2

 #define DEBUG  0

/*
 * Variabeln
 */
bool high;
bool runStepper;
int i = 64;
int ledPosition = 0;
int btnVal = 0;
unsigned long lastRotationTime = 0;

bool btnLastState = false;
bool btnLock = false;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
bool blinky = false;
bool error = false;
int errorCounter = 0;
int errorResetCounter = 0;
int standbyCounter = 0;
int ledRing[12][3];
int standbyLED = 0;
bool standby = false;

void setup() {

  if(DEBUG) Serial.begin(9600);
  Serial.begin(9600);

  /*
   * Helligkeit einstellen
   */
  pixels.setBrightness(50);

  /*
   * Reflexlichtschranke einrichten
   */
   pinMode(REFLEX_PIN, INPUT_PULLUP);
   //pinMode(REFLEX_PIN, INPUT);
   attachInterrupt(digitalPinToInterrupt(REFLEX_PIN), checkRotate, CHANGE);

  /*
   * Button Pin einrichten
   */
   pinMode(btn_Pin,INPUT_PULLUP);

  /*
   * Pins für den Motor setzen
   */
  cli();
  
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH); //deactivate driver (LOW active)
  
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW); //LOW or HIGH
  
  pinMode(STEP_PIN, OUTPUT);
  
  digitalWrite(EN_PIN, LOW); //deactivate driver
  
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


  /*
   * Watchdog Timer einrichten
   */
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = 1<<WDP0 | 1<<WDP1 | 1<<WDP2;
  WDTCSR |= 1<<WDIE;



  sei();

  pixels.begin(); // This initializes the NeoPixel library.


  for(int led=0;led<NUMPIXELS;led++){
    
      ledRing[led][0]=0;
      ledRing[led][1]=0;
      ledRing[led][2]=255;
      delay(100);
      refreshLedRing();
 
  }

  runStepper = false;
  readyLight();

}

void refreshLedRing(){
  for(int led=0;led<NUMPIXELS;led++){
    pixels.setPixelColor(led, pixels.Color(ledRing[led][0],ledRing[led][1],ledRing[led][2]));
    //Serial.println(ledRing[led][2]);
  }
  pixels.show();
}

void loop() {

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  btnVal = digitalRead(btn_Pin);
  
  if(btnVal != btnLastState){
    lastDebounceTime = millis();
    btnLock = false;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    
    if(!btnVal && !btnLock) {
      
      if(error) {

        error = false;
        i = 64;
        readyLight();
        
      }else {
        
        if(runStepper) {
          runStepper = false;
          readyLight();
        }
        else {
          digitalWrite(EN_PIN, LOW); //activate driver
          runStepper = true;
          standby = false;
          standbyCounter = 0;
          standbyLED = 0;
          clearLedRing();
        }
        errorCounter = 0;
        i = 64;

      }
      
      btnLock = true;
      
    }

    
          
  }

  delay(100);

  /*
   * Prüfen wieviele Umdrehungen erreicht wurden
   */
   if(DEBUG) {
    Serial.print(millis() - lastRotationTime);
    Serial.print(" ");
    Serial.println(errorCounter);
   }
   
   if(runStepper && (millis() - lastRotationTime) > 100 && i==14) {
      errorCounter++;
   }else{
      errorResetCounter++;
   }
   
   if(errorCounter>5) {

    errorCounter = 0;
    error = true;
    
   }

   if(errorResetCounter >= 100) {
      errorCounter--;
      errorResetCounter = 0;
   }

   if(errorCounter <0) errorCounter = 0;
   

  if(!error){
    
    if(runStepper){
      
      runLEDRing();
      i--;
      if(i<14)i=14;
  
    } else {

      if(!standby) standbyCounter++;
      
    }

  }else {

    runStepper = false;
    digitalWrite(EN_PIN, HIGH); //deactivate driver
    errorLight();
    
    
  }

  

  if(standbyCounter>100){
    
    standbyCounter=0;
    ledRing[standbyLED][0]=255;
    ledRing[standbyLED][1]=255;
    ledRing[standbyLED][2]=0;
    refreshLedRing();

    standbyLED++;

    if(standbyLED>11){
      delay(500);
      clearLedRing();
      refreshLedRing();
      standby = true;
    }
  }

 
  Serial.println(standby);

  if(standby){
      sleep_enable();
      sleep_mode();
      sleep_disable();
  }

  btnLastState = btnVal;
  //lastRotationTime = 0;
  
}

void runLEDRing(){

  for(int i=0;i<NUMPIXELS;i++){

    if(ledPosition==i) {

      ledRing[i][0]=0;
      ledRing[i][1]=0;
      ledRing[i][2]=255;
              
    }else if(i>=errorCounter){
      
      ledRing[i][0]=0;
      ledRing[i][1]=0;
      ledRing[i][2]=0;
      
    }else if(i<errorCounter){

      ledRing[i][0]=255;
      ledRing[i][1]=0;
      ledRing[i][2]=0;
      
    }

  }
  refreshLedRing();

  nextPosition();
  
}


void readyLight(){
  for(int led=0;led<NUMPIXELS;led++){
    ledRing[led][0]=0;
    ledRing[led][1]=255;
    ledRing[led][2]=0;
    delay(50);
    refreshLedRing();
  }
  
}

void standbyLight(){

  ledRing[1][0]=100;
  ledRing[1][1]=100;
  ledRing[1][2]=0;
  refreshLedRing();
  
}

void errorLight(){

  for(int i=0;i<NUMPIXELS;i++){

    if(blinky) {
      ledRing[i][0]=255;
      ledRing[i][1]=0;
      ledRing[i][2]=0;
    }
    else clearLedRing();
    
  }
  refreshLedRing();

  blinky = !blinky; //toggle blinky
  
}

void nextPosition(){

  ledPosition++;
  if(ledPosition>=NUMPIXELS) {
    ledPosition = 0;
  }
  
}

void clearLedRing(){

  for(int i=0;i<NUMPIXELS;i++){

    ledRing[i][0]=0;
    ledRing[i][1]=0;
    ledRing[i][2]=0;
    
  }
  refreshLedRing();
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


/*
 * Watchdog Timer Routine wird alle 8s aufgerufen und soll dann den Sleep Mode beenden, wenn die Taste gedrückt ist
 */
ISR(WDT_vect){
  
  btnVal = digitalRead(btn_Pin);
  if(!btnVal) {
    standby = false;
    standbyCounter = 0;
    standbyLED = 0;
    btnLock = true;
    readyLight();
  }else{
    if(standby){
      standbyLight();
      delay(100);
      clearLedRing();
    }
  }
}

void checkRotate(){

  lastRotationTime = millis();
  
  
}

