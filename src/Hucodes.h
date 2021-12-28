/** Modyfied for M5Stack @shikarunochi  2021                **/
/* HuC6280 additional */
/* BRK addres = 0xFFF6 */

#define MC_Id(Rg)	K.W=MCZp(); \
			Rg.B.l=RdRAM(K.W);Rg.B.h=RdRAM(K.W+1)
#define MR_Id(Rg)	MC_Id(J);Rg=Rd6502(J.W)
//#define	M_FL2(Rg)	_P&=~(Z_FLAG|N_FLAG|V_FLAG);_P|=(Rg&0xc0)|ZNTable[Rg]
#define	TSB(Rg)	\
	_NF=_VF=Rg; \
	Rg|=_A; _ZF=Rg;
#define	TRB(Rg) \
	_NF=_VF=Rg; \
	Rg&=~_A; _ZF=Rg;
#define T_INIT() \
	word_s src,dist,length; \
	src  = Op6502w(_PC_); _PC_+=2; \
	dist = Op6502w(_PC_); _PC_+=2; \
	length = Op6502w(_PC_); _PC_+=2; \
	cycle=length*6
#define M_ADCx(Rg) \
 { byte *Mx=AdrRAM(_X+ZP); \
  if(_P&D_FLAG) \
  { \
    K.B.l=(*Mx&0x0F)+(Rg&0x0F)+(_P&C_FLAG); \
    K.B.h=(*Mx>>4)+(Rg>>4);/*+(K.B.l>>4);*/ \
    if(K.B.l>9) { K.B.l+=6;K.B.h++; } \
    if(K.B.h>9) K.B.h+=6; \
    *Mx=(K.B.l&0x0F)|(K.B.h<<4); \
    _P=(_P&~C_FLAG)|(K.B.h>15? C_FLAG:0); \
	_ZF=_NF=*Mx; \
	cycle++; \
  } \
  else \
  { \
    K.W=*Mx+Rg+(_P&C_FLAG); \
    _P&=~C_FLAG; \
    _P|=(K.B.h? C_FLAG:0); \
	_VF=(~(*Mx^Rg)&(*Mx^K.B.l))>>1; \
	_ZF=_NF=K.B.l; \
    *Mx=K.B.l; \
  } \
 }

#define M_ANDx(Rg)	*AdrRAM(_X+ZP)&=Rg;M_FL(RdRAM(_X+ZP))
#define M_EORx(Rg)	*AdrRAM(_X+ZP)^=Rg;M_FL(RdRAM(_X+ZP))
#define M_ORAx(Rg)	*AdrRAM(_X+ZP)|=Rg;M_FL(RdRAM(_X+ZP))

case 0xD4:break; /* CSH set clock highspeed */
case 0x54:break; /* CSL set clock lowspeed */

case 0x44: M_PUSH(_PC.B.h);M_PUSH(_PC.B.l); /* BSR * REL */
case 0x80: _PC_+=(offset)Op6502(_PC_)+1; break; /* BRA * REL */
/* JMP ($ssss,x) */
case 0x7C:
	M_LDWORD(K);K.W+=_X;
	_PC.B.l = Op6502(K.W);
	_PC.B.h = Op6502(K.W+1);
  break;

case 0xDA: M_PUSH(_X);break;               /* PHX */
case 0x5A: M_PUSH(_Y);break;               /* PHY */
case 0xFA: M_POP(_X);M_FL(_X);break;       /* PLX */
case 0x7A: M_POP(_Y);M_FL(_Y);break;       /* PLY */

case 0x62: _A=0; break; /* CLA */
case 0x82: _X=0; break; /* CLX */
case 0xC2: _Y=0; break; /* CLY */

case 0x02: I=_X;_X=_Y;_Y=I;break;  /* SXY */
case 0x22: I=_A;_A=_X;_X=I;break;  /* SAX */
case 0x42: I=_A;_A=_Y;_Y=I;break;  /* SAY */

case 0x3A: M_DEC(_A); break; /* DEC A */
case 0x1A: M_INC(_A); break; /* INC A */

