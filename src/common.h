//    PC Engine emulator M5PCE for M5Stack by @shikarunochi 2021.12-
#ifndef _COMMON_H_
#define _COMMON_H_
#ifndef SUPPORT_CPLUSPLUS_11
	#ifndef int8_t
		typedef signed char int8_t;
	#endif
	#ifndef int16_t
		typedef signed short int16_t;
	#endif
	#ifndef int32_t
		typedef signed int int32_t;
	#endif
	#ifndef int64_t
		typedef signed long long int64_t;
	#endif
	#ifndef uint8_t
		typedef unsigned char uint8_t;
	#endif
	#ifndef uint16_t
		typedef unsigned short uint16_t;
	#endif
	#ifndef uint32_t
		typedef unsigned int uint32_t;
	#endif
	#ifndef uint64_t
		typedef unsigned long long uint64_t;
	#endif
#endif

#ifndef BOOL
    typedef int BOOL;
#endif
#ifndef TRUE
    #define TRUE 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif
#ifndef BYTE
    typedef uint8_t BYTE;
#endif
#ifndef WORD
    typedef uint16_t WORD;
#endif
#ifndef DWORD
    typedef uint32_t DWORD;
#endif

#endif
