/*
	Portable PC-Engine Emulator
	1998 by BERO bero@geocities.co.jp

    Modified 1998 by hmmx hmmx@geocities.co.jp
    Modified for M5Stack by @shikarunochi 2021.12-
*/

//#include	<stdio.h>

#include	<stdlib.h>
#include    "M5Pce.h"
#include	"m6502.h"
#include	"pce.h"
#ifdef SOUND
#include	"sound.h"
#endif // SOUND
//#include	"info.h"
//#include	"log.h"

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

byte RAM[0x8000];
//byte PopRAM[0x8000];
//byte PopRAM[0x10000];
#define	ZW	64
//byte ZBuf[ZW*256];
byte *Page[8],*ROMMap[256];
//BOOL IsROM[8];
byte *VRAM,*ROM,*vchange,*vchanges,*PCM,*WRAM,*DMYROM,*IOAREA;
short SPRAM[64*4];
byte *SPM;//[WIDTH*HEIGHT]; //後で確保
unsigned long *VRAM2,*VRAMS;
int ROM_size;
extern int Debug,vmode;
int Country;
int IPeriod;
extern int BaseClock, UPeriod;
static int TimerCount,CycleOld;
int TimerPeriod;
int scanlines_per_frame  = 263;
int scanline;
#define MinLine	io.minline
#define MaxLine io.maxline
//int MinLine = 0,MaxLine = 255;
#define MAXDISP	227
byte BGONSwitch = 1, SPONSwitch = 1;
char *cart_name;
byte cart_reload;
byte populus = 0;
int scroll = 0;

byte Pal[512];

IO io;

extern char	*pCartName;

extern BOOL snd_bSound;

#define	VRR	2
enum _VDC_REG {
	MAWR,MARR,VWR,vdc3,vdc4,CR,RCR,BXR,
	BYR,MWR,HSR,HDR,VPR,VDW,VCR,DCR,
	SOUR,DISTR,LENR,SATB};
#define	NODATA	0xff
#define	ENABLE	1
#define	DISABLE	0

#define	VDC_CR	0x01
#define	VDC_OR	0x02
#define	VDC_RR	0x04
#define	VDC_DS	0x08
#define	VDC_DV	0x10
#define	VDC_VD	0x20
#define	VDC_BSY	0x40
#define	VDC_SpHit	VDC_CR
#define	VDC_Over	VDC_OR
#define	VDC_RasHit	VDC_RR
#define	VDC_InVBlank VDC_VD
#define	VDC_DMAfinish	VDC_DV
#define	VDC_SATBfinish	VDC_DS

#define	SpHitON		(io.VDC[CR].W&0x01)
#define	OverON		(io.VDC[CR].W&0x02)
#define	RasHitON	(io.VDC[CR].W&0x04)
#define	VBlankON	(io.VDC[CR].W&0x08)
#define	SpriteON	(io.VDC[CR].W&0x40)
#define	ScreenON	(io.VDC[CR].W&0x80)

#define SATBIntON	(io.VDC[DCR].W&0x01)
#define DMAIntON	(io.VDC[DCR].W&0x02)

#define	IRQ2	1
#define	IRQ1	2
#define	TIRQ	4

//#define ScrollX	io.scroll_x
//#define ScrollY io.scroll_y
#define	ScrollX	io.VDC[BXR].W
#define	ScrollY	io.VDC[BYR].W
int ScrollYDiff;
int	oldScrollX;
int	oldScrollY;
int	oldScrollYDiff;

typedef struct {
	short y,x,no,atr;
} SPR;


int CheckSprites(void);
int JoyStick(void);

void  IO_write(word_s A,byte V);
byte  IO_read(word_s A);
void RefreshLine(int Y1,int Y2);
void RefreshSprite(int Y1,int Y2,BYTE bg);
void RefreshScreen(void);

void bank_set(byte P,byte V)
{
	//Serial.printf("setBank:P=%x,v=%02x\n",P,V);
	if (V>=ROM_size && V<0xF7) {

		//Serial.printf("ignore bank set %x:%02x\n",P,V);
	//	TrashMachine();
	//	exit(-1);
	}
	//Serial.printf("P=%d:V=%d:ROMMAP[V]=%x\n",P,V,ROMMap[V]);
	if (ROMMap[V]==IOAREA){
		Page[P]=IOAREA;
		//Serial.printf("Set PAGE[%d][IOAREA]=%x\n",P,Page[P]);
	}
	else{
		Page[P]=ROMMap[V]-P*0x2000;
		//Serial.printf("Set PAGE[%d][ROMMAP[V]-P*x02000]=%x\n",P,Page[P]);
	}


	//Serial.printf("PAGE[%d]=%x\n",P,Page[P]);
//	IsROM[P] = (V < 0xF7);
}

byte _Rd6502(word_s A)
{
	if (Page[A>>13]!=IOAREA) return Page[A>>13][A];
	else return IO_read(A);
}

void _Wr6502(word_s A,byte V)
{
	if (Page[A>>13]!=IOAREA)
	{
//void DebugDumpFp(int nMemDump, BOOL bDispStack);
//		if (A==0x2003)
//		{
//			TRACE("val=%02X\n",V);
//			DebugDumpFp(4, TRUE);
//		}
//		if (IsROM[A>>13])
//			TRACE0("Writed to ROM\n");
//		else
			Page[A>>13][A]=V;
	}
	else IO_write(A,V);
}

/* write */
	M6502 M;

#ifdef _DEBUG
void DebugDumpTrace(int nMemDump, BOOL bDispStack)
{
	int		i;

	TRACE("PC=%04X, A=%02X,X=%02X,Y=%02X, S=%02X\n", M.PC.W, M.A, M.X, M.Y, M.S);
	TRACE0("Page=");
	for (i = 0; i < 8; i++)
		TRACE("%02X ", M.MPR[i]);
	if (nMemDump > 0)
	{
		TRACE0("\nMem=");
		for (i = -8; i < nMemDump; i++)
			TRACE("%02X ", Page[(M.PC.W+i)>>13][M.PC.W+i]);
		TRACE0("\n");
	}
	if (bDispStack)
	{
		TRACE0("Stack=");
		for (i = M.S+1; i <= 0xFF; i++)
			TRACE("%02X ", Page[1][0x2100+i]);
		TRACE0("\n");
	}
}

FILE	*fp;
void DebugDumpFp(int nMemDump, BOOL bDispStack)
{
	int		i;

	if (fp == NULL)
		fp = fopen("c:\\temp.out", "w");

	fprintf(fp, "PC=%04X, A=%02X,X=%02X,Y=%02X, S=%02X\n", M.PC.W, M.A, M.X, M.Y, M.S);
	fprintf(fp, "Page=");
	for (i = 0; i < 8; i++)
		fprintf(fp, "%02X ", M.MPR[i]);
	if (nMemDump > 0)
	{
		fprintf(fp, "\nMem=");
		for (i = -8; i < nMemDump; i++)
			fprintf(fp, "%02X ", Page[(M.PC.W+i)>>13][M.PC.W+i]);
		fprintf(fp, "\n");
	}
	if (bDispStack)
	{
		fprintf(fp, "Stack=");
		for (i = M.S+1; i <= 0xFF; i++)
			fprintf(fp, "%02X ", Page[1][0x2100+i]);
		fprintf(fp, "\n");
	}
}
#endif // _DEBUG

