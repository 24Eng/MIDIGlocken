// Not all pins on the Leonardo and Micro support change interrupts,
// so only the following can be used for RX:
// 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).
#include <SoftwareSerial.h>
SoftwareSerial MIDISerial(A4, 22); // RX, TX

// Declare variables
/*
  Single-player modes
Mode 0    Free play mode. Responds to the buttons or MIDI input
Mode 1    Prandom notes. Tempo is set by the next button press. Far left button is slowest, far right button is fastest.
Mode 2    Prandom groups. Uses the tempo set in mode 1, and plays bursts of notes based on the next button.

  Two-player modes
Mode 10   Uses tap tempo to play a note shortly after the user plays a note.
MOde 11   Prandom groups play based on the previous mode's tempo and sizes the group based on the next button press. Each group starts after a button press.
*/
int mode = 0;
int singleModes = 2;    // Zero justified
int doubleModes = 10 + 1;    // Ten plus zero justified
int solenoidDuration = 50;
long solenoidHolding[8];
int heldTimerForAI[3];
int holdTimeForAI[]={200, 200, 10};
int redLED = 0;
int bluLED = 0;
int grnLED = 0;
boolean serialComplete = false;
int incomingMIDIByte = 0;
int incomingMIDIMessage[3];
int incomingMIDICounter = 0;
boolean MIDIOn[8];
int mainTempo = 1500;
int prandomNote = 0;
int noteGroupSize = 0;
int noteGroupProgression = 0;
long nextNoteTime[8];
long lastNoteTime = 0;
int lastNoteGap = 0;
int currentNote = 0;


// Declare the pins
int OnePlayerPin = A6;
int TwoPlayerPin = A5;
int button0Pin = 10;
int button1Pin = 11;
int button2Pin = 12;
int button3Pin = 13;
int button4Pin = A0;
int button5Pin = A1;
int button6Pin = A2;
int button7Pin = A3;
int solenoid0 = 2;
int solenoid1 = 3;
int solenoid2 = 4;
int solenoid3 = 6;
int solenoid4 = 5;
int solenoid5 = 8;
int solenoid6 = 7;
int solenoid7 = 9;

int buttonPin[] = {button0Pin, button1Pin, button2Pin, button3Pin, button4Pin, button5Pin, button6Pin, button7Pin, OnePlayerPin, TwoPlayerPin};
boolean buttonState[10];
boolean oldButtonState[10];
boolean risingEdge[10];
boolean fallingEdge[10];
int solenoidPin[] = {solenoid0, solenoid1, solenoid2, solenoid3, solenoid4, solenoid5, solenoid6, solenoid7};


void setup() {
  for(int i=0; i<10; i++){
    if (i<10){
      pinMode(buttonPin[i], INPUT_PULLUP);
    }
    buttonState[i] = HIGH;
    oldButtonState[i] = HIGH;
  }
  for(int i=0; i<8; i++){
    pinMode(solenoidPin[i], OUTPUT);
    digitalWrite(solenoidPin[i], HIGH);
  }
  
  Serial.begin(57600);
  Serial.println("MIDIGlocken by 24Eng");
  MIDISerial.begin(31250);
}

void loop() {
  // put your main code here, to run repeatedly:
  checkButtons();
  calculateMode();
  playMode();
  serialSTUFF();
}

void serialSTUFF(){
  if (MIDISerial.available()) {
    incomingMIDIByte = MIDISerial.read();
//    printHEX(incomingMIDIByte);
    incomingMIDIMessage[incomingMIDICounter] = incomingMIDIByte;
    incomingMIDICounter++;
    serialComplete = true;
  }else{
    if (serialComplete){
      if((incomingMIDIMessage[0] >= 0x90) && (incomingMIDIMessage[0] < 0xA0)){
//        printHEX(incomingMIDIMessage[1]);
        detectMIDIOn(incomingMIDIMessage[1]);
      }
//      Serial.print("\n");
      incomingMIDICounter = 0;
      serialComplete = false;
    }
  }
}

