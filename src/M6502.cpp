/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                         M6502.c                         **/
/**                                                         **/
/** This file contains implementation for 6502 CPU. Don't   **/
/** forget to provide Rd6502(), Wr6502(), Loop6502(), and   **/
/** possibly Op6502() functions to accomodate the emulated  **/
/** machine's architecture.                                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/**               Alex Krasivsky  1996                      **/
/** Modyfied      BERO            1998                      **/
/** Modyfied      hmmx            1998                      **/
/** Modyfied for M5Stack @shikarunochi  2021                **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/   
/**     changes to this file.                               **/
/*************************************************************/

#include "M6502.h"
#ifdef HuC6280
#include "HuTables.h"
#define	BANK_SET(P,V)	bank_set(P,V)
extern void bank_set(byte P,byte V);
//Page[P]=ROMMap[V]-(P)*0x2000/*;printf("bank%x,%02x ",P,V)*/
extern byte *ROMMap[];
extern void  IO_write(word_s A,byte V);
#else
#include "Tables.h"
#endif
#include <stdio.h>
#ifdef ARDUINO_XIAO_ESP32S3
#else
#include <M5Stack.h>
#endif
#include "M5Pce.h"
/** INLINE ***************************************************/
/** Different compilers inline C functions differently.     **/
/*************************************************************/
#ifdef __GNUC__
#define INLINE static inline
#else
#ifdef _WIN32
#define INLINE static inline
#else
#define INLINE static
#endif
#endif

/** System-Dependent Stuff ***********************************/
/** This is system-dependent code put here to speed things  **/
/** up. It has to stay inlined to be fast.                  **/
/*************************************************************/
#if 1 //def INES
#define FAST_RDOP
extern byte *Page[];
extern byte RAM[];
#if 1
INLINE byte Op6502(register unsigned A) { return (Page[A>>13][A]); }
INLINE unsigned Op6502w(register unsigned A) { return(Page[A>>13][A])|(Page[(A+1)>>13][A+1]<<8); }
INLINE unsigned RdRAMw(register unsigned A) { return RAM[A]|(RAM[A+1]<<8); }

//byte Op6502(unsigned int A) { 
//  Serial.printf("A>>13=%x:A=%x\n",A>>13,A);
////  unsigned char *p = 0;//Page[A>>13];
////  Serial.printf("p=%x\n",*p);
////  return *(p + A);
//  return (Page[A>>13][A]); 
//}
//unsigned int Op6502w(unsigned int A) { return(Page[A>>13][A])|(Page[(A+1)>>13][A+1]<<8); }
//unsigned int RdRAMw(unsigned int A) { return RAM[A]|(RAM[A+1]<<8); }

/*
INLINE void Wr6502(unsigned A,byte V)
{
	if (A<0x800) RAM[A]=V; else _Wr6502(A,V);
}

INLINE byte Rd6502(unsigned A)
{
	if (A<0x800 || A>=0x8000) return Page[A>>13][A]; else return _Rd6502(A);
}
*/
/*
#ifdef HuC6280
extern byte *IOAREA;
extern void  IO_write(word_s A,byte V);
extern byte  IO_read(word_s A);

#define _Rd6502(A) \
	((Page[(A)>>13]!=IOAREA) ? Page[(A)>>13][A] : IO_read(A))

#define _Wr6502(A,V) \
do { \
	word_s a=A; \
	byte v=V; \
	(Page[a>>13]!=IOAREA) ? Page[a>>13][a]=v : IO_write(a,v); \
} while (0)

/*
inline byte _Rd6502(word_s A)
{
	if (Page[A>>13]!=IOAREA) return Page[A>>13][A];
	else return IO_read(A);
}

inline void _Wr6502(word_s A,byte V)
{
	if (Page[A>>13]!=IOAREA) Page[A>>13][A]=V;
	else IO_write(A,V);
}
*/
/*#endif
*/
#define	RdRAM(A)	RAM[A]
#define	WrRAM(A,V)	RAM[A]=V
#define	AdrRAM(A)	&RAM[A]
#else
#define	Op6502(A)	Rd6502(A)
#define	Op6502w(A)	(Op6502(A)|(Op6502(A+1)<<8))
#define	RdRAM(A)	Rd6502(A)
#define	WrRAM(A,V)	Wr6502(A,V)
#endif

