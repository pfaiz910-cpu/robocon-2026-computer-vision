#include <Arduino.h>

// --- CAMERA SWITCH STAGE MACHINE ---
// Stage 0 (STAGE_WEAPON):  Detecting weapon (fist/palm/spear) on Orbbec
// Stage 1 (STAGE_ROBOTS):  Detecting robots (R1, R1 Blue, R2 Real, R2 Real Blue,
//                          R2 Fake, R2 Fake Blue) on Orbbec
// Stage 2 (STAGE_DONE):    Both stages finished -> camera switched to webcam
enum CameraStage { STAGE_WEAPON, STAGE_ROBOTS, STAGE_DONE };
CameraStage stage = STAGE_WEAPON;

bool weaponSeen = false;
bool robotSeen = false;
unsigned long lastWeaponSeenTime = 0;
unsigned long lastRobotSeenTime = 0;
const unsigned long LOST_TIMEOUT = 1000; // ms with no detection before advancing stage

void setup() {
  Serial.begin(115200); // Matches Python
  delay(1000);
  Serial.println("--- ROBOCON SYSTEM ONLINE ---");
}

void loop() {
  // 1. CLEAR BUFFER & REQUEST DATA
  while (Serial.available() > 0) Serial.read();
  Serial.println("1"); // Send request pulse

  // 2. WAIT FOR RESPONSE (500ms timeout)
  unsigned long start = millis();
  while (millis() - start < 500) {
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');

      if (data.indexOf('<') != -1 && data.indexOf('>') != -1) {
        data.replace("<", ""); data.replace(">", "");
        int c1 = data.indexOf(',');
        int c2 = data.lastIndexOf(',');

        if (c1 != -1 && c2 != -1) {
          int id = data.substring(0, c1).toInt();
          int x = data.substring(c1 + 1, c2).toInt();
          int y = data.substring(c2 + 1).toInt();

          // 3. DIAGNOSTIC OUTPUT & LOGIC (unchanged behavior)
          Serial.print("ID: "); Serial.print(id);
          Serial.print(" | X: "); Serial.print(x);
          Serial.print(" | Y: "); Serial.print(y);

          if ((id >= 2 && id <= 6) || (id >= 9 && id <= 11)) {
             Serial.println("  --> [LOCKED]");
             // lockOnMechanism(x, y);
          }
          else if (id == 7 || id == 8 || id == 15 || id == 16) {
             Serial.println("  --> [DANGER: FAKE/R1] CHANGING POSITION!");
             // changePosition();
          }
          else {
             Serial.println("  --> [SEARCHING]");
          }

          // 4. STAGE TRACKING (separate from diagnostic message above)
          // Weapon IDs = 2 (fist), 3 (palm), 4 (spear)
          bool isWeapon = (id == 2 || id == 3 || id == 4);
          // Robot IDs = 5 (R2 Real), 6 (R2 Real Blue), 7 (R2 Fake), 8 (R2 Fake Blue),
          //             15 (R1), 16 (R1 Blue)
          bool isRobot  = (id == 5 || id == 6 || id == 7 || id == 8 || id == 15 || id == 16);

          if (stage == STAGE_WEAPON && isWeapon) {
            weaponSeen = true;
            lastWeaponSeenTime = millis();
          }
          else if (stage == STAGE_ROBOTS && isRobot) {
            robotSeen = true;
            lastRobotSeenTime = millis();
          }
          break;
        }
      }
    }

    // Check for stage timeouts and advance stages
    if (stage == STAGE_WEAPON && weaponSeen && (millis() - lastWeaponSeenTime) > LOST_TIMEOUT) {
      stage = STAGE_ROBOTS;
      weaponSeen = false;
      Serial.println("--- STAGE: ROBOTS ---");
    }
    else if (stage == STAGE_ROBOTS && robotSeen && (millis() - lastRobotSeenTime) > LOST_TIMEOUT) {
      stage = STAGE_DONE;
      robotSeen = false;
      Serial.println("--- STAGE: DONE (switch to webcam) ---");
    }
  }
}
