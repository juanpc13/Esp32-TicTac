#include "FS.h"

class utilsSPIFFS {
  private:
  public:

    bool begin(bool format) {
      if (format) {
        SPIFFS.format();
      }
      if (!SPIFFS.begin()) {
        Serial.println("SPIFFS Mount Failed");
        return false;
      }
      return true;
    }

    bool writeFile(String path, String message) {
      File file = SPIFFS.open(path, "w");
      if (!file) {
        Serial.println("- failed to open file for writing");
        file.close();
        return false;
      }
      if (file.print(message)) {
        file.close();
        return true;
      } else {
        Serial.println("- frite failed");
        file.close();
        return false;
      }
    }

    String readFile(String path) {
      String dataText = "";
      File file = SPIFFS.open(path, "r");
      if (!file) {
        Serial.println("- failed to open file for reading");
        file.close();
        return dataText;
      }
      //Storing in dataText
      while (file.available()) {
        char c = file.read();
        dataText += c;
      }
      file.close();
      return dataText;
    }

    bool appendFile(String path, String message) {
      File file = SPIFFS.open(path, "a");
      if (!file) {
        Serial.println("- failed to open file for appending");
        file.close();
        return false;
      }
      if (file.println(message)) {
        file.close();
        return true;
      } else {
        Serial.println("- append failed");
        file.close();
        return false;
      }
    }


    bool renameFile(String path1, String path2) {
      if (SPIFFS.rename(path1, path2)) {
        return true;
      } else {
        Serial.println("- rename failed");
        return false;
      }
    }


    bool deleteFile(String path) {
      if (SPIFFS.remove(path)) {
        Serial.println("- file deleted");
        return true;
      } else {
        Serial.println("- delete failed");
        return false;
      }
    }

    bool fileExits(String path) {
      return SPIFFS.exists(path);
    }

};