#endif

/** FAST_RDOP ************************************************/
/** With this #define not present, Rd6502() should perform  **/
/** the functions of Rd6502().                              **/
/*************************************************************/
#ifndef FAST_RDOP
#define Op6502(A) Rd6502(A)
#endif

#define	C_SFT	0
#define	Z_SFT	1
#define	I_SFT	2
#define	D_SFT	3
#define	B_SFT	4
#define	R_SFT	5
#define	V_SFT	6
#define	N_SFT	7

#define	_A	R->A
#define	_P	R->P
#define	_X	R->X
#define	_Y	R->Y
#define	_S	R->S
#define	_PC	R->PC
#define	_PC_	R->PC.W
#define _ZF	R->ZF
#define _VF	R->VF
#define _NF	R->NF
#define	_IPeriod	R->IPeriod
#define	_ICount		R->ICount
#define	_IRequest	R->IRequest
#define	_IPeriod	R->IPeriod
#define	_IBackup	R->IBackup
#define	_TrapBadOps	R->TrapBadOps
#define	_Trace	R->Trace
#define	_Trap	R->Trap
#define	_AfterCLI	R->AfterCLI
#define	_User	R->User
#define _CycleCount	(*(int*)&_User)

#define	ZP	0
#define	SP	0x100

/** Addressing Methods ***************************************/
/** These macros calculate and return effective addresses.  **/
/*************************************************************/
#define MCZp()	(Op6502(_PC_++))+ZP
#define MCZx()	(byte)(Op6502(_PC_++)+_X)+ZP
#define MCZy()	(byte)(Op6502(_PC_++)+_Y)+ZP
//#define MCZx()	((Op6502(_PC_++)+_X)&0xff)
//#define MCZy()	((Op6502(_PC_++)+_Y)&0xff)
#define	MCIx()	(RdRAMw(MCZx()))
#define	MCIy()	(RdRAMw(MCZp())+_Y)
#define	MCAb()	(Op6502w(_PC_))
#define MCAx()	(Op6502w(_PC_)+_X)
#define MCAy()	(Op6502w(_PC_)+_Y)

#define MC_Ab(Rg)	M_LDWORD(Rg)
#define MC_Zp(Rg)	Rg.B.l=Op6502(_PC_++);Rg.B.h=ZP>>8
#define MC_Zx(Rg)	Rg.B.l=Op6502(_PC_++)+R->X;Rg.B.h=ZP>>8
#define MC_Zy(Rg)	Rg.B.l=Op6502(_PC_++)+R->Y;Rg.B.h=ZP>>8
#define MC_Ax(Rg)	M_LDWORD(Rg);Rg.W+=_X
#define MC_Ay(Rg)	M_LDWORD(Rg);Rg.W+=_Y
#if 1
#define MC_Ix(Rg)	K.W=MCZx(); \
			Rg.B.l=RdRAM(K.W);Rg.B.h=RdRAM(K.W+1)
#define MC_Iy(Rg)	K.W=MCZp(); \
			Rg.B.l=RdRAM(K.W);Rg.B.h=RdRAM(K.W+1); \
			Rg.W+=_Y
#else
#define	MC_Ix(Rg)	Rg.W=(RdRAMw(MCZx()))
#define	MC_Iy(Rg)	Rg.W=(RdRAMw(MCZp())+_Y)
#endif
/*
#define MC_Ix(Rg)	{byte *p=AdrRAM(MCZx()); \
			Rg.B.l=p[0];Rg.B.h=p[1]; }
#define MC_Iy(Rg)	{byte *p=AdrRAM(MCZp()); \
			Rg.B.l=p[0];Rg.B.h=p[1]; } \
			Rg.W+=_Y;
*/

