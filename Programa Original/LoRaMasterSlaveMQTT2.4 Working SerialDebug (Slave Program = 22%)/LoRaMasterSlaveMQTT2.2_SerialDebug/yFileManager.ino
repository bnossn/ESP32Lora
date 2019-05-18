#ifdef MASTER

char strResult[10001];

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    sprintf (Sbuffer, "Listing directory: %s\n", dirname);
    Serial_Println(bDebug, Sbuffer);

    File root = fs.open(dirname);
    if(!root){
        Serial_Println(bDebug, "Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial_Println(bDebug, "Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial_Print(bDebug, "  DIR : ");
            Serial_Print(bDebug,file.name());
            
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            sprintf (Sbuffer, "  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
            Serial_Println(bDebug, Sbuffer);
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial_Print(bDebug, "  FILE: ");
            Serial_Print(bDebug, file.name());
            Serial_Print(bDebug, "  SIZE: ");
            sprintf (Sbuffer, "%lu", file.size());
            Serial_Println(bDebug, Sbuffer);
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            sprintf (Sbuffer, "  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
            Serial_Println(bDebug, Sbuffer);
        }
        file = root.openNextFile();
    }
}

const char*readFile(fs::FS &fs, const char * path){
    
    strcpy(strResult, "");
    
    sprintf (Sbuffer,"Reading file: %s\n", path);
    Serial_Println(bDebug, Sbuffer);

    File file = fs.open(path);
    if(!file){
        Serial_Println(bDebug, "Failed to open file for reading");
        return "";
    }

    if (file.size() > sizeof(strResult)-1){
      Serial_Println(bDebug, "Failed, file bigger than buffer");
      return "";
    }
    
    int i = 0;
    while(file.available()){
      strResult[i] = (char) file.read();
      i++;
    }
    strResult[i] = '\0';

    Serial_Print(bDebug, "strResult: ");
    Serial_Println(bDebug, strResult);
    
    file.close();
    return strResult;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    sprintf (Sbuffer,"Writing file: %s\n", path);
    Serial_Println(bDebug, Sbuffer);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial_Println(bDebug, "Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial_Println(bDebug, "File written");
    } else {
        Serial_Println(bDebug, "Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    sprintf (Sbuffer, "Appending to file: %s\n", path);
    Serial_Println(bDebug, Sbuffer);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial_Println(bDebug, "Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial_Println(bDebug, "Message appended");
    } else {
        Serial_Println(bDebug, "Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    sprintf (Sbuffer, "Renaming file %s to %s\n", path1, path2);
    Serial_Println(bDebug, Sbuffer);
    if (fs.rename(path1, path2)) {
        Serial_Println(bDebug, "File renamed");
    } else {
        Serial_Println(bDebug, "Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    sprintf (Sbuffer, "Deleting file: %s\n", path);
    Serial_Println(bDebug, Sbuffer);
    if(fs.remove(path)){
        Serial_Println(bDebug, "File deleted");
    } else {
        Serial_Println(bDebug, "Delete failed");
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
      Serial_Println(bDebug, "Card Mount Failed");
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

#endif
