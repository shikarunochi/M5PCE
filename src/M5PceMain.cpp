//    PC Engine emulator M5PCE for M5Stack by @shikarunochi 2021.12-
#ifdef XIAO_GC9107
#include<M5GFX.h>
#include "XIAO_GC9107.h"
#elif defined XIAO_ST7789
#include "XIAO_ST7789.h"
#else
#include <M5Stack.h>
#define LGFX_M5STACK    
#define LGFX_USE_V1 
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>  
#endif
#include "M5pce.h"
#include "pce.h"

static LGFX lcd;  
static LGFX_Sprite sprite(&lcd);

unsigned char *XBuf;
int UPeriod;
int BaseClock;
int vmode=0;
int Debug=0;
char	*pCartName = NULL;

uint8_t needDrawUpdateFlag;
uint8_t nowDrawingFlag;

void graph_updae_thread(void *pvParameters);

BOOL InitMachine(void){

	XBuf = (BYTE *)ps_malloc(sizeof(BYTE) * WIDTH * HEIGHT);
	
	if( !XBuf )
	{
		TRACE("FAILED\n");
		return 0; 
	}
	memset(XBuf,0,sizeof(BYTE) * WIDTH * HEIGHT);

    //UPeriod = 2;
    //BaseClock = 7170000;

  	needDrawUpdateFlag = FALSE;
   	nowDrawingFlag = FALSE;

	lcd.init();

    lcd.setSwapBytes(true); // バイト順変換を有効にする。
	lcd.setColorDepth(8);
    lcd.fillScreen(TFT_BLACK);
#ifdef ARDUINO_XIAO_ESP32S3
	//lcd.setRotation(2);
#endif
	sprite.setColorDepth(8);
	sprite.setSwapBytes(true);
	sprite.createSprite(WIDTH, HEIGHT); 
  	
	xTaskCreatePinnedToCore(graph_updae_thread, "graph_updae_thread", 8192, NULL, 1, NULL, 0);

    return TRUE;

};

void TrashMachine(void){


};

#define	JOY_A	1
#define	JOY_B	2
#define	JOY_SELECT	4
#define	JOY_START	8
#define	JOY_UP	0x10
#define	JOY_DOWN	0x40
#define	JOY_LEFT	0x80
#define	JOY_RIGHT	0x20

#define JOYSTICK_ADDR 0x52

int Joysticks(void){
	int JS = 0;
#ifdef ARDUINO_XIAO_ESP32S3
#else
	M5.update();
	
	if(M5.BtnA.wasReleased()){
		JS = JS | JOY_START;
	}

	if(digitalRead(BUTTON_B_PIN) == 0) {
		JS = JS | JOY_A;
	}
	if(digitalRead(BUTTON_C_PIN) == 0) {
		JS = JS | JOY_B;
	}

	int joyPress = 0;
	int joyX = 0;
	int joyY = 0;

	if(Wire.requestFrom(JOYSTICK_ADDR,3) >= 3){
		if(Wire.available()){joyX = Wire.read();}
		if(Wire.available()){joyY = Wire.read();}
		if(Wire.available()){joyPress = Wire.read();}//Press
		//RIGHT  
		if(joyX < 80){
			JS = JS | JOY_RIGHT;
		} else      
		//LEFT
		if(joyX > 160){
			JS = JS | JOY_LEFT;
		}

		//UP
		if(joyY < 80){
			JS = JS | JOY_UP;
		} else      
		//DOWN
		if(joyY > 160){
			JS = JS | JOY_DOWN;
		}

		if(joyPress == 1){
			JS = JS | JOY_SELECT;
		}
	}
	if(JS > 0){ //Faces GameBoy 以外の入力があればそれを返す
		return JS;
	}
  	//if(digitalRead(pin_int_face) == LOW) {//状態変化の割り込みが無くても、現在の状態をチェック
		// If yes, request 1 byte from the panel
		Wire.requestFrom(GAMEBOY_I2C_ADDRESS, (uint8_t)1);

		// Check if data on the I2C is available
		if(Wire.available()) {
			// Receive one byte as character
			uint8_t key_val = Wire.read();
			if(key_val != 0x00) {
				if(key_val == 0xff){
					return 0;
				}
				//Serial.printf("%x\n",key_val);
				//押された箇所のビットが 0 になります
				if((key_val & ~GAMEBOY_KEY_SELECT & 0xFF) == 0){
					JS = JS | JOY_SELECT;
				}  
				//if((key_val & 0x80) == 0){
				if((key_val & ~GAMEBOY_KEY_START & 0xFF) == 0){
					JS = JS | JOY_START;
				}  
				if((key_val & ~GAMEBOY_KEY_A & 0xFF) == 0){
					JS = JS | JOY_A;
				}  
				if((key_val & ~GAMEBOY_KEY_B & 0xFF) == 0){
					JS = JS | JOY_B;
				}  
				if((key_val & ~GAMEBOY_KEY_UP & 0xFF) == 0){
					JS = JS | JOY_UP;
				}  
				if((key_val & ~GAMEBOY_KEY_DOWN & 0xFF) == 0){
					JS = JS | JOY_DOWN;
				}  
				if((key_val & ~GAMEBOY_KEY_LEFT & 0xFF) == 0){
					JS = JS | JOY_LEFT;
				}  
				if((key_val & ~GAMEBOY_KEY_RIGHT & 0xFF) == 0){
					JS = JS | JOY_RIGHT;
				}  
			}
		}
	//}
#endif
	return JS;
}

uint8_t tempBuffer[WIDTH];
void PutImage(int X,int Y,int W,int H){
	unsigned char * lpSrc = XBuf + Y * WIDTH + X; 
	for(int yIndex = 0;yIndex < H;yIndex++){
		//GRB 333 を RGB332 に変換。もともと 8ビットなので GRB333 ではないかも？
		//GRB 323 → RGB 233 っぽい？よくわからない…。
		for(int index = 0;index < W;index++){
			unsigned char srcColor = *(lpSrc + index);
			//tempBuffer[index] = (srcColor >> 2 & 0b1110000)|(srcColor << 10 & 0b11100000000000)|(srcColor << 2 & 0b00000000011100);
			//tempBuffer[index] = (srcColor >> 4 & 0b00011100)|(srcColor << 2 & 0b11100000)|(srcColor >> 1 & 0b00000011);
			//tempBuffer[index] = (srcColor >> 3 & 0b000111000)|(srcColor << 3 & 0b011000000)|(srcColor & 0b000000111);
			tempBuffer[index] = (srcColor >> 3 & 0b000111000)|(srcColor << 3 & 0b011000000)|(srcColor & 0b000000111);
		}

		sprite.pushImage(X - 54 , Y + yIndex, W, 1, tempBuffer);
		lpSrc = lpSrc + WIDTH;
	}
	needDrawUpdateFlag = TRUE;
}

void graphUpdate(){
	needDrawUpdateFlag = TRUE;
}
BOOL checkNeedDrawUpdate(){
	return needDrawUpdateFlag;
}

 void graph_updae_thread(void *pvParameters){
   while(1){
     if(needDrawUpdateFlag == TRUE){
       nowDrawingFlag = TRUE;
#ifdef XIAO_GC9107
	sprite.pushRotateZoomWithAA(83,60,0,0.39,0.39);
#elif defined XIAO_ST7789
	sprite.pushRotateZoomWithAA(150,100,0,0.666,0.666);
	
#else
	sprite.pushSprite(0,0);
#endif
       nowDrawingFlag = FALSE;
       needDrawUpdateFlag = FALSE;
     }
     delay(10);
   }
 }
