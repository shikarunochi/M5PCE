/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                          Tables.h                       **/
/**                                                         **/
/** This file contains tables of used by 6502 emulation to  **/
/** compute NEGATIVE and ZERO flags. There are also timing  **/
/** tables for 6502 opcodes. This file is included from     **/
/** 6502.c.                                                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/** Modified by BERO for HuC6280                            **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/   
/**     changes to this file.                               **/
/*************************************************************/

static byte Cycles[256] =
{
	8,7,3, 4,6,4,6,7,3,2,2,0,7,5,7,6,
	2,7,7, 4,6,4,6,7,2,5,2,0,7,5,7,6,
	7,7,3, 4,4,4,6,7,3,2,2,0,5,5,7,6,
	2,7,7, 0,4,4,6,7,2,5,2,0,5,5,7,6,
	7,7,3, 4,8,4,6,7,3,2,2,0,4,5,7,6,
	2,7,7, 5,0,4,6,7,2,5,3,0,0,5,7,6,
	7,7,2, 0,4,4,6,7,3,2,2,0,7,5,7,6,
	2,7,7,17,4,4,6,7,2,5,3,0,7,5,7,6,
	4,7,2, 7,4,4,4,7,2,2,2,0,5,5,5,6,
	2,7,7, 8,4,4,4,7,2,5,2,0,5,5,5,6,
	2,7,2, 7,4,4,4,7,2,2,2,0,5,5,5,6,
	2,7,7, 8,4,4,4,7,2,5,2,0,5,5,5,6,
	2,7,2,17,4,4,6,7,2,2,2,0,5,5,7,6,
	2,7,7,17,0,4,6,7,2,5,3,0,0,5,7,6,
	2,7,0,17,4,4,6,7,2,2,2,0,5,5,7,6,
	2,7,7,17,2,4,6,7,2,5,3,0,0,5,7,0
};
/*
byte ZNTable[256] =
{
  Z_FLAG,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
  N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
};
*/
