
char strResult[10001];

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.print (file.name());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.print(file.size());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}

const char*readFile(fs::FS &fs, const char * path){
    
    strcpy(strResult, "");
    
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return "";
    }

    if (file.size() > sizeof(strResult)-1){
      Serial.println("Failed, file bigger than buffer");
      return "";
    }
    
    int i = 0;
    while(file.available()){
      strResult[i] = (char) file.read();
      i++;
    }
    strResult[i] = '\0';

    Serial.print("strResult: ");
    Serial.println(strResult);
    
    file.close();
    return strResult;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void resetConfigFiles(){
  SPIFFS.format();
  
  writeFile(SPIFFS, "/Wifi.config", "{\"SSID\":\"\",\"Password\":\"\"}");
  writeFile(SPIFFS, "/MQTT.config", "{\"MQTT_Server\":\"saorafael.sytes.net\", \"MQTT_Port\":\"1883\"}");
  writeFile(SPIFFS, "/SlavesID.config", "{\"SlavesID\":\"\", \"nSlaves\":0}");
  writeFile(SPIFFS, "/ClientID.config", "{\"ClientID\":\"10\"}");
}

void startFileManager(){
  
  while(!SPIFFS.begin()){
      Serial.println("Card Mount Failed");
      yield();
      display.fillScreen(ILI9341_BLACK);
      yield();
      display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("SPIFFS Failed");
      resetConfigFiles();
      delay(100);
  }

  if (!SPIFFS.exists("/Wifi.config") || !SPIFFS.exists("/MQTT.config") 
                  || !SPIFFS.exists("/SlavesID.config") || !SPIFFS.exists("/ClientID.config")){
    resetConfigFiles();
  }
  
}