case 0x72: MR_Id(I);M_ADC(I);break;       /* ADC ($ss) INDIR */
case 0x32: MR_Id(I);M_AND(I);break;       /* AND ($ss) INDIR */
case 0xD2: MR_Id(I);M_CMP(_A,I);break;       /* CMP ($ss) INDIR */
case 0x52: MR_Id(I);M_EOR(I);break;       /* EOR ($ss) INDIR */
case 0xB2: MR_Id(_A);M_FL(_A);break;       /* LDA ($ss) INDIR */
case 0x12: MR_Id(I);M_ORA(I);break;       /* ORA ($ss) INDIR */
case 0x92: MC_Id(J);Wr6502(J.W,_A);break;  /* STA ($ss) INDIR */
case 0xF2: MR_Id(I);M_SBC(I);break;      /* SBC ($ss) INDIR */

case 0x89: MR_Im(I);M_BIT(I);break;       /* BIT #$ss IMM */
case 0x34: MR_Zx(I);M_BIT(I);break;       /* BIT $ss,x ZP,x */
case 0x3C: MR_Ax(I);M_BIT(I);break;       /* BIT $ssss,x ABS,x */

case 0x64: MW_Zp(0x00);break;             /* STZ $ss ZP */
case 0x74: MW_Zx(0x00);break;             /* STZ $ss,x ZP,x */
case 0x9C: MW_Ab(0x00);break;             /* STZ $ssss ABS */
case 0x9E: MW_Ax(0x00);break;             /* STZ $ssss,x ABS,x */

case 0xF4: /* SET */
	I=Op6502(_PC_++);
	cycle+=Cycles[I]+3;
	switch(I){
	case 0x65: MR_Zp(I);M_ADCx(I);break; /* ADC $ss ZP */
	case 0x6D: MR_Ab(I);M_ADCx(I);break; /* ADC $ssss ABS */
	case 0x69: MR_Im(I);M_ADCx(I);break; /* ADC #$ss IMM */
	case 0x75: MR_Zx(I);M_ADCx(I);break; /* ADC $ss,x ZP,x */
	case 0x79: MR_Ay(I);M_ADCx(I);break; /* ADC $ssss,y ABS,y */
	case 0x7D: MR_Ax(I);M_ADCx(I);break; /* ADC $ssss,x ABS,x */
	case 0x61: MR_Ix(I);M_ADCx(I);break; /* ADC ($ss,x) INDEXINDIR */
	case 0x71: MR_Iy(I);M_ADCx(I);break; /* ADC ($ss),y INDIRINDEX */
	case 0x72: MR_Id(I);M_ADCx(I);break; /* ADC ($ss) INDIR */

	case 0x25: MR_Zp(I);M_ANDx(I);break; /* AND $ss ZP */
	case 0x2D: MR_Ab(I);M_ANDx(I);break; /* AND $ssss ABS */
	case 0x29: MR_Im(I);M_ANDx(I);break; /* AND #$ss IMM */
	case 0x35: MR_Zx(I);M_ANDx(I);break; /* AND $ss,x ZP,x */
	case 0x39: MR_Ay(I);M_ANDx(I);break; /* AND $ssss,y ABS,y */
	case 0x3D: MR_Ax(I);M_ANDx(I);break; /* AND $ssss,x ABS,x */
	case 0x21: MR_Ix(I);M_ANDx(I);break; /* AND ($ss,x) INDEXINDIR */
	case 0x31: MR_Iy(I);M_ANDx(I);break; /* AND ($ss),y INDIRINDEX */
	case 0x32: MR_Id(I);M_ANDx(I);break; /* AND ($ss) INDIR */

	case 0x45: MR_Zp(I);M_EORx(I);break; /* EOR $ss ZP */
	case 0x4D: MR_Ab(I);M_EORx(I);break; /* EOR $ssss ABS */
	case 0x49: MR_Im(I);M_EORx(I);break; /* EOR #$ss IMM */
	case 0x55: MR_Zx(I);M_EORx(I);break; /* EOR $ss,x ZP,x */
	case 0x59: MR_Ay(I);M_EORx(I);break; /* EOR $ssss,y ABS,y */
	case 0x5D: MR_Ax(I);M_EORx(I);break; /* EOR $ssss,x ABS,x */
	case 0x41: MR_Ix(I);M_EORx(I);break; /* EOR ($ss,x) INDEXINDIR */
	case 0x51: MR_Iy(I);M_EORx(I);break; /* EOR ($ss),y INDIRINDEX */
	case 0x52: MR_Id(I);M_EORx(I);break; /* EOR ($ss) INDIR */

	case 0x05: MR_Zp(I);M_ORAx(I);break; /* ORA $ss ZP */
	case 0x0D: MR_Ab(I);M_ORAx(I);break; /* ORA $ssss ABS */
	case 0x09: MR_Im(I);M_ORAx(I);break; /* ORA #$ss IMM */
	case 0x15: MR_Zx(I);M_ORAx(I);break; /* ORA $ss,x ZP,x */
	case 0x19: MR_Ay(I);M_ORAx(I);break; /* ORA $ssss,y ABS,y */
	case 0x1D: MR_Ax(I);M_ORAx(I);break; /* ORA $ssss,x ABS,x */
	case 0x01: MR_Ix(I);M_ORAx(I);break; /* ORA ($ss,x) INDEXINDIR */
	case 0x11: MR_Iy(I);M_ORAx(I);break; /* ORA ($ss),y INDIRINDEX */
	case 0x12: MR_Id(I);M_ORAx(I);break; /* ORA ($ss) INDIR */

	default:
		TRACE("no sense SET\n");
		cycle-=Cycles[I];
		_PC_--;
		break;
	}
	break;

