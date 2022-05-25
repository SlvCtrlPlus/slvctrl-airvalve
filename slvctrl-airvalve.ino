#include <Servo.h>
#include <math.h>
#include <SerialCommands.h>

const String DEVICE_TYPE = "air_valve";
const int FM_VERSION = 10000; // 1.00.00

const int ANGLE_OPEN = 10;
const int ANGLE_CLOSED = 85;

Servo servo;

char serial_command_buffer[32];
SerialCommands serialCommands(&Serial, serial_command_buffer, sizeof(serial_command_buffer), "\n", " ");

void commandIntroduce(SerialCommands* sender);
void commandStress(SerialCommands* sender);
void commandFlowGet(SerialCommands* sender);
void commandFlowSet(SerialCommands* sender);

void setup() {
    servo.attach(8);
    servo.write(ANGLE_OPEN); // set to open
    
    Serial.begin(9600);

    // Add commands
    serialCommands.SetDefaultHandler(commandUnrecognized);
    serialCommands.AddCommand(new SerialCommand("introduce", commandIntroduce));
    serialCommands.AddCommand(new SerialCommand("stress", commandStress));
    serialCommands.AddCommand(new SerialCommand("flow-get", commandFlowGet));
    serialCommands.AddCommand(new SerialCommand("flow-set", commandFlowSet));
    serialCommands.AddCommand(new SerialCommand("angle-get", commandAngleGet));
    serialCommands.AddCommand(new SerialCommand("angle-set", commandAngleSet));

    serialCommands.GetSerial()->write(0x07);
}

void loop() {
    serialCommands.ReadSerial();
}

void commandIntroduce(SerialCommands* sender) {
    sender->GetSerial()->println(DEVICE_TYPE + "," + FM_VERSION);
}

void commandFlowSet(SerialCommands* sender) {
    char* percentageStr = sender->Next();
    char* fadeStr = sender->Next();

    if (percentageStr == NULL) {
        sender->GetSerial()->println("Percentage parameter missing");
        return;
    }
  
    if (fadeStr == NULL) {
        sender->GetSerial()->println("Fade parameter missing");
        return;
    }
  
    int percentage = atoi(percentageStr);
    int fade = atoi(fadeStr);
  
    setFlow(percentage, fade);

    sender->GetSerial()->println(strprintf("flow-set,%d", getFlow()));
}

void commandFlowGet(SerialCommands* sender) {
    sender->GetSerial()->println(strprintf("flow-get,%d", getFlow()));
}

void commandAngleGet(SerialCommands* sender) {
    sender->GetSerial()->println(strprintf("angle-get,%d", servo.read()));
}

void commandAngleSet(SerialCommands* sender) {
    char* angleStr = sender->Next();
    int angle = atoi(angleStr);

    servo.write(angle);
    
    sender->GetSerial()->println(strprintf("angle-set,%d", servo.read()));
}

void commandStress(SerialCommands* sender) {
  char* roundsStr = sender->Next();

  if (roundsStr == NULL) {
      sender->GetSerial()->println("Rounds parameter missing");
      return;
  }
  
  int rounds = atoi(roundsStr);
  
  for (int i = 0; i < rounds; i++) {
      servo.write(ANGLE_CLOSED);
      sender->GetSerial()->println(strprintf("Stress round %d: closed", i));
      delay(1000);

      servo.write(ANGLE_OPEN);
      sender->GetSerial()->println(strprintf("Stress round %d: opened", i));
      delay(1000);
  }
}

void commandUnrecognized(SerialCommands* sender, const char* cmd)
{
    sender->GetSerial()->println(strprintf("Unrecognized command [%s]", cmd));
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
}

int getFlow() {
    return 100 - round((servo.read() - ANGLE_OPEN) * 100 / (ANGLE_CLOSED-ANGLE_OPEN));
}
