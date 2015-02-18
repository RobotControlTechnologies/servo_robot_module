#include <Wire.h>
#include <Multiservo.h>

#define MULTISERVO_PIN_0 0
#define MULTISERVO_PIN_1 1
#define MULTISERVO_PIN_2 2
#define MULTISERVO_PIN_3 3
#define MULTISERVO_PIN_4 4
#define MULTISERVO_PIN_5 5

Multiservo myservos[6];

int q = 0;
byte rxBuf = 0;
char stateMachine = 0, counter = 0;
byte dataBuf[2] = {0};

String input_buffer = "";

void setup(void) {
  Serial.begin(9600);
  Wire.begin();
  myservos[0].attach(MULTISERVO_PIN_0);
  myservos[1].attach(MULTISERVO_PIN_1);
  myservos[2].attach(MULTISERVO_PIN_2);
  myservos[3].attach(MULTISERVO_PIN_3);
  myservos[4].attach(MULTISERVO_PIN_4);
  myservos[5].attach(MULTISERVO_PIN_5);
  
  myservos[0].write(90);
  delay(150);
  myservos[1].write(90);
  delay(150);
  myservos[2].write(90);
  delay(150);
  myservos[3].write(90);
  delay(150);  
  myservos[4].write(90);
  delay(150);  
  myservos[5].write(90);
  delay(150);  
}

void loop(void) {
  q = Serial.available();
  if (q) {
    while(q--) {
      rxBuf = Serial.read(); 
      if (stateMachine == 0) {
        stateMachine = rxBuf == 0xFF ? 1 : 0;
      } else if (stateMachine == 1) {
        dataBuf[counter++] = rxBuf;
        if(counter > 2) {
          stateMachine = 0;
          counter=0;
          
          myservos[dataBuf[0]].write(dataBuf[1]);
          delay(150);
        }
      }
    }
  }
}