void  IO_write(word_s A,byte V)
{
	//printf("w%04x,%02x ",A&0x3FFF,V);
  switch(A&0x1C00) {
  case 0x0000:	/* VDC */
	switch(A&3){
	case 0: io.vdc_reg = V&31; return;
	case 1: return;
	case 2:
		//printf("vdc_l%d,%02x ",io.vdc_reg,V);
		switch(io.vdc_reg){
		case VWR:
			/*VRAM[io.VDC[MAWR].W*2] = V;*/ io.vdc_ratch = V;
			//io.VDC_ratch[VWR] = V;
/*			if (0x1000 <= io.VDC[MAWR].W && io.VDC[MAWR].W < 0x1008)
			{
				TRACE("L: PC = %X, ", M.PC.W);
				for (int i = -10; i < 4; i++)
					TRACE("%02X ", Page[(M.PC.W+i)>>13][M.PC.W+i]);
				TRACE("\nL: V[%X] = %X\n", io.VDC[MAWR].W, V);
			}
*/			return;
		case HDR:
			io.screen_w = (V+1)*8;
			//printf("screen:%dx%d %d\n",io.screen_w,256,V);
			//TRACE("HDRl: %X\n", V);
			break;
		case MWR:
			{static byte bgw[]={32,64,128,128};
			io.bg_h=(V&0x40)?64:32;
			io.bg_w=bgw[(V>>4)&3];}
			TRACE("bg:%dx%d, V:%X\n",io.bg_w,io.bg_h, V);
			//TRACE("MWRl: %02X\n", V);
			break;
		case BYR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BYR].B.l = V;
			scroll=1;
//			io.vdc_iswrite_h = 0;
//			if (scanline <= MaxLine)
//			{
				//TRACE("BYRl = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
			//if (RasHitON)
				ScrollYDiff=scanline-1;
//			}
//			else
//				ScrollYDiff=0;
			//TRACE("BYRl = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
			return;
		case BXR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BXR].B.l = V;
			scroll=1;
			return;
			
#define PRINT_VDC_L(REG)	case REG: TRACE(#REG "l: %X\n",V);break;
//			PRINT_VDC_L(CR);
/*			PRINT_VDC_L(HSR);
*/			PRINT_VDC_L(VPR);
			PRINT_VDC_L(VDW);
			PRINT_VDC_L(VCR);
/*			PRINT_VDC_L(SOUR);
			PRINT_VDC_L(DISTR);
*/			PRINT_VDC_L(DCR);
			
		}
		io.VDC[io.vdc_reg].B.l = V; //io.VDC_ratch[io.vdc_reg] = V;
//		if (io.vdc_reg != CR)
//			TRACE("vdc_l: %02X,%02X\n", io.vdc_reg, V);
		if (io.vdc_reg>19) {
			TRACE("ignore write lo vdc%d,%02x\n",io.vdc_reg,V);
		}
		return;
	case 3:
		//printf("vdc_h%d,%02x ",io.vdc_reg,V);
		switch(io.vdc_reg){
		case VWR:
			//printf("v%04x\n",io.VDC[MAWR].W);
			VRAM[io.VDC[MAWR].W*2]=io.vdc_ratch;
			VRAM[io.VDC[MAWR].W*2+1]=V;
/*			if (0x1000<=io.VDC[MAWR].W&&io.VDC[MAWR].W<0x1008)
			{
				TRACE("adr=%04X,val=%02X%02X\n", io.VDC[MAWR].W, V, io.vdc_ratch);
				//DebugDumpFp(4, TRUE);
			}
*/
/*			if (0x1000 <= io.VDC[MAWR].W && io.VDC[MAWR].W < 0x1100)
			{
				TRACE("PC = %X, ", M.PC.W);
				for (int i = -10; i < 4; i++)
					TRACE("%02X ", Page[(M.PC.W+i)>>13][M.PC.W+i]);
				TRACE("\nPage = %d", (Page[(M.PC.W)>>13] - ROM)/0x2000);
				TRACE("\nV[%X] = %02X%02X\n", io.VDC[MAWR].W, VRAM[io.VDC[MAWR].W*2+1], VRAM[io.VDC[MAWR].W*2]);
				TRACE("Page: ");
				for (i = 0; i < 8; i++)
					TRACE("%02X ", M.MPR[i]);
				TRACE("\nZero: ");
				for (i = 0; i < 32; i++)
					TRACE("%02X ", RAM[i]);
				TRACE("\n");
			}
*/			vchange[io.VDC[MAWR].W/16]=1;
			vchanges[io.VDC[MAWR].W/64]=1;
			io.VDC[MAWR].W+=io.vdc_inc;
			io.vdc_ratch=0;
			return;
		case VDW:
			//io.VDC[VDW].B.l = io.VDC_ratch[VDW];
			io.VDC[VDW].B.h = V;
			io.screen_h = (io.VDC[VDW].W&511)+1;
			MaxLine = io.screen_h-1;
			TRACE("VDWh: %X\n", io.VDC[VDW].W);
			return;
		case LENR:
			//io.VDC[LENR].B.l = io.VDC_ratch[LENR];
			io.VDC[LENR].B.h = V;
			TRACE("DMA:%04x %04x %04x\n",io.VDC[DISTR].W,io.VDC[SOUR].W,io.VDC[LENR].W);
			/* VRAM to VRAM DMA */
			memcpy(VRAM+io.VDC[DISTR].W*2,VRAM+io.VDC[SOUR].W*2,
				(io.VDC[LENR].W+1)*2);
			memset(vchange+io.VDC[DISTR].W/16,1,(io.VDC[LENR].W+1)/16);
			memset(vchange+io.VDC[DISTR].W/64,1,(io.VDC[LENR].W+1)/64);
			io.VDC[DISTR].W += io.VDC[LENR].W+1;
			io.VDC[SOUR].W += io.VDC[LENR].W+1;
			io.vdc_status|=VDC_DMAfinish;
			return;
		case CR :
			{static byte incsize[]={1,32,64,128};
			io.vdc_inc = incsize[(V>>3)&3];
			//TRACE("CRh: %02X\n", V);
			}
			break;
		case HDR:
			//io.screen_w = (io.VDC_ratch[HDR]+1)*8;
			//TRACE0("HDRh\n");
			break;
		case BYR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BYR].B.h = V&1;
			scroll=1;
//			io.vdc_iswrite_h = 1;
//			if (scanline <= MaxLine)
//			{
				//TRACE("BYRh = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
			//if (RasHitON)
			{
				ScrollYDiff=scanline-1;
				//TRACE("BYRh = %d, scanline = %d, h = %d\n", io.VDC[BYR].W, scanline, io.screen_h);
				//DebugDumpFp(4, TRUE);
			}
//			}
//			else
//				ScrollYDiff=0;
			//ScrollY = 51-35;
			return;
		case SATB:
			io.VDC[SATB].B.h = V;
			//TRACE("SATB=%X,scanline=%d\n", io.VDC[SATB].W, scanline);
			//memcpy(SPRAM,VRAM+io.VDC[SATB].W*2,64*8);
			io.vdc_satb=1;
			io.vdc_status&=~VDC_SATBfinish;
			return;
		case BXR:
			if (!scroll) {
				oldScrollX = ScrollX;
				oldScrollY = ScrollY;
				oldScrollYDiff = ScrollYDiff;
			}
			io.VDC[BXR].B.h = V & 3;
			scroll=1;
//			ScrollX = io.VDC[BXR].W;
//			TRACE("BXRh = %d, scanline = %d\n", io.VDC[BXR].W, scanline);
//			io.VDC[BXR].W = 256;
			return;
			
#define PRINT_VDC_H(REG)	case REG: TRACE(#REG "h: %X\n",V);break;
/*			PRINT_VDC_H(HSR);
			//PRINT_VDC_H(HDR);
*/			PRINT_VDC_H(VPR);
			PRINT_VDC_H(VCR);
/*			PRINT_VDC_H(SOUR);
			PRINT_VDC_H(DISTR);
*/			PRINT_VDC_H(DCR);
		//case RCR: TRACE("RCR: %02X%02X\n", V, io.VDC[io.vdc_reg].B.l);break;

		}
		//io.VDC[io.vdc_reg].B.l = io.VDC_ratch[io.vdc_reg];
		io.VDC[io.vdc_reg].B.h = V;