/** Reading From Memory **************************************/
/** These macros calculate address and read from it.        **/
/*************************************************************/
#define MR_Ab(Rg)	MC_Ab(J);Rg=Rd6502(J.W)
//#define MR_Ab(Rg)	Rg=Rd6502(MCAb());_PC_+=2
#define MR_Im(Rg)	Rg=Op6502(_PC_++)
#define	MR_Zp(Rg)	Rg=RdRAM(MCZp())
#define MR_Zx(Rg)	Rg=RdRAM(MCZx())
#define MR_Zy(Rg)	Rg=RdRAM(MCZy())
#define	MR_Ax(Rg)	MC_Ax(J);Rg=Rd6502(J.W)
#define MR_Ay(Rg)	MC_Ay(J);Rg=Rd6502(J.W)
//#define MR_Ax(Rg)	Rg=Rd6502(MCAx());_PC_+=2;
//#define MR_Ay(Rg)	Rg=Rd6502(MCAy());_PC_+=2;
//#define MR_Ix(Rg)	Rg=Rd6502(MCIx())
//#define MR_Iy(Rg)	Rg=Rd6502(MCIy())
#define MR_Ix(Rg)	MC_Ix(J);Rg=Rd6502(J.W)
#define MR_Iy(Rg)	MC_Iy(J);Rg=Rd6502(J.W)

/** Writing To Memory ****************************************/
/** These macros calculate address and write to it.         **/
/*************************************************************/
//#define MW_Ab(Rg)	Wr6502(MCAb(),Rg);_PC_+=2
#define MW_Ab(Rg)	MC_Ab(J);Wr6502(J.W,Rg)
#define MW_Zp(Rg)	WrRAM(MCZp(),Rg)
#define MW_Zx(Rg)	WrRAM(MCZx(),Rg)
#define MW_Zy(Rg)	WrRAM(MCZy(),Rg)
#define MW_Ax(Rg)	MC_Ax(J);Wr6502(J.W,Rg)
#define MW_Ay(Rg)	MC_Ay(J);Wr6502(J.W,Rg)
//#define MW_Ax(Rg)	Wr6502(MCAx(),Rg);_PC_+=2
//#define MW_Ay(Rg)	Wr6502(MCAy(),Rg);_PC_+=2
//#define MW_Ix(Rg)	Wr6502(MCIx(),Rg)
//#define MW_Iy(Rg)	Wr6502(MCIy(),Rg)
#define MW_Ix(Rg)	MC_Ix(J);Wr6502(J.W,Rg)
#define MW_Iy(Rg)	MC_Iy(J);Wr6502(J.W,Rg)

/** Modifying Memory *****************************************/
/** These macros calculate address and modify it.           **/
/*************************************************************/
#define MM_Ab(Cmd)	MC_Ab(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Zp(Cmd)	J.W=MCZp();I=RdRAM(J.W);Cmd(I);WrRAM(J.W,I)
#define MM_Zx(Cmd)	J.W=MCZx();I=RdRAM(J.W);Cmd(I);WrRAM(J.W,I)
//#define MM_Zp(Cmd)	{unsigned A=MCZp();I=RdRAM(A);Cmd(I);WrRAM(A,I); }
//#define MM_Zx(Cmd)	{unsigned A=MCZx();I=RdRAM(A);Cmd(I);WrRAM(A,I); }
//#define MM_Zp(Cmd)	{byte *p=AdrRAM(MCZp());I=*p;Cmd(I);*p=I;}
//#define MM_Zx(Cmd)	{byte *p=AdrRAM(MCZx());I=*p;Cmd(I);*p=I;}
#define MM_Ax(Cmd)	MC_Ax(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)

/** Other Macros *********************************************/
/** Calculating flags, stack, jumps, arithmetics, etc.      **/
/*************************************************************/
#define M_FL(Rg)	_ZF=_NF=Rg
#define M_LDWORD(Rg)	Rg.B.l=Op6502(_PC_);Rg.B.h=Op6502(_PC_+1);_PC_+=2