void detectMIDIOn(int funIncomingNote){
  for(int i=0; i<8; i++){
    MIDIOn[i] = 0;
  }
  switch (funIncomingNote){
    case 0x35:
      MIDIOn[0]=HIGH;
      break;
    case 0x37:
      MIDIOn[1]=HIGH;
      break;
    case 0x39:
      MIDIOn[2]=HIGH;
      break;
    case 0x3B:
      MIDIOn[3]=HIGH;
      break;
    case 0x3C:
      MIDIOn[4]=HIGH;
      break;
    case 0x3E:
      MIDIOn[5]=HIGH;
      break;
    case 0x40:
      MIDIOn[6]=HIGH;
      break;
    case 0x41:
      MIDIOn[7]=HIGH;
      break;
    case 0x36:
      mode = 0;
      Serial.print("MIDI Mode Select: ");
      Serial.print(mode);
      Serial.print("\n");
      break;
    case 0x38:
      mode = 1;
      Serial.print("MIDI Mode Select: ");
      Serial.print(mode);
      Serial.print("\n");
      break;
    case 0x3A:
      mode = 2;
      Serial.print("MIDI Mode Select: ");
      Serial.print(mode);
      Serial.print("\n");
      break;
    case 0x3D:
      mode = 10;
      Serial.print("MIDI Mode Select: ");
      Serial.print(mode);
      Serial.print("\n");
      break;
    case 0x3F:
      mode = 11;
      Serial.print("MIDI Mode Select: ");
      Serial.print(mode);
      Serial.print("\n");
      break;
    default:
      Serial.println("Undefined");
      break;
  }
}

void printHEX(int funIncomingByte){
  Serial.print("0x");
  Serial.print(funIncomingByte, HEX);
  Serial.print(" ");
}

void checkButtons(){
  for(int i=0; i<10; i++){
    if (i<8){
      buttonState[i] = digitalRead(buttonPin[i]);
    }else{
      int rawReading = analogRead(buttonPin[i]);
      int endOfListReading = i-8;
      if(rawReading<2){
        heldTimerForAI[endOfListReading]++;
      }else{
        heldTimerForAI[endOfListReading] = 0;
      }
      if (heldTimerForAI[endOfListReading] > holdTimeForAI[endOfListReading]){
        buttonState[i] = false;
      }else{
        buttonState[i] = true;
      }
    }
    risingEdge[i] = false;
    fallingEdge[i] = false;
    if (buttonState[i] != oldButtonState[i]){
      if (buttonState[i] == false){
        risingEdge[i] = true;
        Serial.print("Press ");
        Serial.println(i);
      }else{
        fallingEdge[i] = true;
//        Serial.print("Release ");
//        Serial.println(i);
      }
      oldButtonState[i] = buttonState[i];
    }
  }
}

void playMode(){
  switch (mode){
    case 0:
      mode00();
      break;
    case 1:
      mode01();
      break;
    case 2:
      mode02();
      break;
    case 10:
      mode10();
      break;
    case 11:
      mode11();
      break;
    default:
      Serial.println("Unknown mode");
      mode = 0;
      break;
  }
}

void mode00(){
      // Write the relay output LOW
      // Calculate and write the millis() time to drop that relay
      // Check if any relays need to drop
  for(int i=0; i<8; i++){
    if((risingEdge[i]) || (MIDIOn[i])){
      digitalWrite(solenoidPin[i], LOW);
      solenoidHolding[i] = millis() + solenoidDuration;
      MIDIOn[i] = 0;
    }
    if(millis() > solenoidHolding[i]){
      digitalWrite(solenoidPin[i], HIGH);
    }
  }
}

void mode01(){
  // If the tempo hasn't been established, start with the default.
  for(int i=0; i<8; i++){
    if((risingEdge[i]) || (MIDIOn[i])){
      MIDIOn[i] = 0;
      int mainBPM = map(i, 0, 7, 40, 200);
//      mainTempo = map(i, 0, 7, 1500, 300);
      mainTempo = 60000 / mainBPM;
      Serial.print("Tempo: ");
//      Serial.print(mainBPM);
//      Serial.print(" BPM\n");
      Serial.print(mainTempo);
      Serial.print(" mS delay\n");
    }
  }
  // Play notes based on prandom number generation between 0 and 7.
  if(millis() % mainTempo < 2){
    delay(2);
    prandomNote = random(0,8);
//      Serial.println(prandomNote);
    digitalWrite(solenoidPin[prandomNote], LOW);
    solenoidHolding[prandomNote] = millis() + solenoidDuration;
  }
  if(millis() > solenoidHolding[prandomNote]){
    digitalWrite(solenoidPin[prandomNote], HIGH);
  }
}

void mode02(){
  // Establish how many notes will be in a group.
  for(int i=0; i<8; i++){
    if((risingEdge[i]) || (MIDIOn[i])){
      MIDIOn[i] = 0;
      noteGroupSize = i+2;
      Serial.print("Grouping: ");
      Serial.print(noteGroupSize);
      Serial.print(" notes\n");
    }
  }
  if ((noteGroupProgression >= noteGroupSize) && (millis() % mainTempo < 2)){
    delay(2);
    noteGroupProgression = 0;
  }
  // Play notes based on prandom number generation between 0 and 7.
  if(millis() % mainTempo < 2){
    noteGroupProgression++;
    delay(2);
    prandomNote = random(0,8);
//      Serial.println(prandomNote);
    digitalWrite(solenoidPin[prandomNote], LOW);
    solenoidHolding[prandomNote] = millis() + solenoidDuration;
  }
  if(millis() > solenoidHolding[prandomNote]){
    digitalWrite(solenoidPin[prandomNote], HIGH);
  }
}