//		if (io.vdc_reg != CR)
//			TRACE("vdc_h: %02X,%02X\n", io.vdc_reg, V);
		if (io.vdc_reg>19) {
			TRACE("ignore write hi vdc%d,%02x\n",io.vdc_reg,V);
		}
		return;
	}
	break;

  case 0x0400:	/* VCE */
	switch(A&7) {
	case 0: /*io.vce_reg.W = 0; io.vce_ratch=0;*/ /*??*/
		TRACE("VCE 0, V=%X\n", V); return;
	case 2: io.vce_reg.B.l = V; return;
	case 3: io.vce_reg.B.h = V&1; return;
	case 4: io.VCE[io.vce_reg.W].B.l= V;
		{byte c; int i,n;
		n = io.vce_reg.W;
		c = io.VCE[n].W>>1;
		if (n==0) {
			for(i=0;i<256;i+=16) Pal[i]=c;
		}else if (n&15) Pal[n] = c;
		}
		return;
	case 5:
		/*io.VCE[io.vce_reg.W].B.l = io.vce_ratch;*/
//	DebugDumpFp(4, TRUE);
		io.VCE[io.vce_reg.W].B.h = V;
		{byte c; int i,n;
		n = io.vce_reg.W;
		c = io.VCE[n].W>>1;
		if (n==0) {
			for(i=0;i<256;i+=16) Pal[i]=c;
		}else if (n&15) Pal[n] = c;
		}
		io.vce_reg.W=(io.vce_reg.W+1)&0x1FF;
		return;
	//case 6: /* ?? */ return;
	case 1:	TRACE("VCE 1, V=%X\n", V); return;
	case 6:	TRACE("VCE 6, V=%X\n", V); return;
	case 7:	TRACE("VCE 7, V=%X\n", V); return;
	}
	break;

  case 0x0800:	/* PSG */
	if (snd_bSound && io.psg_ch < 6)
	{
//		if (io.psg_ch==0)
//			TRACE("PSG %d,%02X\n", A&15, V);
		// if ((A&15) <= 1)
		// {
		// 	int	i;
		// 	for (i = 0; i < 6; i++)
		// 		write_psg(i);
		// }
		// else
		// 	write_psg(io.psg_ch);
	}
  	switch(A&15){
	case 0: io.psg_ch = V&7; return;
	case 1: io.psg_volume = V; return;
	case 2: io.PSG[io.psg_ch][2] = V; break;
	case 3: io.PSG[io.psg_ch][3] = V&15; break;
	case 4: io.PSG[io.psg_ch][4] = V; break;
	case 5: io.PSG[io.psg_ch][5] = V; break;
	case 6: if (io.PSG[io.psg_ch][4]&0x40){
				io.wave[io.psg_ch][0]=V&31;
			}else {
				io.wave[io.psg_ch][io.wavofs[io.psg_ch]]=V&31;
				io.wavofs[io.psg_ch]=(io.wavofs[io.psg_ch]+1)&31;
			} break;
	case 7: io.PSG[io.psg_ch][7] = V; break;
	case 8: io.psg_lfo_freq = V; break;
	case 9: io.psg_lfo_ctrl = V; break;
	default: TRACE("ignored PSG write\n");
    }
    return;

  case 0x0c00:	/* timer */
	//TRACE("Timer Access: A=%X,V=%X\n", A, V);
	switch(A&1){
	case 0: io.timer_reload = V&127; return;
	case 1: 
		V&=1;
		if (V && !io.timer_start)
			io.timer_counter = io.timer_reload;
		io.timer_start = V;
		return;
	}
	break;

  case 0x1000:	/* joypad */
//	  TRACE("V=%02X\n", V);
		io.joy_select = V&1;
		if (V&2) io.joy_counter = 0;
		return;

  case 0x1400:	/* IRQ */
	switch(A&15){
	case 2: io.irq_mask = V;/*TRACE("irq_mask = %02X\n", V);*/ return;
	case 3: io.irq_status= (io.irq_status&~TIRQ)|(V&0xF8); return;
	}
	break;

  case 0x1800:	/* CD-ROM extention */
	switch(A&15){
	case 7: io.backup = ENABLE; return;
/*	case 8: io.adpcm_ptr.B.l = V; return;
	case 9: io.adpcm_ptr.B.h = V; return;
	case 0xa: PCM[io.adpcm_wptr++] = V; return;
	case 0xd:
		if (V&4) io.adpcm_wptr = io.adpcm_ptr.W;
		else { io.adpcm_rptr = io.adpcm_ptr.W; io.adpcm_firstread = TRUE; }
		return;
*/	}
	break;
  }
	TRACE("ignore I/O write %04x,%02x\n",A,V);
//	DebugDumpTrace(4, TRUE);
}

/* read */
byte IO_read(word_s A)
{
	byte ret;
	//printf("r%04x ",A&0x3FFF);
  switch(A&0x1C00){
  case 0x0000: /* VDC */
  	switch(A&3){
	case 0:
		ret = io.vdc_status;
		io.vdc_status=0;//&=VDC_InVBlank;//&=~VDC_BSY;
		return ret;
	case 1:
		return 0;
	case 2:
		if (io.vdc_reg==VRR)
			return VRAM[io.VDC[MARR].W*2];
		else return io.VDC[io.vdc_reg].B.l;
	case 3:
		if (io.vdc_reg==VRR) {
			ret = VRAM[io.VDC[MARR].W*2+1];
			io.VDC[MARR].W+=io.vdc_inc;
//			TRACE0("VRAM read\n");
			return ret;
		} else return io.VDC[io.vdc_reg].B.h;
	}
	break;

  case 0x0400:/* VCE */
  	switch(A&7){
	case 4: return io.VCE[io.vce_reg.W].B.l;
	case 5: return io.VCE[io.vce_reg.W++].B.h;
	}
	break;

  case 0x0800:	/* PSG */
	switch(A&15){
	case 0: return io.psg_ch;
	case 1: return io.psg_volume;
	case 2: return io.PSG[io.psg_ch][2];
	case 3: return io.PSG[io.psg_ch][3];
	case 4: return io.PSG[io.psg_ch][4];
	case 5: return io.PSG[io.psg_ch][5];
	case 6:
		{
			int	ofs=io.wavofs[io.psg_ch];
			io.wavofs[io.psg_ch]=(io.wavofs[io.psg_ch]+1)&31;
			return io.wave[io.psg_ch][ofs];
		}
	case 7: return io.PSG[io.psg_ch][7];
	case 8: return io.psg_lfo_freq;
	case 9: return io.psg_lfo_ctrl;
	default: return NODATA;
	}
	break;

  case 0x0c00:	/* timer */
	return io.timer_counter;

  case 0x1000:	/* joypad */
//	  TRACE("js=%d\n", io.joy_counter);
		ret = io.JOY[io.joy_counter]^0xff;
		if (io.joy_select&1) ret>>=4;
		else { ret&=15; io.joy_counter=(io.joy_counter+1)%5; }
		return ret|Country; /* country 0:JPN 1=US */

  case 0x1400:	/* IRQ */
	switch(A&15){
	case 2: return io.irq_mask;
	case 3: ret = io.irq_status;io.irq_status=0;return ret;
	}
	break;

  case 0x1800:	/* CD-ROM extention */
	switch(A&15){
	case 3: return io.backup = DISABLE;
//	case 0xa: 
//		if (!io.adpcm_firstread) return PCM[io.adpcm_rptr++];
//		else {io.adpcm_firstread=FALSE; return NODATA;}
	}
	break;
  }
	TRACE("ignore I/O read %04x\n",A);
//	DebugDumpFp(4, TRUE);
	return NODATA;
}

