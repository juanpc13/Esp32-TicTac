class ButtonTool {
  private:
    uint8_t _pinButton;
    boolean singlePush = true;
    int _longPressTimmer = 750;
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
      if (digitalRead(_pinButton) != LOW) {
        pressedTime = millis();
        singlePush = true;
      }
    }

    boolean isPressed() {
      unsigned long delta = millis() - pressedTime;
      if (delta >= 100) {
        if (singlePush) {
          return true;
          singlePush = false;
        }
      }
      return false;
    }

    void setLongPressTimmer(int longPressTimmer){
      _longPressTimmer = longPressTimmer;
    }

    boolean longPress() {
      if (millis() - pressedTime > _longPressTimmer) {
        return true;
      }
      return false;
    }

};