case 0x03: IO_write(0,Op6502(_PC_++));break; /* ST0 */
case 0x13: IO_write(2,Op6502(_PC_++));break; /* ST1 */
case 0x23: IO_write(3,Op6502(_PC_++));break; /* ST2 */

case 0x43: /* TMAi */
	I=Op6502(_PC_++);
	{int i;
	  for(i=0;i<8;i++,I>>=1){
		if (I&1) break;
	  }
	  _A = R->MPR[i];
	}
	break;

case 0x53: /* TAMi */
	I=Op6502(_PC_++);
	{int i;
	 for(i=0;i<8;i++,I>>=1){
	 	if (I&1) {
	 		R->MPR[i]=_A;
	 		BANK_SET(i,_A);
	 	}
	 }
#ifdef _DEBUG
/*	 extern int ROM_size;
	 if (ROM_size <= _A && _A < 0xF7)
	 {
		TRACE("Illegal TAMi: A=%02X, PC=%04X\n", _A, _PC_);
		for (i = 0; i < 8; i++)
			TRACE("%02X ", R->MPR[i]);
		TRACE("\n");
	 }
*//*
	 if ((_A&0xF0) == 0x40)
	 {
		 for (i = 0; i < 8; i++)
			TRACE("%02X ", R->MPR[i]);
		TRACE("\n");
	 }
*/
#endif
	}
	break;

case 0xC3: /* TDD */
	{ T_INIT();
	do {
		Wr6502(dist--,Rd6502(src));
		src--;
	} while(--length);
	}
	break;

case 0x73: /* TII */
	{ T_INIT();
	do {
		Wr6502(dist++,Rd6502(src));
		src++;
	} while(--length);
	}
	break;

case 0xE3: /* TIA */
	{ T_INIT();
	do {
		Wr6502(dist  ,Rd6502(src));
		src++;
		if (!(--length)) break;
		Wr6502(dist+1,Rd6502(src));
		src++;
	} while(--length);
	}
	break;

case 0xF3: /* TAI */
	{ T_INIT();
	do {
		Wr6502(dist++,Rd6502(src));
		if (!(--length)) break;
		Wr6502(dist++,Rd6502(src+1));
	} while(--length);
	}
	break;