byte Loop6502(M6502 *R)
{
	static int UCount = 0;
	static int ACount = 0;
	static int prevline;
	int dispmin, dispmax;
	int ret;
	//printf("PC:%04x ",R->PC.W);

	dispmin = (MaxLine-MinLine>MAXDISP ? MinLine+((MaxLine-MinLine-MAXDISP+1)>>1) : MinLine);
	dispmax = (MaxLine-MinLine>MAXDISP ? MaxLine-((MaxLine-MinLine-MAXDISP+1)>>1) : MaxLine);

	scanline=(scanline+1)%scanlines_per_frame;
	//printf("scan %d\n",scanline);
	ret = INT_NONE;
	io.vdc_status&=~VDC_RasHit;
	if (scanline>MaxLine)
		io.vdc_status|=VDC_InVBlank;
//	if (scanline==MinLine+scanlines_per_frame-1)
//	else 
	if (scanline==MinLine) {
		io.vdc_status&=~VDC_InVBlank;
		prevline=dispmin;
		ScrollYDiff = 0;
		oldScrollYDiff = 0;
//		if (io.vdc_iswrite_h)
//		{
//			io.vdc_iswrite_h = 0;
//			ScrollY = io.VDC[BYR].W;
//		}
//		TRACE("\nFirstLine\n");
	}else
	if (scanline==MaxLine) {
		if (CheckSprites()) io.vdc_status|=VDC_SpHit;
		else io.vdc_status&=~VDC_SpHit;
		if (UCount) UCount--;
		else {
			if (SpriteON && SPONSwitch) RefreshSprite(prevline,dispmax,0);
			RefreshLine(prevline,dispmax-1);
			if (SpriteON && SPONSwitch) RefreshSprite(prevline,dispmax,1);
			prevline=dispmax;
			UCount=UPeriod;
			RefreshScreen();
		}
	}
	if (scanline>=MinLine && scanline<=MaxLine) {
		if (scanline==(io.VDC[RCR].W&1023)-64) {
			if (RasHitON && !UCount && dispmin<=scanline && scanline<=dispmax) {
				if (SpriteON && SPONSwitch) RefreshSprite(prevline,scanline,0);
				RefreshLine(prevline,scanline-1);
				if (SpriteON && SPONSwitch) RefreshSprite(prevline,scanline,1);
				prevline=scanline;
			}
			io.vdc_status|=VDC_RasHit;
			if (RasHitON) {
				//TRACE("rcr=%d\n", scanline);
				ret = INT_IRQ;
			}
		} else if (scroll) {
			if (scanline-1>prevline && !UCount) {
				int	tmpScrollX, tmpScrollY, tmpScrollYDiff;
				tmpScrollX=ScrollX;
				tmpScrollY=ScrollY;
				tmpScrollYDiff=ScrollYDiff;
				ScrollX=oldScrollX;
				ScrollY=oldScrollY;
				ScrollYDiff=oldScrollYDiff;
				if (SpriteON && SPONSwitch) RefreshSprite(prevline,scanline-1,0);
				RefreshLine(prevline,scanline-2);
				if (SpriteON && SPONSwitch) RefreshSprite(prevline,scanline-1,1);
				prevline=scanline-1;
				ScrollX = tmpScrollX;
				ScrollY = tmpScrollY;
				ScrollYDiff = tmpScrollYDiff;
			}
		}
	} else {
		int rcr = (io.VDC[RCR].W&1023)-64;
		if (scanline==rcr)
		{
//			ScrollYDiff = scanline;
			if (RasHitON) {
				//TRACE("rcr=%d\n", scanline);
				io.vdc_status |= VDC_RasHit;
				ret = INT_IRQ;
			}
		}
	}
	scroll=0;
	if (scanline==MaxLine+1) {
//	if (scanline==scanlines_per_frame-63) {
		int J=JoyStick();
		if (J&0x10000) return INT_QUIT;
		io.JOY[0]=J;

		/* VRAM to SATB DMA */
		if (io.vdc_satb==1 || io.VDC[DCR].W&0x0010)
		{
			memcpy(SPRAM,VRAM+io.VDC[SATB].W*2,64*8);
			io.vdc_satb=1;
			io.vdc_status&=~VDC_SATBfinish;
		}
		if (ret==INT_IRQ)
			io.vdc_pendvsync = 1;
		else {
			//TRACE("vsync\n");
			//io.vdc_status|=VDC_InVBlank;
			if (VBlankON) {
				//TRACE("vsync=%d\n", scanline);
				ret = INT_IRQ;
			}
		}
	}
	else 
	if (scanline==min(MaxLine+5, scanlines_per_frame-1)) {
		if (io.vdc_satb) {
			io.vdc_status|=VDC_SATBfinish;
			io.vdc_satb = 0;
			if (SATBIntON) {
				//TRACE("SATB=%d\n", scanline);
				ret = INT_IRQ;
			}
/*		} else {
			io.vdc_status&=~VDC_SATBfinish;
			io.vdc_satb = 0;
*/		}
	} else if (io.vdc_pendvsync && ret!=INT_IRQ) {
		io.vdc_pendvsync = 0;
		//io.vdc_status|=VDC_InVBlank;
		if (VBlankON) {
			//TRACE("vsync=%d\n", scanline);
			ret = INT_IRQ;
		}
	}
	if (ret == INT_IRQ) {
		if (!(io.irq_mask&IRQ1)) {
			io.irq_status|=IRQ1;
			//if (io.vdc_status&0x20)
			//TRACE("status=%02X\n", io.vdc_status);
			//TRACE("irq:scan %d\n ",scanline);
			return ret;
		}
	}
	return INT_NONE;
}

byte TimerInt(M6502 *R)
{
	if (io.timer_start) {
		io.timer_counter--;
		if (io.timer_counter > 128) {
			io.timer_counter = io.timer_reload;
			//io.irq_status &= ~TIRQ;
			if (!(io.irq_mask&TIRQ)) {
				io.irq_status |= TIRQ;
				//TRACE("tirq=%d\n",scanline);
				//TRACE("tirq\n");
				return INT_TIMER;
			}
		}
	}
	return INT_NONE;
}

int toRGB565(long c24) {
 /*
  *  R: R7 R6 R5 R4 R3
  *  G: G7 G6 G5 G4 G3 G2
  *  B: B7 B6 B5 B4 B3
  *
  */
  return (c24 >> 8 & 0xf800) |
         (c24 >> 5 & 0x7e0) |
         (c24 >> 3 & 0x1f);
}
/*
	Hit Chesk Sprite#0 and others
*/
int CheckSprites(void)
{
	int i,x0,y0,w0,h0,x,y,w,h;
	SPR *spr;
	spr = (SPR*)SPRAM;
	x0 = spr->x;
	y0 = spr->y;
	w0 = (((spr->atr>>8 )&1)+1)*16;
	h0 = (((spr->atr>>12)&3)+1)*16;
	spr++;
	for(i=1;i<64;i++,spr++) {
		x = spr->x;
		y = spr->y;
		w = (((spr->atr>>8 )&1)+1)*16;
		h = (((spr->atr>>12)&3)+1)*16;
		if ((x<x0+w0)&&(x+w>x0)&&(y<y0+h0)&&(y+h>y0)) return TRUE;
	}
	return FALSE;
}