#define M_PUSH(Rg)	WrRAM(SP+_S,Rg);_S--
#define M_POP(Rg)	_S++;Rg=RdRAM(SP+_S)
#define M_PUSH_P(Rg)	M_PUSH(((Rg)&~(N_FLAG|V_FLAG|Z_FLAG))|(_NF&N_FLAG)|(_VF&V_FLAG)|(_ZF? 0:Z_FLAG))
#define M_POP_P(Rg)		M_POP(Rg);_NF=_VF=Rg;_ZF=(Rg&Z_FLAG? 0:1)
#ifdef HuC6280
#define M_JR		_PC_+=(offset)Op6502(_PC_)+1;cycle+=2
#else
#define M_JR		_PC_+=(offset)Op6502(_PC_)+1;cycle++
#endif

#define M_ADC(Rg) \
  if(_P&D_FLAG) \
  { \
    K.B.l=(_A&0x0F)+(Rg&0x0F)+(_P&C_FLAG); \
    K.B.h=(_A>>4)+(Rg>>4);/*+(K.B.l>15? 1:0);*/ \
    if(K.B.l>9) { K.B.l+=6;K.B.h++; } \
    if(K.B.h>9) K.B.h+=6; \
    _A=(K.B.l&0x0F)|(K.B.h<<4); \
    _P=(_P&~C_FLAG)|(K.B.h>15? C_FLAG:0); \
	_ZF=_NF=_A; \
	cycle++; \
  } \
  else \
  { \
    K.W=_A+Rg+(_P&C_FLAG); \
    _P&=~C_FLAG; \
    _P|=(K.B.h? C_FLAG:0); \
    _VF=(~(_A^Rg)&(_A^K.B.l))>>1; \
	_ZF=_NF=K.B.l; \
    _A=K.B.l; \
  }

/* Warning! C_FLAG is inverted before SBC and after it */
#define M_SBC(Rg) \
  if(_P&D_FLAG) \
  { \
    K.B.l=(_A&0x0F)-(Rg&0x0F)-(~_P&C_FLAG); \
    if(K.B.l&0x10) K.B.l-=6; \
    K.B.h=(_A>>4)-(Rg>>4)-((K.B.l&0x10)==0x10); \
    if(K.B.h&0x10) K.B.h-=6; \
    _A=(K.B.l&0x0F)|(K.B.h<<4); \
    _P=(_P&~C_FLAG)|((K.B.h&0x10)? 0:C_FLAG); \
	_ZF=_NF=_A; \
	cycle++; \
  } \
  else \
  { \
    K.W=_A-Rg-(~_P&C_FLAG); \
    _P&=~C_FLAG; \
    _P|=(K.B.h? 0:C_FLAG); \
    _VF=((_A^Rg)&(_A^K.B.l))>>1; \
    _ZF=_NF=K.B.l; \
    _A=K.B.l; \
  }

#define M_CMP(Rg1,Rg2) \
  K.W=Rg1-Rg2; \
  _P&=~C_FLAG; \
  _P|=(K.B.h? 0:C_FLAG); \
  _ZF=_NF=K.B.l
#define M_BIT(Rg) \
  _NF=_VF=Rg;_ZF=Rg&_A

#define M_AND(Rg)	_A&=Rg;M_FL(_A)
#define M_ORA(Rg)	_A|=Rg;M_FL(_A)
#define M_EOR(Rg)	_A^=Rg;M_FL(_A)
#define M_INC(Rg)	Rg++;M_FL(Rg)
#define M_DEC(Rg)	Rg--;M_FL(Rg)

#define M_ASL(Rg)	_P&=~C_FLAG;_P|=Rg>>7;Rg<<=1;M_FL(Rg)
#define M_LSR(Rg)	_P&=~C_FLAG;_P|=Rg&C_FLAG;Rg>>=1;M_FL(Rg)
#define M_ROL(Rg)	K.B.l=(Rg<<1)|(_P&C_FLAG); \
			_P&=~C_FLAG;_P|=Rg>>7;Rg=K.B.l; \
			M_FL(Rg)
#define M_ROR(Rg)	K.B.l=(Rg>>1)|(_P<<7); \
			_P&=~C_FLAG;_P|=Rg&C_FLAG;Rg=K.B.l; \
			M_FL(Rg)

