#include <Arduino.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <NeoSWSerial.h>

// automat 8
// up 4
// down 5
// cw 6
// ccw 7

float az=0.0,el=0.0,setAz=0.0,setEl=0.0,oldAz=0.0,oldEl=0.0;
String buffer;
char crlfbuf[80];
bool autom = false, setdisp = false;

LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7);
NeoSWSerial rotserial(3,2);

void setup() {
  Serial.begin(9600);
  rotserial.begin(9600);

  //for(int p = 5; p < 9; p++){
  //  pinMode(p, INPUT_PULLUP);
  //}
  DDRD &= ~240;
  DDRB &= ~1;
  PORTD |= 240;
  PORTB |= 1;


  lcd.begin(16,2);
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.print("DragonRotCtrl");
  lcd.setCursor(0,1);
  lcd.print("By Rob OK9UWU");

  delay(5000);

}

void displayprint(float azimut, float elevace, bool set){
  if(set){
    lcd.setCursor(8,0);
  }
  else{
    lcd.setCursor(0,0);
  }
  lcd.print("Az:");
  lcd.print(String(azimut,1)); 

  if(set){
  lcd.setCursor (8,1);
  }
  else{
    lcd.setCursor(0,1);
  }        

  lcd.print("El:");
  lcd.print(String(elevace,1));
}

void rotgoto(float rotaz, float rotel){ //Send command with requested AZ EL to the rotator
    char setAzStr[6]; 
    char setElStr[6];
    dtostrf(abs(rotaz),5,1,setAzStr);
    dtostrf(abs(rotel),5,1,setElStr);
    rotserial.print("AZ");
    rotserial.print(setAzStr);
    rotserial.print(" EL");
    rotserial.println(setElStr);
}


void loop() {

    while(PINB & 1){
    if(!autom){
      lcd.clear();
      lcd.print("AUTOMATIC");
      autom = true;
    }
    if(Serial.available()){
      rotserial.write(Serial.read());
    }
    if(rotserial.available()){
      Serial.write(rotserial.read());
    } 
  }
    autom = false;

    while(!(PINB & 1)){
      if(!autom){
        lcd.clear();
        lcd.print("MANUAL");
        autom = true;
      }

    static unsigned long timer = 0;
    if(millis() > timer){
      timer = millis() + 200;

      if(!(PIND & 16)){
        setEl++;
      }
      if(!(PIND & 32)){
        setEl--;
      }
      if(!(PIND & 64)){
        setAz++;
      }
      if(!(PIND & 128)){
        setAz--;
      }
    }
    

    if(az != (setAz + az) || el != (setEl + el)){ // wtf but it works, conditions for negative values must be added not to confuse the rotator
    az += setAz;
    el += setEl;
    oldAz = az;
    oldEl = el;
    rotgoto(az,el);
    setAz=0.0,setEl=0.0;
    displayprint(az,el,true);
    setdisp = true;  
    }

    else {
      static unsigned long req = 0;
      if (millis() > req) { //počkaj a zeptaj sa ancáblu kai míří
        req = millis() + 1000;
        rotserial.println("AZ EL");
      }
    }
    static unsigned long disptimeout = 0;
    if(millis() > disptimeout && setdisp){
      disptimeout = millis() + 5000;
      lcd.setCursor(8,0);
      lcd.print("        ");
      lcd.setCursor(8,1);
      lcd.print("        ");
      setdisp = false;
      }


    while(rotserial.available() > 0){ //Parsing AZ EL data sent by the rotator.
      char rx = rotserial.read();
      buffer += rx;

      if((rx == '\n' || rx == '\r')){
        buffer.toCharArray(crlfbuf,40);
        if((buffer.startsWith("AZ")) && (buffer.length() < 26)){
          char strAz[10];
          char strEl[10];
          if(sscanf(buffer.c_str(), "AZ%s EL%s", strAz, strEl) == 2){
            az = strtod(strAz,NULL);
            el = strtod(strEl,NULL);
            displayprint(az,el,false);
          }
        }
        buffer = "";
      } 
    }
  }
  autom = false;
}
