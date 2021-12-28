// pce.h
//    Modified for M5Stack by @shikarunochi 2021.12-
#define	WIDTH	(360+64)
#define	HEIGHT	256

#include "M6502.h"

typedef struct tagIO {
	pair VDC[32];
//	byte VDC_ratch[32];
	pair VCE[0x200];
	pair vce_reg;
	/* VDC */
	word_s vdc_inc,vdc_raster_count;
	byte vdc_reg,vdc_status,vdc_ratch,vce_ratch;
//	byte vdc_iswrite_h;
	byte vdc_satb;
	byte vdc_pendvsync;
	int bg_h,bg_w;
	int screen_w,screen_h;
	int scroll_y;
	int minline, maxline;
	/* joypad */
	byte JOY[16];
	byte joy_select,joy_counter;
	/* PSG */
	byte PSG[6][8],wave[6][32],wavofs[6];
	byte psg_ch,psg_volume,psg_lfo_freq,psg_lfo_ctrl;
	/* TIMER */
	byte timer_reload,timer_start,timer_counter;
	/* IRQ */
	byte irq_mask,irq_status;
	/* CDROM extention */
	int backup,adpcm_firstread;
	pair adpcm_ptr;
	word_s adpcm_rptr,adpcm_wptr;
} IO;

void PutImage(int X,int Y,int W,int H);
int CartLoad(char *name);
void ResetPCE(M6502 *M);
int InitPCE(char *name);
void TrashPCE(void);
BOOL InitMachine(void);
void TrashMachine(void);
int Joysticks(void);
int main(char* fileName);

void graphUpdate();
BOOL checkNeedDrawUpdate();