static void plane2pixel(int no)
{
	unsigned long M;
	byte *C=VRAM+no*32;
	unsigned long L,*C2 = VRAM2+no*8;
	int j;
  for(j=0;j<8;j++,C+=2,C2++) {
    M=C[0];
    L =((M&0x88)>>3)|((M&0x44)<<6)|((M&0x22)<<15)|((M&0x11)<<24);
    M=C[1];
    L|=((M&0x88)>>2)|((M&0x44)<<7)|((M&0x22)<<16)|((M&0x11)<<25);
    M=C[16];
    L|=((M&0x88)>>1)|((M&0x44)<<8)|((M&0x22)<<17)|((M&0x11)<<26);
    M=C[17];
    L|=((M&0x88))|((M&0x44)<<9)|((M&0x22)<<18)|((M&0x11)<<27);
    C2[0] = L; //37261504
  }
}

#if 0
static void sprite2pixel(void *dst,void *src,int h)
{
  int i;
  long *C2 = dst;
  byte *C = src;
  for(i=0;i<h*2;i++,C++,C2++){
	long L;
	byte M;
    M=C[0];
    L =((M&0x88)>>3)|((M&0x44)<<6)|((M&0x22)<<15)|((M&0x11)<<24);
    M=C[32];
    L|=((M&0x88)>>2)|((M&0x44)<<7)|((M&0x22)<<16)|((M&0x11)<<25);
    M=C[64];
    L|=((M&0x88)>>1)|((M&0x44)<<8)|((M&0x22)<<17)|((M&0x11)<<26);
    M=C[96];
    L|=((M&0x88))|((M&0x44)<<9)|((M&0x22)<<18)|((M&0x11)<<27);
	C2[0]=L;
/*
	M = C[0];
    L  =((M&0x8888)>>3)|((M&0x4444)<<14);
    LL =((M&0x2222)>>1)|((M&0x1111)<<16);
	M = C[16];
    L |=((M&0x8888)>>2)|((M&0x4444)<<15);
    LL|=((M&0x2222))|((M&0x1111)<<17);
	M = C[32];
    L |=((M&0x8888)>>1)|((M&0x4444)<<16);
    LL|=((M&0x2222)<<1)|((M&0x1111)<<18);
	M = C[64];
    L |=((M&0x8888))|((M&0x4444)<<17);
    LL|=((M&0x2222)<<2)|((M&0x1111)<<19);
	C2[0]=L;   //159D048C
	C2[1]=LL;  //37BF26AE
	C++;
	C2+=2;
*/
  }
}
#endif
/*
static long sp2pixel(byte *C)
{
	long L;
	byte M;
    M=C[0];
    L =((M&0x88)>>3)|((M&0x44)<<6)|((M&0x22)<<15)|((M&0x11)<<24);
    M=C[32];
    L|=((M&0x88)>>2)|((M&0x44)<<7)|((M&0x22)<<16)|((M&0x11)<<25);
    M=C[64];
    L|=((M&0x88)>>1)|((M&0x44)<<8)|((M&0x22)<<17)|((M&0x11)<<26);
    M=C[96];
    L|=((M&0x88))|((M&0x44)<<9)|((M&0x22)<<18)|((M&0x11)<<27);
	return L;
}
*/
static void sp2pixel(int no)
{
	byte M;
	byte *C;
	unsigned long *C2;
	C=&VRAM[no*128];
	C2=&VRAMS[no*32];
	int i;
	for(i=0;i<32;i++,C++,C2++){
		long L;
		M=C[0];
		L =((M&0x88)>>3)|((M&0x44)<<6)|((M&0x22)<<15)|((M&0x11)<<24);
		M=C[32];
		L|=((M&0x88)>>2)|((M&0x44)<<7)|((M&0x22)<<16)|((M&0x11)<<25);
		M=C[64];
		L|=((M&0x88)>>1)|((M&0x44)<<8)|((M&0x22)<<17)|((M&0x11)<<26);
		M=C[96];
		L|=((M&0x88))|((M&0x44)<<9)|((M&0x22)<<18)|((M&0x11)<<27);
		C2[0]=L;
	}
}

#define	PAL(c)	R[c]
#define	SPal	(Pal+256)
extern byte *XBuf;
byte Black = 0;
#define	FC_W	io.screen_w
#define	FC_H	256

void RefreshLine(int Y1,int Y2)
{
	int X1,XW,Line;
	int x,y,h,offset,Shift;

	byte *PP;//,*ZP;
	//TRACE("%d-%d,Scroll=%d\n",Y1,Y2,ScrollY-ScrollYDiff);
	Y2++;
	PP=XBuf+WIDTH*(HEIGHT-FC_H)/2+(WIDTH-FC_W)/2+WIDTH*Y1;
//	if(!ScreenON || !BGONSwitch) memset(XBuf+Y1*WIDTH,Black,(Y2-Y1)*WIDTH);
//	else {
	if(ScreenON && BGONSwitch) {
		//TRACE("ScrollY=%d,diff=%d\n", ScrollY, ScrollYDiff);
		//TRACE("ScrollX=%d\n", ScrollX);
	y = Y1+ScrollY-ScrollYDiff;
	offset = y&7;
	h = 8-offset;
	if (h>Y2-Y1) h=Y2-Y1;
	y>>=3;
	PP-=ScrollX&7;
	XW=io.screen_w/8+1;
	Shift = ScrollX&7;
//	{byte *Z=ZBuf+Y1*ZW;
//	for(Line=Y1;Line<Y2;Line++,Z+=ZW) Z[0]=0;
//	}

  for(Line=Y1;Line<Y2;y++) {
//    ZP = ZBuf+Line*ZW;
	x = ScrollX/8;
	y &= io.bg_h-1;
	for(X1=0;X1<XW;X1++,x++,PP+=8/*,ZP++*/){
		byte *R,*P,*C;//,*Z;
		unsigned long *C2;
		int no,i;
		x&=io.bg_w-1;
		no = ((word_s*)VRAM)[x+y*io.bg_w];
		R = &Pal[(no>>12)*16];
		no&=0xFFF;
		if (vchange[no]) { vchange[no]=0; plane2pixel(no);}
		C2 = &VRAM2[no*8+offset];
		C = &VRAM[no*32+offset*2];
		P = PP;
//		Z = ZP;
		for(i=0;i<h;i++,P+=WIDTH,C2++,C+=2/*,Z+=ZW*/) {
			unsigned long L;
			BYTE	J;

			J = (C[0]|C[1]|C[16]|C[17]);
			if (!J) continue;

			L=C2[0];
			if (J&0x80) P[0]=PAL((L>>4)&15);
			if (J&0x40) P[1]=PAL((L>>12)&15);
			if (J&0x20) P[2]=PAL((L>>20)&15);
			if (J&0x10) P[3]=PAL((L>>28));
			if (J&0x08) P[4]=PAL((L)&15);
			if (J&0x04) P[5]=PAL((L>>8)&15);
			if (J&0x02) P[6]=PAL((L>>16)&15);
			if (J&0x01) P[7]=PAL((L>>24)&15);
//			L = J<<Shift;
//			Z[0]|=L>>8;Z[1]=L;
		}
	}
	Line+=h;
	PP+=WIDTH*h-XW*8;
	offset = 0;
	h = Y2-Line;
	if (h>8) h=8;
  }
  	}
	/* Refresh Sprite */
//	if (SpriteON) RefreshSprite(Y1,Y2);
}

#define	V_FLIP	0x8000
#define	H_FLIP	0x0800
#define	SPBG	0x80
#define	CGX		0x100

