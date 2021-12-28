//    PC Engine emulator M5PCE for M5Stack by @shikarunochi 2021.12-
#include <M5Stack.h>
#define LGFX_M5STACK    
#define LGFX_USE_V1 
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>  
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
	   sprite.pushSprite(0,0);
       nowDrawingFlag = FALSE;
       needDrawUpdateFlag = FALSE;
     }
     delay(10);
   }
 }