case 0xD3: /* TIN */
	{ T_INIT();
	do {
		Wr6502(dist,Rd6502(src));
		src++;
	} while(--length);
	}
	break;

case 0x14: MM_Zp(TRB);break; /* TRB $ss ZP */
case 0x1C: MM_Ab(TRB);break; /* TRB $ssss ABS */

case 0x04: MM_Zp(TSB);break; /* TSB $ss ZP */
case 0x0C: MM_Ab(TSB);break; /* TSB $ssss ABS */

case 0x83: /* TST #$ss,$ss IMM,ZP */
	I=Op6502(_PC_++); J.B.l=RdRAM(MCZp());
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	break;
case 0xA3: /* TST #$ss,$ss,x IMM,ZP,x */
	I=Op6502(_PC_++); J.B.l=RdRAM(MCZx());
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	break;
case 0x93: /* TST #$ss,$ssss IMM,ABS */
	I=Op6502(_PC_++); MR_Ab(J.B.l);
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	break;
case 0xB3: /* TST #$ss,$ssss,x IMM,ABS,x */
	I=Op6502(_PC_++);MR_Ax(J.B.l);
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	break;

case 0x0F: if (RdRAM(MCZp())&0x01) _PC_++; else { M_JR; } break; /* BBRi */
case 0x1F: if (RdRAM(MCZp())&0x02) _PC_++; else { M_JR; } break;
case 0x2F: if (RdRAM(MCZp())&0x04) _PC_++; else { M_JR; } break;
case 0x3F: if (RdRAM(MCZp())&0x08) _PC_++; else { M_JR; } break;
case 0x4F: if (RdRAM(MCZp())&0x10) _PC_++; else { M_JR; } break;
case 0x5F: if (RdRAM(MCZp())&0x20) _PC_++; else { M_JR; } break;
case 0x6F: if (RdRAM(MCZp())&0x40) _PC_++; else { M_JR; } break;
case 0x7F: if (RdRAM(MCZp())&0x80) _PC_++; else { M_JR; } break;

case 0x8F: if (RdRAM(MCZp())&0x01) { M_JR; } else _PC_++; break; /* BBSi */
case 0x9F: if (RdRAM(MCZp())&0x02) { M_JR; } else _PC_++; break;
case 0xAF: if (RdRAM(MCZp())&0x04) { M_JR; } else _PC_++; break;
case 0xBF: if (RdRAM(MCZp())&0x08) { M_JR; } else _PC_++; break;
case 0xCF: if (RdRAM(MCZp())&0x10) { M_JR; } else _PC_++; break;
case 0xDF: if (RdRAM(MCZp())&0x20) { M_JR; } else _PC_++; break;
case 0xEF: if (RdRAM(MCZp())&0x40) { M_JR; } else _PC_++; break;
case 0xFF: if (RdRAM(MCZp())&0x80) { M_JR; } else _PC_++; break;

#define	M_RMB(n)	*AdrRAM(MCZp())&=~n
#define	M_SMB(n)	*AdrRAM(MCZp())|=n

case 0x07: M_RMB(0x01); break; /* RMBi */
case 0x17: M_RMB(0x02); break; /* RMBi */
case 0x27: M_RMB(0x04); break; /* RMBi */
case 0x37: M_RMB(0x08); break; /* RMBi */
case 0x47: M_RMB(0x10); break; /* RMBi */
case 0x57: M_RMB(0x20); break; /* RMBi */
case 0x67: M_RMB(0x40); break; /* RMBi */
case 0x77: M_RMB(0x80); break; /* RMBi */

case 0x87: M_SMB(0x01); break; /* SMBi */
case 0x97: M_SMB(0x02); break; /* SMBi */
case 0xA7: M_SMB(0x04); break; /* SMBi */
case 0xB7: M_SMB(0x08); break; /* SMBi */
case 0xC7: M_SMB(0x10); break; /* SMBi */
case 0xD7: M_SMB(0x20); break; /* SMBi */
case 0xE7: M_SMB(0x40); break; /* SMBi */
case 0xF7: M_SMB(0x80); break; /* SMBi */