static void PutSprite(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH) {
		J = ((word_s*)C)[0]|((word_s*)C)[16]|((word_s*)C)[32]|((word_s*)C)[48];
		if (!J) continue;
		L = C2[1];//sp2pixel(C+1);
		if (J&0x8000) P[0]=PAL((L>>4)&15);
		if (J&0x4000) P[1]=PAL((L>>12)&15);
		if (J&0x2000) P[2]=PAL((L>>20)&15);
		if (J&0x1000) P[3]=PAL((L>>28));
		if (J&0x0800) P[4]=PAL((L)&15);
		if (J&0x0400) P[5]=PAL((L>>8)&15);
		if (J&0x0200) P[6]=PAL((L>>16)&15);
		if (J&0x0100) P[7]=PAL((L>>24)&15);
		L = C2[0];//sp2pixel(C);
		if (J&0x80) P[8 ]=PAL((L>>4)&15);
		if (J&0x40) P[9 ]=PAL((L>>12)&15);
		if (J&0x20) P[10]=PAL((L>>20)&15);
		if (J&0x10) P[11]=PAL((L>>28));
		if (J&0x08) P[12]=PAL((L)&15);
		if (J&0x04) P[13]=PAL((L>>8)&15);
		if (J&0x02) P[14]=PAL((L>>16)&15);
		if (J&0x01) P[15]=PAL((L>>24)&15);
	}
}

static void PutSpriteHflip(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH) {
		J = ((word_s*)C)[0]|((word_s*)C)[16]|((word_s*)C)[32]|((word_s*)C)[48];
		if (!J) continue;
		L = C2[1];//sp2pixel(C+1);
		if (J&0x8000) P[15]=PAL((L>>4)&15);
		if (J&0x4000) P[14]=PAL((L>>12)&15);
		if (J&0x2000) P[13]=PAL((L>>20)&15);
		if (J&0x1000) P[12]=PAL((L>>28));
		if (J&0x0800) P[11]=PAL((L)&15);
		if (J&0x0400) P[10]=PAL((L>>8)&15);
		if (J&0x0200) P[9]=PAL((L>>16)&15);
		if (J&0x0100) P[8]=PAL((L>>24)&15);
		L = C2[0];//sp2pixel(C);
		if (J&0x80) P[7]=PAL((L>>4)&15);
		if (J&0x40) P[6]=PAL((L>>12)&15);
		if (J&0x20) P[5]=PAL((L>>20)&15);
		if (J&0x10) P[4]=PAL((L>>28));
		if (J&0x08) P[3]=PAL((L)&15);
		if (J&0x04) P[2]=PAL((L>>8)&15);
		if (J&0x02) P[1]=PAL((L>>16)&15);
		if (J&0x01) P[0]=PAL((L>>24)&15);
	}
}

static void PutSpriteM(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,byte *M,byte pr)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,M+=WIDTH) {
		J = ((word_s*)C)[0]|((word_s*)C)[16]|((word_s*)C)[32]|((word_s*)C)[48];
		if (!J) continue;
		L = C2[1];//sp2pixel(C+1);
		if ((J&0x8000)&&M[0]<=pr) P[0]=PAL((L>>4)&15);
		if ((J&0x4000)&&M[1]<=pr) P[1]=PAL((L>>12)&15);
		if ((J&0x2000)&&M[2]<=pr) P[2]=PAL((L>>20)&15);
		if ((J&0x1000)&&M[3]<=pr) P[3]=PAL((L>>28));
		if ((J&0x0800)&&M[4]<=pr) P[4]=PAL((L)&15);
		if ((J&0x0400)&&M[5]<=pr) P[5]=PAL((L>>8)&15);
		if ((J&0x0200)&&M[6]<=pr) P[6]=PAL((L>>16)&15);
		if ((J&0x0100)&&M[7]<=pr) P[7]=PAL((L>>24)&15);
		L = C2[0];//sp2pixel(C);
		if ((J&0x80)&&M[8 ]<=pr) P[8 ]=PAL((L>>4)&15);
		if ((J&0x40)&&M[9 ]<=pr) P[9 ]=PAL((L>>12)&15);
		if ((J&0x20)&&M[10]<=pr) P[10]=PAL((L>>20)&15);
		if ((J&0x10)&&M[11]<=pr) P[11]=PAL((L>>28));
		if ((J&0x08)&&M[12]<=pr) P[12]=PAL((L)&15);
		if ((J&0x04)&&M[13]<=pr) P[13]=PAL((L>>8)&15);
		if ((J&0x02)&&M[14]<=pr) P[14]=PAL((L>>16)&15);
		if ((J&0x01)&&M[15]<=pr) P[15]=PAL((L>>24)&15);
	}
}

static void PutSpriteHflipM(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,byte *M,byte pr)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,M+=WIDTH) {
		J = ((word_s*)C)[0]|((word_s*)C)[16]|((word_s*)C)[32]|((word_s*)C)[48];
		if (!J) continue;
		L = C2[1];//sp2pixel(C+1);
		if ((J&0x8000)&&M[15]<=pr) P[15]=PAL((L>>4)&15);
		if ((J&0x4000)&&M[14]<=pr) P[14]=PAL((L>>12)&15);
		if ((J&0x2000)&&M[13]<=pr) P[13]=PAL((L>>20)&15);
		if ((J&0x1000)&&M[12]<=pr) P[12]=PAL((L>>28));
		if ((J&0x0800)&&M[11]<=pr) P[11]=PAL((L)&15);
		if ((J&0x0400)&&M[10]<=pr) P[10]=PAL((L>>8)&15);
		if ((J&0x0200)&&M[9]<=pr) P[9]=PAL((L>>16)&15);
		if ((J&0x0100)&&M[8]<=pr) P[8]=PAL((L>>24)&15);
		L = C2[0];//sp2pixel(C);
		if ((J&0x80)&&M[7]<=pr) P[7]=PAL((L>>4)&15);
		if ((J&0x40)&&M[6]<=pr) P[6]=PAL((L>>12)&15);
		if ((J&0x20)&&M[5]<=pr) P[5]=PAL((L>>20)&15);
		if ((J&0x10)&&M[4]<=pr) P[4]=PAL((L>>28));
		if ((J&0x08)&&M[3]<=pr) P[3]=PAL((L)&15);
		if ((J&0x04)&&M[2]<=pr) P[2]=PAL((L>>8)&15);
		if ((J&0x02)&&M[1]<=pr) P[1]=PAL((L>>16)&15);
		if ((J&0x01)&&M[0]<=pr) P[0]=PAL((L>>24)&15);
	}
}

static void PutSpriteMakeMask(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,byte *M,byte pr)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,M+=WIDTH) {
		J = ((word_s*)C)[0]|((word_s*)C)[16]|((word_s*)C)[32]|((word_s*)C)[48];
		if (!J) continue;
		L = C2[1];//sp2pixel(C+1);
		if (J&0x8000) {P[0]=PAL((L>>4)&15);  M[0]=pr;}
		if (J&0x4000) {P[1]=PAL((L>>12)&15); M[1]=pr;}
		if (J&0x2000) {P[2]=PAL((L>>20)&15); M[2]=pr;}
		if (J&0x1000) {P[3]=PAL((L>>28));    M[3]=pr;}
		if (J&0x0800) {P[4]=PAL((L)&15);     M[4]=pr;}
		if (J&0x0400) {P[5]=PAL((L>>8)&15);  M[5]=pr;}
		if (J&0x0200) {P[6]=PAL((L>>16)&15); M[6]=pr;}
		if (J&0x0100) {P[7]=PAL((L>>24)&15); M[7]=pr;}
		L = C2[0];//sp2pixel(C);
		if (J&0x80) {P[8 ]=PAL((L>>4)&15);  M[8]=pr;}
		if (J&0x40) {P[9 ]=PAL((L>>12)&15); M[9]=pr;}
		if (J&0x20) {P[10]=PAL((L>>20)&15); M[10]=pr;}
		if (J&0x10) {P[11]=PAL((L>>28));    M[11]=pr;}
		if (J&0x08) {P[12]=PAL((L)&15);     M[12]=pr;}
		if (J&0x04) {P[13]=PAL((L>>8)&15);  M[13]=pr;}
		if (J&0x02) {P[14]=PAL((L>>16)&15); M[14]=pr;}
		if (J&0x01) {P[15]=PAL((L>>24)&15); M[15]=pr;}
	}
}

