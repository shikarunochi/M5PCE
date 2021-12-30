//    PC Engine emulator M5PCE for M5Stack by @shikarunochi 2021.12-
#include <M5Stack.h>
#include "M5Pce.h"

//https://github.com/tobozo/M5Stack-SD-Updater/blob/master/examples/M5Stack-SD-Menu/M5Stack-SD-Menu.ino
void sortList(String fileList[], int fileListCount) {
  bool swapped;
  String temp;
  String name1, name2;
  do {
    swapped = false;
    for (int i = 0; i < fileListCount - 1; i++ ) {
      name1 = fileList[i];
      name1.toUpperCase();
      name2 = fileList[i + 1];
      name2.toUpperCase();
      if (name1.compareTo(name2) > 0) {
        temp = fileList[i];
        fileList[i] = fileList[i + 1];
        fileList[i + 1] = temp;
        swapped = true;
      }
    }
  } while (swapped);
}

String pceSelect(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  File fileRoot;
  String fileList[MAX_FILES];
  String fileDir = "/pceROM"; 
  fileRoot = SD.open(fileDir);
  int fileListCount = 0;
  while (1)
  {
    File entry = fileRoot.openNextFile();
    if (!entry)
    { // no more files
      break;
    }
    //ファイルのみ取得
    if (!entry.isDirectory())
    {
      String fullFileName = entry.name();
      String fileName = fullFileName.substring(fullFileName.lastIndexOf("/") + 1);
      String ext = fileName.substring(fileName.lastIndexOf(".") + 1);
      ext.toUpperCase();
      if(ext.equals("PCE")==false){
        continue;
      }

      fileList[fileListCount] = fileName;
      fileListCount++;
      Serial.println(fileName);
    }
    entry.close();
  }
  fileRoot.close();

  if(fileListCount == 0){
      M5.Lcd.println("NO PCE FILE [SD:/pceROM/]");
      return "";
  }
  int startIndex = 0;
  int endIndex = startIndex + 10;
  if (endIndex > fileListCount)
  {
    endIndex = fileListCount;
  }

  sortList(fileList, fileListCount);

  bool needRedraw = true;
  int selectIndex = 0;
  int preStartIndex = 0;
  while (true)
  {
    if (needRedraw == true)
    {
      M5.Lcd.setCursor(0, 0);
      startIndex = selectIndex - 5;
      if (startIndex < 0)
      {
        startIndex = 0;
      }
      endIndex = startIndex + 12;
      if (endIndex > fileListCount)
      {
        endIndex = fileListCount;
        startIndex = endIndex - 12;
        if (startIndex < 0) {
          startIndex = 0;
        }
      }
      if (preStartIndex != startIndex) {
        //スクロールで全画面書き換え
        M5.Lcd.fillScreen(BLACK);
        preStartIndex = startIndex;
      }
      int y = 0;
      for (int index = startIndex; index < endIndex; index++)
      {

        if (index == selectIndex)
        {
           M5.Lcd.setTextColor(TFT_GREEN);
        }
        else
        {
          M5.Lcd.setTextColor(TFT_WHITE);
        }
        M5.Lcd.println(fileList[index].substring(0, 26));
        y++;
      }
      M5.Lcd.setTextColor(TFT_WHITE);
      needRedraw = false;
    }
    M5.update();
    if (M5.BtnB.wasReleased())
    {
        String fileName = fileList[selectIndex];
        String ext = fileName.substring(fileName.lastIndexOf(".") + 1);
        ext.toUpperCase();
        
        Serial.print("select:");
        Serial.println(fileName);
        delay(10);
        M5.Lcd.fillScreen(TFT_BLACK);
        return "/pceROM/" + fileName;
    }else if (M5.BtnA.wasReleased()){
      selectIndex--;
      if (selectIndex < 0)
      {
        selectIndex = fileListCount -1;
      }
      needRedraw = true;
    }else if (M5.BtnC.wasReleased()){
      selectIndex++;
      if (selectIndex >= fileListCount)
      {
        selectIndex = 0;
      }
      needRedraw = true;
    }
    
    if(digitalRead(GAMEBOY_INT_PIN) == LOW) {
      Wire.requestFrom(GAMEBOY_I2C_ADDRESS, (uint8_t)1);
      if(Wire.available()) {
        // Receive one byte as character
        uint8_t key_val = Wire.read();
        //Serial.printf("%x\n",key_val);
        switch(key_val){
          case GAMEBOY_KEY_UP:
            selectIndex--;
            if (selectIndex < 0)
            {
              selectIndex = fileListCount -1;
            }
            needRedraw = true;
            break;
          case GAMEBOY_KEY_DOWN:
            selectIndex++;
            if (selectIndex >= fileListCount)
            {
              selectIndex = 0;
            }
            needRedraw = true;
            break;
          case GAMEBOY_KEY_START:
          case GAMEBOY_KEY_SELECT:
          case GAMEBOY_KEY_A:
          case GAMEBOY_KEY_B:      
            String fileName = fileList[selectIndex];
            String ext = fileName.substring(fileName.lastIndexOf(".") + 1);
            ext.toUpperCase();
            Serial.print("select:");
            Serial.println(fileName);
            delay(10);
            M5.Lcd.fillScreen(TFT_BLACK);
            return "/pceROM/" + fileName;
        }
      }
    }
    delay(100);
  }
}