/** Reset6502() **********************************************/
/** This function can be used to reset the registers before **/
/** starting execution with Run6502(). It sets registers to **/
/** their initial values.                                   **/
/*************************************************************/
void Reset6502(M6502 *R)
{
#ifdef HuC6280
  R->MPR[7]=0x00; BANK_SET(7,0x00);
  R->MPR[6]=0x05; BANK_SET(6,0x05);
  R->MPR[5]=0x04; BANK_SET(5,0x04);
  R->MPR[4]=0x03; BANK_SET(4,0x03);
  R->MPR[3]=0x02; BANK_SET(3,0x02);
  R->MPR[2]=0x01; BANK_SET(2,0x01);
  R->MPR[1]=0xF8; BANK_SET(1,0xF8);
  R->MPR[0]=0xFF; BANK_SET(0,0xFF);
#endif
  _A=_X=_Y=0x00;
  _P=I_FLAG;
  _NF=_VF=0;
  _ZF=0xFF;
  _S=0xFF;
  _PC.B.l=Op6502(VEC_RESET);

  _PC.B.h=Op6502(VEC_RESET+1);   
  //Serial.printf("%x:%x:%x:%x : StartPC = %x \n", VEC_RESET>>13, VEC_RESET, Page[VEC_RESET>>13][VEC_RESET], Page[(VEC_RESET+1)>>13][VEC_RESET+1],_PC);
  _ICount=_IPeriod;
  _IRequest=INT_NONE;
  _AfterCLI=0;
#ifdef HuC6280
  _CycleCount=0;
#endif
}

#if 0
/** Exec6502() ***********************************************/
/** This function will execute a single 6502 opcode. It     **/
/** will then return next PC, and current register values   **/
/** in R.                                                   **/
/*************************************************************/
word_s Exec6502(M6502 *R)
{
  register pair J,K;
  register byte I;

  I=Op6502(_PC_++);
  _ICount-=Cycles[I];
  switch(I)
  {
#include "Codes.h"
  }

  /* We are done */
  return(_PC_);
}
#endif

/** Int6502() ************************************************/
/** This function will generate interrupt of a given type.  **/
/** INT_NMI will cause a non-maskable interrupt. INT_IRQ    **/
/** will cause a normal interrupt, unless I_FLAG set in R.  **/
/*************************************************************/
void Int6502(M6502 *R,byte Type)
{
  register pair J;

  if((Type==INT_NMI)||(/*(Type==INT_IRQ)&&*/!(_P&I_FLAG)))
  {
    _ICount-=7;
    M_PUSH(_PC.B.h);
    M_PUSH(_PC.B.l);
    M_PUSH_P(_P&~(B_FLAG|T_FLAG));
    _P&=~D_FLAG;
    if (Type==INT_NMI){
		J.W=VEC_NMI;
    } else {
    _P|=I_FLAG;
    switch(Type){
    case INT_IRQ:J.W=VEC_IRQ;break;
#ifdef HuC6280
    case INT_IRQ2:J.W=VEC_IRQ2;break;
    case INT_TIMER:J.W=VEC_TIMER;break;
#endif
    }
    }
    _PC.B.l=Op6502(J.W);
    _PC.B.h=Op6502(J.W+1);
  } else {
	  _IRequest|=Type;
  }
}

extern word_s PCbuf[];
extern int PCstep,PCstepMask;

/** Run6502() ************************************************/
/** This function will run 6502 code until Loop6502() call  **/
/** returns INT_QUIT. It will return the PC at which        **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
word_s Run6502(M6502 *R)
{
  register pair J,K;
  register byte I;
  int	cycle;
  static int CycleCountOld;
  extern int TimerPeriod;
/*
  WORD *pp = (WORD *)malloc(100000);
  ASSERT(pp != NULL);
  WORD *p = pp;
  BOOL	bTrace = FALSE;
  int iCount = 0;
*/
#ifdef _DEBUG
	_Trace = 1;
	_Trap = 0xFFFF;