static void PutSpriteHflipMakeMask(byte *P,byte *C,unsigned long *C2,byte *R,int h,int inc,byte *M,byte pr)
{
	int i,J;
	unsigned long L;
	for(i=0;i<h;i++,C+=inc,C2+=inc,P+=WIDTH,M+=WIDTH) {
		J = ((word_s*)C)[0]|((word_s*)C)[16]|((word_s*)C)[32]|((word_s*)C)[48];
		if (!J) continue;
		L = C2[1];//sp2pixel(C+1);
		if (J&0x8000) {P[15]=PAL((L>>4)&15);  M[15]=pr;}
		if (J&0x4000) {P[14]=PAL((L>>12)&15); M[14]=pr;}
		if (J&0x2000) {P[13]=PAL((L>>20)&15); M[13]=pr;}
		if (J&0x1000) {P[12]=PAL((L>>28));    M[12]=pr;}
		if (J&0x0800) {P[11]=PAL((L)&15);     M[11]=pr;}
		if (J&0x0400) {P[10]=PAL((L>>8)&15);  M[10]=pr;}
		if (J&0x0200) {P[9]=PAL((L>>16)&15);  M[9]=pr;}
		if (J&0x0100) {P[8]=PAL((L>>24)&15);  M[8]=pr;}
		L = C2[0];//sp2pixel(C);
		if (J&0x80) {P[7]=PAL((L>>4)&15);  M[7]=pr;}
		if (J&0x40) {P[6]=PAL((L>>12)&15); M[6]=pr;}
		if (J&0x20) {P[5]=PAL((L>>20)&15); M[5]=pr;}
		if (J&0x10) {P[4]=PAL((L>>28));    M[4]=pr;}
		if (J&0x08) {P[3]=PAL((L)&15);     M[3]=pr;}
		if (J&0x04) {P[2]=PAL((L>>8)&15);  M[2]=pr;}
		if (J&0x02) {P[1]=PAL((L>>16)&15); M[1]=pr;}
		if (J&0x01) {P[0]=PAL((L>>24)&15); M[0]=pr;}
	}
}

void RefreshSprite(int Y1,int Y2,BYTE bg)
{
	int n;
	SPR *spr;
	static int usespbg = 0;

	spr = (SPR*)SPRAM + 63;

	if (bg==0)
		usespbg=0;

	for(n=0;n<64;n++,spr--){
		int x,y,no,atr,inc,cgx,cgy;
		byte *R,*C;
		unsigned long *C2;
		int	pos;
		int h,t,i,j;
		int y_sum;
		int spbg;

		atr=spr->atr;
		spbg = (atr>>7)&1;
		if (spbg != bg)
			continue;
		y = (spr->y&1023)-64;
		x = (spr->x&1023)-32;
		no= spr->no&2047;
		cgx = (atr>>8)&1;
		cgy = (atr>>12)&3;
		cgy |= cgy>>1;
		no = (no>>1)&~(cgy*2+cgx);
		if (y>=Y2 || y+(cgy+1)*16<Y1 || x>=FC_W || x+(cgx+1)*16<0) continue;

		R = &SPal[(atr&15)*16];
		for (i=0;i<cgy*2+cgx+1;i++) {
			if (vchanges[no+i]) {
				vchanges[no+i] = 0;
				sp2pixel(no+i);
			}
			if (!cgx) i++;
		}
		C = &VRAM[no*128];
		C2 = &VRAMS[no*32];
		pos = WIDTH*(HEIGHT-FC_H)/2+(WIDTH-FC_W)/2+WIDTH*y+x;
		inc = 2;
		if (atr&V_FLIP) { inc=-2; C+=15*2+cgy*256; C2+=15*2+cgy*64;}
		y_sum = 0;
		//printf("(%d,%d,%d,%d,%d)",x,y,cgx,cgy,h);
		//TRACE("Spr#%d,no=%d,x=%d,y=%d,CGX=%d,CGY=%d,xb=%d,yb=%d\n", n, no, x, y, cgx, cgy, atr&H_FLIP, atr&V_FLIP);
		for(i=0;i<=cgy;i++) {
			t = Y1-y-y_sum;
			h = 16;
			if (t>0) {
				C+=t*inc;
				C2+=t*inc;
				h-=t;
				pos+=t*WIDTH;
			}
			if (h>Y2-y-y_sum) h = Y2-y-y_sum;
			if (spbg==0){
				usespbg=1;
				if (atr&H_FLIP){
				  for(j=0;j<=cgx;j++)
					PutSpriteHflipMakeMask(XBuf+pos+(cgx-j)*16,C+j*128,C2+j*32,R,h,inc,SPM+pos+(cgx-j)*16,n);
				}else{
				  for(j=0;j<=cgx;j++)
					PutSpriteMakeMask(XBuf+pos+j*16,C+j*128,C2+j*32,R,h,inc,SPM+pos+j*16,n);
				}
			} else if (usespbg) {
				if (atr&H_FLIP){
				  for(j=0;j<=cgx;j++)
					PutSpriteHflipM(XBuf+pos+(cgx-j)*16,C+j*128,C2+j*32,R,h,inc,SPM+pos+(cgx-j)*16,n);
				}else{
				  for(j=0;j<=cgx;j++)
					PutSpriteM(XBuf+pos+j*16,C+j*128,C2+j*32,R,h,inc,SPM+pos+j*16,n);
				}
			} else {
				if (atr&H_FLIP){
				  for(j=0;j<=cgx;j++)
					PutSpriteHflip(XBuf+pos+(cgx-j)*16,C+j*128,C2+j*32,R,h,inc);
				}else{
				  for(j=0;j<=cgx;j++)
					PutSprite(XBuf+pos+j*16,C+j*128,C2+j*32,R,h,inc);
				}
			}
			pos+=h*WIDTH;
			C+=h*inc+16*7*inc;
			C2+=h*inc+16*inc;
			y_sum+=16;
		}
	}
}

void RefreshScreen(void)
{
	//graphUpdate();
	//memset(SPM,1,HEIGHT*WIDTH);
	//memset(SPM+MinLine*WIDTH,0,(MaxLine-MinLine)*WIDTH);

	//memset(SPM+MinLine*WIDTH,0,(MaxLine-MinLine)*WIDTH);

	int dispmin,dispmax;

	dispmin = (MaxLine-MinLine>MAXDISP ? MinLine+((MaxLine-MinLine-MAXDISP+1)>>1) : MinLine);
	dispmax = (MaxLine-MinLine>MAXDISP ? MaxLine-((MaxLine-MinLine-MAXDISP+1)>>1) : MaxLine);
	PutImage((WIDTH-FC_W)/2,(HEIGHT-FC_H)/2+MinLine+dispmin,FC_W,dispmax-dispmin+1);
	memset(XBuf+MinLine*WIDTH,Pal[0],(MaxLine-MinLine)*WIDTH);
	memset(SPM+MinLine*WIDTH,0,(MaxLine-MinLine)*WIDTH);
}

int CartLoad(char *name)
{
	File fp;
	int fsize;
#ifdef ARDUINO_XIAO_ESP32S3
	fp=SPIFFS.open(name,"rb");
#else
	fp=SD.open(name,"rb");
#endif
	if (fp==NULL) {
		TRACE("%s not found.\n",name);
		return -1;
	}
	fp.seek(0, SeekEnd);
	fsize = fp.position();
	fp.seek(fsize&0x1fff, SeekSet);
	
	//printf("seekptr %x",ftell(fp));
	fsize&=~0x1fff;
	ROM = (BYTE*)ps_malloc(fsize);
	ROM_size = fsize/0x2000;
	//printf("ROM size:%x\n",fsize);
	fp.read((byte*)ROM, fsize) ;	
	fp.close();
	if (strstr(name, "populous.pce"))
	{
		TRACE("populus\n");
		populus = 1;
	}
	else
		populus = 0;
	return 0;
}


