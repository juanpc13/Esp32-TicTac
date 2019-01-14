class ButtonTool {
  private:
    uint8_t _pinButton;
    boolean permit = false;
    boolean lastPush = HIGH;
    boolean singlePush = true;
    int _minPressTimmer = 10;
    int _longPressTimmer = 250;
    unsigned long pressedTime = 0;

  public:
    ButtonTool() {
    }

    ButtonTool(uint8_t pinButton) {
      _pinButton = pinButton;
    }

    void begin() {
      pinMode(_pinButton, INPUT_PULLUP);
    }

    void update() {
      permit = (lastPush == LOW && digitalRead(_pinButton) == HIGH);
      lastPush = digitalRead(_pinButton);

      if (digitalRead(_pinButton) == HIGH && permit == false) {
        pressedTime = millis();
        singlePush = true;
      }
    }

    boolean isPressed() {
      unsigned long delta = millis() - pressedTime;
      if (delta >= _minPressTimmer) {
        if (singlePush) {
          return true;
          singlePush = false;
        }
      }
      return false;
    }

    void setLongPressTimmer(int longPressTimmer) {
      _longPressTimmer = longPressTimmer;
    }

    boolean longPress() {
      if (millis() - pressedTime > _longPressTimmer) {
        return true;
      }
      return false;
    }

    boolean longPress(int customPressTimmer) {
      if (millis() - pressedTime > customPressTimmer) {
        if (permit) {
          return true;
        }        
      }
      return false;
    }

    uint8_t doubleSwitch() {
      uint8_t s = 0;
      unsigned long delta = millis() - pressedTime;
      if (delta >= _minPressTimmer && delta <= _longPressTimmer) {
        if (permit) {
          s = 1;
        }
      } else if (delta > _longPressTimmer) {
        if (permit) {
          s = 2;
        }
      }
      return s;
    }

};
