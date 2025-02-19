/*******************************************************************************************
 File Name: HWI.h

 Version: 1.00

 Discription:


 Author:yulin deng

 Email :flowingfeeze@163.com

 Date:2013-09-14 16:47:39

 Copyright:(c)JIELI  2011  @ , All Rights Reserved.
*******************************************************************************************/
#ifndef HWI_h
#define HWI_h


void hwi_init(void);

#define TIME0_INIT  2
#define TIME1_INIT  3
#define TIME2_INIT  4
#define TIME3_INIT  5
#define USB_SOF     6
#define USB_CTRL    7
//#define IRTC_INIT   8
#define ALINK_INIT  9
#define DAC_INIT    10
#define SPI0_INIT   12
#define SPI1_INIT   13
#define SOFT_INIT   31
#define SD0_INIT    14
#define SD1_INIT    15
#define UART0_INIT  16

#define REG_INIT_HANDLE(a)\
    extern void _##a();\
    void handle__##a()\
    {\
        asm volatile ("call _ISR_PUSH_REG ; ") ;asm("link 12 ; call %0; unlink;  \t\t ; call _ISR_POP_REG ;": : "i" (a )); ; \
    }

#define INTALL_HWI(a,b,c)\
    extern void handle__##b();\
    HWI_Install(a,handle__##b,c)

#define ENABLE_INT()   asm volatile ("[--sp] =r0 ; r0 = retx ;bitset(r0, 8)  ;bitclr(r0, 9) ; retx = r0 ;r0 =[sp++];")
#define DISABLE_INT()   asm volatile ("[--sp] =r0 ; r0 = retx ;bitclr(r0, 8) ; retx = r0 ;r0 =[sp++];")



extern void hwi_init() ;

#endif