void ResetPCE(M6502 *M)
{
	Serial.println("1\n");
	memset(M,0,sizeof(*M));
	TimerCount = TimerPeriod;
	M->IPeriod = IPeriod;
	M->TrapBadOps = 1;
	Serial.println("2\n");
	memset(&io, 0, sizeof(IO));
	Serial.println("3\n");
	scanline = 0;
	io.vdc_status=0;//VDC_InVBlank;
	io.vdc_inc = 1;
	io.minline = 0;
	io.maxline = 255;
	io.irq_mask = 0;
	io.psg_volume = 0;
	io.psg_ch = 0;
	Serial.println("4\n");
	for (int i = 0; i < 6; i++)
	{
		io.PSG[i][4] = 0x80;
	}
	CycleOld = 0;
	Serial.println("5\n");
	Reset6502(M);
	//printf("reset PC=%04x ",M->PC.W);
	Serial.println("6\n");
}

int InitPCE(char *name, char *backmemname)
{
	int i,ROMmask;
	cart_name = name;
	if (CartLoad(name)) return -1;
#define	VRAMSIZE	0x20000

    SPM = (BYTE*)ps_malloc(WIDTH*HEIGHT); 
	memset(SPM,NODATA,WIDTH*HEIGHT);
	DMYROM=(BYTE *)ps_malloc(0x2000);
	memset(DMYROM,NODATA,0x2000);
	WRAM=(BYTE *)ps_malloc(0x2000);
	memset(WRAM,0,0x2000);
	VRAM=(BYTE *)ps_malloc(VRAMSIZE);
	VRAM2=(unsigned long *)ps_malloc(VRAMSIZE);
	VRAMS=(unsigned long *)ps_malloc(VRAMSIZE);
	//memset(VRAM,0,VRAMSIZE);
	IOAREA=(BYTE *)ps_malloc(0x2000);
	memset(IOAREA,0xFF,0x2000);
	vchange = (BYTE *)ps_malloc(VRAMSIZE/32);
	memset(vchange,1,VRAMSIZE/32);
	vchanges = (BYTE *)ps_malloc(VRAMSIZE/128);
	memset(vchanges,1,VRAMSIZE/128);
	ROMmask = 1;
	while(ROMmask<ROM_size) ROMmask<<=1;
	ROMmask--;
	TRACE("ROMmask=%02X, ROM_size=%02X\n", ROMmask, ROM_size);
	for(i=0;i<0xF7;i++)
	{
		if (ROM_size == 0x30)
		{
			switch (i&0x70)
			{
			case 0x00:
			case 0x10:
			case 0x50:
				ROMMap[i]=ROM+(i&ROMmask)*0x2000;
				break;
			case 0x20:
			case 0x60:
				ROMMap[i]=ROM+((i-0x20)&ROMmask)*0x2000;
				break;
			case 0x30:
			case 0x70:
				ROMMap[i]=ROM+((i-0x10)&ROMmask)*0x2000;
				break;
			case 0x40:
				ROMMap[i]=ROM+((i-0x20)&ROMmask)*0x2000;
				break;
			}
		}
		else
			ROMMap[i]=ROM+(i&ROMmask)*0x2000;
	}
//		ROMMap[i]=ROM+(i%ROM_size+i/ROM_size*0x10)*0x2000;
/*		if (((i&ROMmask)+i/(ROMmask+1)) < ROM_size)
			ROMMap[i]=ROM+((i&ROMmask)+i/(ROMmask+1)*0x20)*0x2000;
		else
			ROMMap[i]=ROM;
*///		ROMMap[i]=ROM+(i&ROMmask)*0x2000;
	if (populus)
	{
		//ROMMap[0x40] = PopRAM + (0)*0x2000;
		//ROMMap[0x41] = PopRAM + (1)*0x2000;
		//ROMMap[0x42] = PopRAM + (2)*0x2000;
		//ROMMap[0x43] = PopRAM + (3)*0x2000;
	}
/*	ROMMap[0x80] = PopRAM + (0)*0x2000;
	ROMMap[0x81] = PopRAM + (1)*0x2000;
	ROMMap[0x82] = PopRAM + (2)*0x2000;
	ROMMap[0x83] = PopRAM + (3)*0x2000;
	ROMMap[0x84] = PopRAM + (4)*0x2000;
	ROMMap[0x85] = PopRAM + (5)*0x2000;
	ROMMap[0x86] = PopRAM + (6)*0x2000;
	ROMMap[0x87] = PopRAM + (7)*0x2000;
*/	ROMMap[0xF7]=WRAM;
	ROMMap[0xF8]=RAM;
	ROMMap[0xF9]=RAM+0x2000;
	ROMMap[0xFA]=RAM+0x4000;
	ROMMap[0xFB]=RAM+0x6000;
	ROMMap[0xFF]=IOAREA; //NULL; /* NULL = I/O area */

	// File fp;
	// fp=SD.open(backmemname,"rb");
	// if (fp != true){
	// 	//LogDump("Can't open %s\n", backmemname);
	// }
	// else
	// {
	// 	fp.read(WRAM, 0x2000);
	// 	fp.close();
	// }
	TRACE("INIT OK!");
	return 0;
}

int RunPCE(void)
{
	TRACE("Run PCE\n");
//	M6502 M;
	ResetPCE(&M);
	TRACE("RESET OK\n");
	Run6502(&M);
	TRACE("Run6502 OK\n");
	return 1;
}

void TrashPCE(char *backmemname)
{
	File fp;

#ifdef _DEBUG
	fp = fopen("romdump.pce", "wb");
	if (fp == NULL)
		TRACE0("can't open file\n");
	else
	{
		fwrite(ROM, 0x2000, ROM_size, fp);
		fclose(fp);
	}
#endif
#ifdef ARDUINO_XIAO_ESP32S3
	fp=SPIFFS.open(backmemname,"wb");
#else	
	fp=SD.open(backmemname,"wb");
#endif
	if (fp != true){
		//LogDump("Can't open %s\n", backmemname);
	}
	else
	{
		fp.write(WRAM, 0x2000);
		fp.close();
	}
	if (IOAREA) free(IOAREA);
	if (vchange) free(vchange);
	if (vchanges) free(vchanges);
	if (DMYROM) free(DMYROM);
	if (WRAM) free(WRAM);
	if (VRAM) free(VRAM);
	if (VRAM2) free(VRAM2);
	if (VRAMS) free(VRAMS);
	if (ROM) free(ROM);
}

int JoyStick(void)
{
	return Joysticks();
}
#ifndef _WIN32
int main(char* fileName)
#else /* WIN32 */
int main_loop(int argc,char *argv[])
#endif /* _WIN32 */
{
	BaseClock = 7160000;//7160000; //3.58-21.48;
	IPeriod = BaseClock/(scanlines_per_frame*60);

	TRACE("IPeriod = %d\n", IPeriod);
    UPeriod = 2;

	TimerPeriod = BaseClock/1000*3*1024/21480;

	TRACE("TimerPeriod = %d\n", TimerPeriod);

	vmode = 0;
	Debug = 0;
	Serial.println(fileName);
	if (!InitMachine()) return -1;
	if (!InitPCE(fileName, "/pceROM/pce_mem.dat")) {
		while (1)
		{
			//SetInfoString("Emulation started");
			RunPCE();
			TrashPCE("/pceROM/pce_mem.dat");
			if (!cart_reload)
				break;
			cart_reload = 0;
			InitPCE(pCartName, "/pceROM/pce_mem.dat");
		}
	}
	TrashMachine();
	return 0;
}