#endif
  for(;;)
  {
#ifdef DEBUG
	extern int Debug;
	if (Debug) {
	PCbuf[PCstep]=_PC_; PCstep=(PCstep+1)&(PCstepMask);
	if (kbhit()) {getch();_Trace=1;}
    /* Turn tracing on when reached trap address */
  	if(_PC_==_Trap) _Trace=1;
    /* Call single-step debugger, exit if requested */
    if(_Trace)
      if(!Debug6502(R)) return(_PC_);
	}
#endif
#ifdef _DEBUG
	if (_PC_==_Trap)
		_Trace=1;
	if (_Trace)
		if (!DebugTrace(R))
			return(_PC_);
#endif
//	printf("%04x ",_PC_);
/*
	if (!bTrace && iCount++ >= 100)
	{
		iCount = 0;
		if (GetKeyState('T') & 0x80)
			bTrace = TRUE;
	}
	if (bTrace)
	{
		*p++ = _PC_;
		if (p >= pp + 45000)
		{
			int		i, j;
			FILE *fp = fopen("c:\\temp.out", "w");
			ASSERT(fp != NULL);
			fprintf(fp, "pc = ");
			for (i = -8*256; i < 0; i++)
			{
				fprintf(fp, "%04X ", p[i]);
				if ((i&7) == 7)
					fprintf(fp, "\n");
			}
			fprintf(fp, "\nmem = ");
			for (i = -8; i < 0; i++)
			{
				fprintf(fp, "\n");
				for (j = 0; j < 4; j++)
					fprintf(fp, "%02X ", Page[(p[i]+j)>>13][p[i]+j]);
			}
			fprintf(fp, "\npage = ");
			for (i = 0; i < 8; i++)
				fprintf(fp, "%02X", R->MPR[i]);
			fclose(fp);
			TRACE("Break\n");
		}
	}
*/
    I=Op6502(_PC_++);
//    _ICount-=Cycles[I];
	cycle = Cycles[I];

//  Serial.printf("[PC:%x] %x \n",(_PC_ -1), I);

    switch(I)
    {
#ifdef HuC6280
 #include "HuCodes.h"
#endif
#include "Codes.h"
    }

	_ICount-=cycle;
#ifdef HuC6280
//	*(DWORD*)&_User+=cycle;
	_CycleCount += cycle;
#endif

    /* If cycle counter expired... */
    if(_ICount<=0)
    {
      /* If we have come after CLI, get INT_? from IRequest */
      /* Otherwise, get it from the loop handler            */
      if(_AfterCLI)
      {
#ifdef HuC6280
		if (_IRequest&INT_TIMER) {
			_IRequest&=~INT_TIMER;
			I=INT_TIMER;
		} else
#endif
		if (_IRequest&INT_IRQ) {
			_IRequest&=~INT_IRQ;
			I=INT_IRQ;
		}
#ifdef HuC6280
		else if (_IRequest&INT_IRQ2) {
			_IRequest&=~INT_IRQ2;
			I=INT_IRQ2;
		}
#endif
		_ICount = 0;
		if (_IRequest==0)
		{
	        _ICount=_IBackup;  /* Restore the ICount        */
	        _AfterCLI=0;            /* Done with AfterCLI state  */
		}
      }
      else
      {
        I=Loop6502(R);            /* Call the periodic handler */
        _ICount+=_IPeriod;     /* Reset the cycle counter   */
      }

      if(I==INT_QUIT) return(_PC_); /* Exit if INT_QUIT     */
      if(I) Int6502(R,I);              /* Interrupt if needed  */ 

	  if ((DWORD)(_CycleCount-CycleCountOld) > (DWORD)TimerPeriod*2)
		CycleCountOld = _CycleCount;
   }
#ifdef HuC6280
	else {
		if (_CycleCount-CycleCountOld >= TimerPeriod) {
			CycleCountOld += TimerPeriod;
			I=TimerInt(R);
			if(I) Int6502(R,I);
		}
	}
#endif
  }


  /* Execution stopped */
  return(_PC_);
}
