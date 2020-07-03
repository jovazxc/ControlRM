#include <RCSwitch.h>

#define RELAY_IGNITION  9
#define RELAY_START     10
#define RELAY_TAILLIGHT 11
#define RELAY_LIGHTS    12
#define RELAY_LOCK      13
#define RELAY_UNLOCK    A0
#define RELAY_HORN      A1
#define RELAY_STEREO    8
#define SALIDA_LIBRE1   7
#define SALIDA_LIBRE2   6
#define SALIDA_LIBRE3   5


const int t[] = {9, 10, 11, 12, 13, A0, A1, 8, 7, 6, 5};

#define STATUS_ENGINE A2
#define BUTTON_START  A3

#define BUTTON_LIGHTS A4
#define SIGNAL_DOORS  A5



RCSwitch mySwitch = RCSwitch();

unsigned long lastPressedTime = 0;
int buttonPressed = 0;

unsigned long lastUnlockTime = 0;
unsigned long lastStartTime = 0;
boolean ignitionState = false;

int statusTailLight = 1;
unsigned long tailLightTime = 0;

int lastButtonPosition = 0;

boolean alarmActivated = false;

int lastLight = 0;

boolean stereoState = false;


void setup() {
  Serial.begin(9600);
  Serial.setTimeout(250);
  Serial.println("Conectado");

  lastUnlockTime = -1;
  
  for(int i = 0; i<11; i++) pinMode(t[i], OUTPUT);
  
  mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
}

void loop() {  
  const int LIGHTS_VALUE = analogRead(BUTTON_LIGHTS);

  if(LIGHTS_VALUE < 200 && (lastButtonPosition == 1 || lastButtonPosition == 2)) {
    // Shutdown
    Serial.print("SwitchOFF ");
    Serial.println(LIGHTS_VALUE);
    setLightsState(0);
    lastButtonPosition = 0;
  }
  if(LIGHTS_VALUE > 250 && LIGHTS_VALUE < 310 && lastButtonPosition != 1) {
    // Tail Lights
    Serial.print("SwitchTAIL ");
    Serial.println(LIGHTS_VALUE);
    setLightsState(1);
    lastButtonPosition = 1;
    delay(50);
  }
  else if(LIGHTS_VALUE > 300 && LIGHTS_VALUE < 350 && lastButtonPosition != 2) {
    Serial.print("SwitchLIGHTS ");
    Serial.println(LIGHTS_VALUE);
    setLightsState(2);   
    lastButtonPosition = 2;
    delay(50);
  }

  if(analogRead(SIGNAL_DOORS) > 700 && alarmActivated) {
    digitalWrite(RELAY_HORN, HIGH);
    delay(500);
    digitalWrite(RELAY_HORN, LOW);
    delay(500);
  }
  
  
  if(statusEngine() == LOW && ignitionState && (millis() - lastStartTime) > 10000){
    
    ignitionState = false;
    digitalWrite(RELAY_IGNITION, LOW);
    Serial.println("TURNING IGNITION OFF BECAUSE ENGINE STOPED");
  }

  if(statusTailLight == 1 && (millis() - tailLightTime) > 3000) {
    statusTailLight = 0;
    digitalWrite(RELAY_TAILLIGHT, LOW);
  }
  
  if(mySwitch.available()) {
    unsigned long value = mySwitch.getReceivedValue();
    Serial.print("RCSwitch received ");
    Serial.println(value);
    
    switch(value) {
      case 9939048: {
        LOCK();
        break;
      }
      case 9939044: {
        UNLOCK();
        break;
      }
      case 9939042: {
        if((millis() - lastPressedTime) < 1000){
          if(buttonPressed == 2) {
            Serial.print("RC Signal, action: ");
            Serial.println(statusEngine() ? "STOP" : "START");
            statusEngine() ? STOP() : START();         } else {
            lastPressedTime = millis();
            
            buttonPressed++;
            Serial.println("buttonpresed++");
          }
        } else {
          lastPressedTime = millis();
          buttonPressed = 1;
          Serial.println("buttonpressed=1");
        }
        break;
      }
      
      case 9939041: {
        if(++lastLight == 3) lastLight = 0;
        setLightsState(lastLight);
        break;
      }
    }

    
    delay(750);
    mySwitch.resetAvailable();
    return;
  }

  if(Serial.available()) {
    String msg = Serial.readString();
    if(msg == "LOCK") LOCK();
    else if(msg == "UNLOCK") UNLOCK();
    else if(msg == "START") START();
    else if(msg == "STOP") STOP();
    else if(msg == "ONTAIL") setLightsState(1);
    else if(msg == "ONLIGHTS") setLightsState(2);
    else if(msg == "OFFLIGHTS") setLightsState(0);
    else if(msg == "ONSTEREO") {
      digitalWrite(RELAY_STEREO, HIGH);
      stereoState = true;
      Serial.println("Stereo turned on");
    }
    else if(msg == "OFFSTEREO"){
      digitalWrite(RELAY_STEREO, LOW);
      stereoState = false;
      Serial.println("Stereo turned off");
    }
    else if(msg == "READ") {
      Serial.println(analogRead(BUTTON_LIGHTS));
    }
    else if(msg == "READ1") {
      Serial.println(analogRead(BUTTON_START));
    }
    Serial.flush();
    Serial.println("Serial received " + msg);
    
  }

  const int signalStart = analogRead(BUTTON_START);
  
  if(signalStart > 250 && signalStart < 350) {
    stereoState = !stereoState; 
    digitalWrite(RELAY_STEREO, stereoState);
    Serial.print("stereoState = ");
    Serial.println(stereoState);
    delay(500);
  }
  else if(signalStart > 350) {
    
    if(ignitionState == false && (millis() - lastUnlockTime) < 30000 && lastUnlockTime != -1) {
      // TURN ON IGNITION
      lastStartTime = millis(); // FOR TURN IGNITION OFF
      ignitionState = true;
      digitalWrite(RELAY_IGNITION, HIGH);   
      delay(300);

      unsigned long pulseTime = millis();

      while(analogRead(BUTTON_START) > 350) {
        digitalWrite(RELAY_START, HIGH);
        if(millis() - pulseTime > 2500) {
          digitalWrite(RELAY_START, LOW);
          delay(5000);
          break;
        }
      }
      if(millis() - pulseTime < 250) delay(550);
      Serial.println(millis()-pulseTime);
      stereoState = true;
      digitalWrite(RELAY_START, LOW);
      digitalWrite(RELAY_STEREO, HIGH);
    }
    else if(statusEngine() == HIGH) {
      Serial.println("Shutting down from button signal");
      lastUnlockTime = -1;
      STOP();
    }
    else {
      Serial.println("Button was pressed, nothing to do");
    }
    delay(500); 
  }
  
}

