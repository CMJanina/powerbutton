#include <Matter.h>
#include <atomic>

constexpr uint8_t pins[] = {3, 4}; // Power, reset.
MatterOnOffPlugin buttons[2];
// Matter callbacks run on a different task from loop().
std::atomic<uint32_t> requested{0};

void setup() {
  Serial.begin(115200);
  for (unsigned i = 0; i < 2; ++i) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
    if (!buttons[i].begin(false)) {
      Serial.println("Failed to create Matter endpoint");
      while (true)
        delay(1000);
    }
  }

  for (unsigned i = 0; i < 2; ++i) {
    buttons[i].onChange([i](bool on) {
      if (on)
        requested.fetch_or(1U << i);
      return true;
    });
  }
  Matter.begin();
  matterWaitUntilReady();
}

void loop() {
  static bool active[2] = {};
  static uint32_t started[2] = {};
  const uint32_t presses = requested.exchange(0);
  const uint32_t now = millis();

  for (unsigned i = 0; i < 2; ++i) {
    if ((presses & (1U << i)) && !active[i]) {
      active[i] = true;
      started[i] = now;
      digitalWrite(pins[i], HIGH);
    }
    if (active[i] && uint32_t(now - started[i]) >= 250) {
      digitalWrite(pins[i], LOW);
      // Retry a failed state report with the physical output already released.
      active[i] = !buttons[i].setOnOff(false);
    }
  }
  delay(1);
}
