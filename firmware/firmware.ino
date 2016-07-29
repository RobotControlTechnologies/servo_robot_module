#include <Wire.h>
#include <Multiservo.h>

#define SERVO_COUNT 6

#define MULTISERVO_PIN_0 0
#define MULTISERVO_PIN_1 1
#define MULTISERVO_PIN_2 2
#define MULTISERVO_PIN_3 3
#define MULTISERVO_PIN_4 4
#define MULTISERVO_PIN_5 5

int count_bytes = 0;
byte rxBuf = 0;
int stateMachine = 0, counter = 0;
byte dataBuf[2] = {0};

struct ServoPos
{
  Multiservo myservo;
  byte x0;
  void moveServo(byte x1);
};

void ServoPos::moveServo(byte x1){
  int delta_x = x1-x0;
  int n = abs(delta_x);
  int step = delta_x/n;

  for (int i = 0; i < n; ++i)
  {
    x0 += step;
    myservo.write(x0);
    delay(15);
  }
};

ServoPos servo_positions[SERVO_COUNT];

String input_buffer = "";

void setup(void) {
  Serial.begin(9600);
  Wire.begin();

  servo_positions[0].myservo.attach(MULTISERVO_PIN_0);
  servo_positions[1].myservo.attach(MULTISERVO_PIN_1);
  servo_positions[2].myservo.attach(MULTISERVO_PIN_2);
  servo_positions[3].myservo.attach(MULTISERVO_PIN_3);
  servo_positions[4].myservo.attach(MULTISERVO_PIN_4);
  servo_positions[5].myservo.attach(MULTISERVO_PIN_5);
  
  for (int i = 0; i < SERVO_COUNT; ++i)
  {
    servo_positions[i].x0 = 0;
  }
}

void loop(void) {
  count_bytes = Serial.available();

  if (count_bytes){
    while(count_bytes--) {
      rxBuf = Serial.read();
      Serial.print("rxBuf = " + String(rxBuf) + "\n");
      if (rxBuf == 254) 
      {
        delay(5);
        byte serv = Serial.read();
        delay(5);
        byte x0 = Serial.read();
        servo_positions[serv - 1].x0 = x0;
        count_bytes = Serial.available();
        continue;
      }
      if (stateMachine == 0) {
        stateMachine = rxBuf == 255 ? 1 : 0;
        Serial.print("stateMachine = " + String(stateMachine) + "\n");
      } else if (stateMachine == 1) {
        dataBuf[counter++] = rxBuf;
        if (counter > 1) {
          stateMachine = 0;
          counter = 0;
          
          Serial.print("servo = " + String(dataBuf[0]) + "; value = " + String(dataBuf[1]) + ";\n");
          
          servo_positions[dataBuf[0]-1].moveServo(dataBuf[1]);
        }
      }
    }
  }
}