bool statusEngine() {
  return analogRead(STATUS_ENGINE) > 700;
}

void setLightsState(int state) {
  Serial.print("setLightsState = ");
  Serial.println(state);
  digitalWrite(RELAY_TAILLIGHT, state >= 1);
  digitalWrite(RELAY_LIGHTS, state >= 2);
}

void LOCK() {
  Serial.println("LOCK");

  if(analogRead(SIGNAL_DOORS) > 700) {
    for(int i = 0; i<3; i++) {
      digitalWrite(RELAY_HORN, HIGH);
      delay(50);
      digitalWrite(RELAY_HORN, LOW);
      delay(50);
    }
    alarmActivated = false;    
  } else {
    digitalWrite(RELAY_HORN, HIGH);
    delay(60);
    digitalWrite(RELAY_HORN, LOW);
    alarmActivated = true;

  }
  
  digitalWrite(RELAY_LOCK, HIGH);
  delay(500);
  digitalWrite(RELAY_LOCK, LOW);

  statusTailLight = 1;
  tailLightTime = millis();
  digitalWrite(RELAY_TAILLIGHT, HIGH);

  
}

void UNLOCK() {
  lastUnlockTime=millis();
  Serial.println("UNLOCK");
  digitalWrite(RELAY_HORN, HIGH);
  delay(60);
  digitalWrite(RELAY_HORN, LOW);
  digitalWrite(RELAY_UNLOCK, HIGH);
  delay(500);
  digitalWrite(RELAY_UNLOCK, LOW);
  
  statusTailLight = 1;
  tailLightTime = millis();
  digitalWrite(RELAY_TAILLIGHT, HIGH);
  alarmActivated = false;
}

void START() {
  Serial.println("TRYING TO START..");
  if(statusEngine()){
    Serial.println("ENGINE IS ALREADY ON");
    return;
  }

  if(!ignitionState) {
    ignitionState = true;
    digitalWrite(RELAY_IGNITION, HIGH);
    Serial.println("IGNITION WAS OFF, WAITING A SECOND");
    delay(1000);   
  }
  Serial.println("STARTING...");
  digitalWrite(RELAY_START, HIGH);
  delay(750);
  digitalWrite(RELAY_START, LOW);
  digitalWrite(RELAY_STEREO, HIGH);
  stereoState = true;
  lastStartTime = millis();
}

void STOP() {
  digitalWrite(RELAY_IGNITION, LOW);
  ignitionState = false;
  Serial.println("  ENGINE STOPPING");
}