void mode10(){
  // Play a note between the player's by calculating the time
  // between their notes.
  // Do not play the same note.
  for(int i=0; i<8; i++){
    if((risingEdge[i]) || (MIDIOn[i])){
      digitalWrite(solenoidPin[i], LOW);
      solenoidHolding[i] = millis() + solenoidDuration;
      MIDIOn[i] = 0;
      
      lastNoteGap = (millis() - lastNoteTime) / 2;
      nextNoteTime[0] = (millis() + lastNoteGap);
      currentNote = i;
      lastNoteTime = millis();
      Serial.print("Last Note gap: ");
      Serial.println(lastNoteGap);
      Serial.print("Next note time / millis: ");
      Serial.print(nextNoteTime[0]);
      Serial.print(" / ");
      Serial.println(millis());
      Serial.print("Current Note: ");
      Serial.println(currentNote);
    }
    if(millis() > solenoidHolding[i]){
      digitalWrite(solenoidPin[i], HIGH);
    }
  }
  if((millis() - nextNoteTime[0]) < 2){
    delay(2);
    prandomNote = random(0,8);
    while(prandomNote == currentNote){
      prandomNote = random(0,8);
    }
    digitalWrite(solenoidPin[prandomNote], LOW);
    solenoidHolding[prandomNote] = millis() + solenoidDuration;
  }
  if(millis() > solenoidHolding[prandomNote]){
    digitalWrite(solenoidPin[prandomNote], HIGH);
  }
}

void mode11(){
  // Establish how many notes will be in a group.
  if (noteGroupSize == 0){
    for(int i=0; i<8; i++){
//      if(risingEdge[i]){
      if((risingEdge[i]) || (MIDIOn[i])){
        MIDIOn[i] = 0;
        noteGroupSize = i+2;
        Serial.print("Grouping: ");
        Serial.print(noteGroupSize);
        Serial.print(" notes\n");
      }
    }
  }
  // Look for incoming signals from the buttons or MIDI.
  // If there is something there, play the appropriate note and
  // set the timing for the next n notes.
  for(int i=0; i<8; i++){
    if((risingEdge[i]) || (MIDIOn[i])){
      digitalWrite(solenoidPin[i], LOW);
      solenoidHolding[i] = millis() + solenoidDuration;
      MIDIOn[i] = 0;
      currentNote = i;
      for(int k=0; k<8; k++){
        if (k < (noteGroupSize - 1)){
          nextNoteTime[k] = millis() + (k * mainTempo) + mainTempo;
          Serial.print(nextNoteTime[k]);
          Serial.print(", ");
        }else{
          nextNoteTime[k] = 0;
        }
      }
      Serial.print("\n");
    }
    if(millis() > solenoidHolding[i]){
      digitalWrite(solenoidPin[i], HIGH);
    }
  }
  for (int i=0; i<8; i++){
    if((millis() - nextNoteTime[i]) < 2){
      delay(2);
      prandomNote = random(0,8);
      while(prandomNote == currentNote){
        prandomNote = random(0,8);
      }
      digitalWrite(solenoidPin[prandomNote], LOW);
      solenoidHolding[prandomNote] = millis() + solenoidDuration;
    }
    if(millis() > solenoidHolding[prandomNote]){
      digitalWrite(solenoidPin[prandomNote], HIGH);
    }
  }
  if(millis() > solenoidHolding[prandomNote]){
    digitalWrite(solenoidPin[prandomNote], HIGH);
  }
}

void calculateMode(){
  // When the single-player buttons is pressed, make sure,
  // the mode increments and stays in the single-player
  // mode range.
  if(risingEdge[8]){
    mode++;
    if((mode < 0) || (mode > singleModes)){
      mode = 0;
    }
  }
  if(risingEdge[9]){
    mode++;
    if((mode < 10) || (mode > doubleModes)){
      mode = 10;
    }
  }
  // If either mode changing button has been pressed, print the results
  // and do some cleanup.
  if ((risingEdge[8]) || (risingEdge[9])){
    for(int i=0; i<8; i++){
      digitalWrite(solenoidPin[i], HIGH);
    }
    Serial.print("Mode: ");
    Serial.print(mode);
    Serial.print("\n");
  }
}
