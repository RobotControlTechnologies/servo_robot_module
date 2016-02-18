#include <Wire.h>
#include <Multiservo.h>

#define MULTISERVO_PIN_0 0
#define MULTISERVO_PIN_1 1
#define MULTISERVO_PIN_2 2
#define MULTISERVO_PIN_3 3
#define MULTISERVO_PIN_4 4
#define MULTISERVO_PIN_5 5

Multiservo myservos[6];

int count_bytes = 0;
byte rxBuf = 0;
int stateMachine = 0, counter = 0;
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
  
}

void loop(void) {
  count_bytes = Serial.available();

  if (count_bytes){
    while(count_bytes--) {
      rxBuf = Serial.read();
      Serial.print("rxBuf = " + String(rxBuf) + "\n");
      if (stateMachine == 0) {
        stateMachine = rxBuf == 255 ? 1 : 0;
        Serial.print("stateMachine = " + String(stateMachine) + "\n");
      } else if (stateMachine == 1) {
        dataBuf[counter++] = rxBuf;
        if (counter > 1) {
          stateMachine = 0;
          counter = 0;
          
          Serial.print("servo = " + String(dataBuf[0]) + "; value = " + String(dataBuf[1]) + ";\n");
          
          myservos[dataBuf[0] -1].write(dataBuf[1]);
          delay(150);
        }
      }
    }
  }
}
