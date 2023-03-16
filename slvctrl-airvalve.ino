#include <Servo.h>
#include <math.h>
#include <SerialCommands.h>

const char* DEVICE_TYPE = "air_valve";
const int FM_VERSION = 10000; // 1.00.00
const int PROTOCOL_VERSION = 10000; // 1.00.00

const int SERVO_PWM_PIN = 1;
const int ANGLE_OPEN = 10;
const int ANGLE_CLOSED = 85;

Servo servo;
int currentFlow = 0;

char serial_command_buffer[32];
SerialCommands serialCommands(&Serial, serial_command_buffer, sizeof(serial_command_buffer), "\n", " ");

void commandIntroduce(SerialCommands* sender);
void commandStress(SerialCommands* sender);
void commandFlowGet(SerialCommands* sender);
void commandFlowSet(SerialCommands* sender);

void setup() {
    servo.attach(SERVO_PWM_PIN);
    servo.write(ANGLE_OPEN); // set to open
    currentFlow = 100;
    
    Serial.begin(9600);

    // Add commands
    serialCommands.SetDefaultHandler(commandUnrecognized);
    serialCommands.AddCommand(new SerialCommand("introduce", commandIntroduce));
    serialCommands.AddCommand(new SerialCommand("status", commandStatus));
    serialCommands.AddCommand(new SerialCommand("attributes", commandAttributes));
    serialCommands.AddCommand(new SerialCommand("stress", commandStress));
    serialCommands.AddCommand(new SerialCommand("get-flow", commandGetFlow));
    serialCommands.AddCommand(new SerialCommand("set-flow", commandSetFlow));
    serialCommands.AddCommand(new SerialCommand("angle-get", commandAngleGet));
    serialCommands.AddCommand(new SerialCommand("angle-set", commandAngleSet));

    serialCommands.GetSerial()->write(0x07);
}

void loop() {
    serialCommands.ReadSerial();
}

void commandIntroduce(SerialCommands* sender) {
    serial_printf(sender->GetSerial(), "introduce;%s,%d,%d\n", DEVICE_TYPE, FM_VERSION, PROTOCOL_VERSION);
}

void commandStatus(SerialCommands* sender) {
    serial_printf(sender->GetSerial(), "status;flow:%d\n", currentFlow);
}

void commandAttributes(SerialCommands* sender) {
    serial_printf(sender->GetSerial(), "attributes;flow:rw[0-100]\n");
}

void commandSetFlow(SerialCommands* sender) {
    char* percentageStr = sender->Next();

    if (percentageStr == NULL) {
        sender->GetSerial()->println("set-flow;;status:failed,reason:percentage_param_missing\n");
        return;
    }
  
    int percentage = atoi(percentageStr);
  
    setFlow(percentage, 0);

    serial_printf(sender->GetSerial(), "set-flow;%d;status:successful\n", getFlow());
}

void commandGetFlow(SerialCommands* sender) {
    serial_printf(sender->GetSerial(), "get-flow;%d\n", getFlow());
}

void commandAngleGet(SerialCommands* sender) {
    serial_printf(sender->GetSerial(), "angle-get,%d\n", servo.read());
}

void commandAngleSet(SerialCommands* sender) {
    char* angleStr = sender->Next();
    int angle = atoi(angleStr);

    servo.write(angle);
    
    serial_printf(sender->GetSerial(), "angle-set,%d\n", servo.read());
}

void commandStress(SerialCommands* sender) {
  char* roundsStr = sender->Next();

  if (roundsStr == NULL) {
      sender->GetSerial()->println("Rounds parameter missing\n");
      return;
  }
  
  int rounds = atoi(roundsStr);
  
  for (int i = 0; i < rounds; i++) {
      servo.write(ANGLE_CLOSED);
      serial_printf(sender->GetSerial(), "Stress round %d: closed\n", i);
      delay(1000);

      servo.write(ANGLE_OPEN);
      serial_printf(sender->GetSerial(), "Stress round %d: opened\n", i);
      delay(1000);
  }
}

void commandUnrecognized(SerialCommands* sender, const char* cmd)
{
    serial_printf(sender->GetSerial(), "Unrecognized command [%s]\n", cmd);
}

void setFlow(int percentage, int fade) {
    int currentAngle = servo.read();
    int newAngle = ANGLE_OPEN + ((int) round((100 - percentage) * (ANGLE_CLOSED-ANGLE_OPEN) * 0.01));
    int diff = newAngle-currentAngle;

    if (newAngle == currentAngle) {
        // Same angle, don't do anything
        return;
    }

    if (fade > 0) {
        int diffAbs = abs(diff);
        int waitPerStep = round(fade/diffAbs);
        int stepDirection = (newAngle > currentAngle) ? +1 : -1;
    
        while (currentAngle != newAngle) {
            currentAngle += stepDirection;
            servo.write(currentAngle);
            delay(waitPerStep);
        }
    } else {
        servo.write(newAngle);
        currentAngle = newAngle;
    }

    currentFlow = percentage;
}

int getFlow() {
    return 100 - round((servo.read() - ANGLE_OPEN) * 100 / (ANGLE_CLOSED-ANGLE_OPEN));
}
