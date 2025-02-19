/*******************************************************************************************
File Name: 	key_drv_tch.c

Version: 	1.00

Discription AC5100 Toutch Key API

Author:     bingquan Cai

Email:      bingquan_cai@zh-jieli.com

Date:		2013.05.09

Copyright (c) 2010 - 2012 JieLi. All Rights Reserved.
*******************************************************************************************/
#include "key_drv_tch.h"
#include "includes.h"
#include "key.h"
#include "ac46.h"

#if KEY_TCH_EN

u8 g_touch_len;        //触摸按键数
u8 touchkey_short_msg; //触摸按键短按消息位置
u8  bCapState;          //电容充放电状态
u8  bCap_ch;            //触摸通道
u16 Touchkey_value_old,Touchkey_value_new;
sCTM_KEY_VAR ctm_key_value;
u8 touchkey_msg_table[4][10];

#define PA811 0
#define PB811 1

#define TOUCH_IO PA811

sCTM_KEY_VAR *ctm_key_var;


void ctm_key_var_init(sCTM_KEY_VAR *ptr)
{
	ctm_key_var = ptr;
}


static void ctm_irq(u16 ctm_res, u8 ch)
{
    u16 temp_u16_0, temp_u16_1;
    s16 temp_s16_0, temp_s16_1;
    s32 temp_s32_0;
//..............................................................................................
//取计数值/通道判断
//..............................................................................................


    if(ctm_key_var->touch_init_cnt[ch])
    {
        ctm_key_var->touch_init_cnt[ch]--;
//		touch_cnt_buf[ch] = rvalue << FLT0CFG;
//		touch_release_buf[ch] = (long)(rvalue) << FLT1CFG0;
        ctm_key_var->touch_cnt_buf[ch] = (u32)ctm_res<<ctm_key_var->FLT0CFG;
        ctm_key_var->touch_release_buf[ch] = (u32)ctm_res<<ctm_key_var->FLT1CFG0;
    }

//..............................................................................................
//当前计数值去抖动滤波器
//..............................................................................................
    temp_u16_0 = ctm_key_var->touch_cnt_buf[ch];
    temp_u16_1 = temp_u16_0;
    temp_u16_1 -= (temp_u16_1 >> ctm_key_var->FLT0CFG);
    temp_u16_1 += ctm_res;//temp_u16_1 += rvalue;
    ctm_key_var->touch_cnt_buf[ch] = temp_u16_1;
    temp_u16_0 += temp_u16_1;
    temp_u16_0 >>= (ctm_key_var->FLT0CFG + 1);


//..............................................................................................
//各通道按键释放计数值滤波器
//..............................................................................................
    temp_s32_0 = ctm_key_var->touch_release_buf[ch];
    temp_u16_1 = temp_s32_0 >> ctm_key_var->FLT1CFG0;	//获得滤波器之后的按键释放值
    temp_s16_0 = temp_u16_0 - temp_u16_1;	//获得和本次检测值的差值，按下按键为负值，释放按键为正值
    temp_s16_1 = temp_s16_0;

//    	if(ch == 0)
//    	{
//    		printf("ch%d: %d  %d", (short)ch, temp_u16_0, temp_s16_1);
//    	}

    if(ctm_key_var->touch_key_state & BIT(ch))		//如果本通道按键目前是处于释放状态
    {
        if(temp_s16_1 >= 0)		//当前计数值大于低通值，放大后参与运算
        {
            if(temp_s16_1 < (ctm_key_var->FLT1CFG2 >> 3))
            {
                temp_s16_1 <<= 3;	//放大后参与运算
            }
            else
            {
                temp_s16_1 = ctm_key_var->FLT1CFG2;	//饱和，防止某些较大的正偏差导致错判
            }
        }
        else if(temp_s16_1 >= ctm_key_var->FLT1CFG1)	//当前计数值小于低通值不多，正常参与运算
        {
        }
        else				//当前计数值小于低通值很多，缩小后参与运算 (有符号数右移自动扩展符号位???)
        {
            temp_s16_1 >>= 3;
        }
    }
    else			//如果本通道按键目前是处于按下状态, 缓慢降低释放计数值
    {
        if(temp_s16_1 <= ctm_key_var->RELEASECFG1)
        {
            temp_s16_1 >>= 3;		//缩小后参与运算
        }
        else
        {
            temp_s16_1 = 0;
        }
    }

    temp_s32_0 += (s32)temp_s16_1;
    ctm_key_var->touch_release_buf[ch] = temp_s32_0;

//..............................................................................................
//按键按下与释放检测
//..............................................................................................
    if(temp_s16_0 <= ctm_key_var->PRESSCFG)				//按键按下
    {
        ctm_key_var->touch_key_state &= ~BIT(ch);
    }
    else if(temp_s16_0 >= ctm_key_var->RELEASECFG0)		//按键释放
    {
        ctm_key_var->touch_key_state |= BIT(ch);
    }
}


tu8 get_tchkey_value(void)
{
    u8 key;
    u8 i;

    for (i = 0; i < g_touch_len; i++)
    {
        if (!(ctm_key_value.touch_key_state & (u8)(BIT(i))))
            break;
    }
    key = (i < g_touch_len) ? i : NO_KEY;

//    if(key != NO_KEY)
//        printf("tch %x\n", key);
//    else
//        puts("\n");

    return key;
}



void set_port_out(u8 chan)
{
    switch(chan)
    {
#if (TOUCH_IO == PA811)
        case 0:
            PORTA_DIR &= ~BIT(8);
            break;
        case 1:
            PORTA_DIR &= ~BIT(9);
            break;
        case 2:
            PORTA_DIR &= ~BIT(10);
            break;
        case 3:
            PORTA_DIR &= ~BIT(11);
            break;
#else
        case 0:
            PORTB_DIR &= ~BIT(8);
            break;
        case 1:
            PORTB_DIR &= ~BIT(9);
            break;
        case 2:
            PORTB_DIR &= ~BIT(10);
            break;
        case 3:
            PORTB_DIR &= ~BIT(11);
            break;
#endif
    }
}

void set_port_out_H(u8 chan)
{
    switch(chan)
    {
#if (TOUCH_IO == PA811)
        case 0:
            PORTA_OUT |= BIT(8);
            break;
        case 1:
            PORTA_OUT |= BIT(9);
            break;
        case 2:
            PORTA_OUT |= BIT(10);
            break;
        case 3:
            PORTA_OUT |= BIT(11);
            break;
#else
        case 0:
            PORTB_OUT |= BIT(8);
            break;
        case 1:
            PORTB_OUT |= BIT(9);
            break;
        case 2:
            PORTB_OUT |= BIT(10);
            break;
        case 3:
            PORTB_OUT |= BIT(11);
            break;
#endif
    }
}

void set_port_pd(u8 chan)
{
    switch(chan)
    {
#if (TOUCH_IO == PA811)
        case 0:
            PORTA_PD |= BIT(8);
            break;
        case 1:
            PORTA_PD |= BIT(9);
            break;
        case 2:
            PORTA_PD |= BIT(10);
            break;
        case 3:
            PORTA_PD |= BIT(11);
            break;
#else
        case 0:
            PORTB_PD |= BIT(8);
            break;
        case 1:
            PORTB_PD |= BIT(9);
            break;
        case 2:
            PORTB_PD |= BIT(10);
            break;
        case 3:
            PORTB_PD |= BIT(11);
            break;
#endif
    }
}

void set_port_in(u8 chan)
{
    switch(chan)
    {
#if (TOUCH_IO == PA811)
        case 0:
            PORTA_DIR |= BIT(8);
            break;
        case 1:
            PORTA_DIR |= BIT(9);
            break;
        case 2:
            PORTA_DIR |= BIT(10);
            break;
        case 3:
            PORTA_DIR |= BIT(11);
            break;
#else
        case 0:
            PORTB_DIR |= BIT(8);
            break;
        case 1:
            PORTB_DIR |= BIT(9);
            break;
        case 2:
            PORTB_DIR |= BIT(10);
            break;
        case 3:
            PORTB_DIR |= BIT(11);
            break;
#endif
    }
}

u8 g_touch_port(u8 chan)
{
#if (TOUCH_IO == PA811)
    u8 first_chan = 9 -1;
    return (first_chan + chan);
#else
    u8 first_chan = 25 -1;
    return (first_chan + chan);
#endif
}

void set_touch_io(u8 chan_id)
{
    IOMC2 &= ~(0xff0000);
    IOMC2 |= g_touch_port(chan_id)<< 16;    ///选定ＩＯ
    set_port_in(chan_id);///设置为输入
}


void scan_capkey(void)
{
    u16 temp;
    u16 Touchkey_value_delta;

    if(0 == g_touch_len)
    {
        return;
    }

    if (bCapState == 0)
    {
      	bCapState = 1;
        Touchkey_value_new = PLCNTVL;   ///获取计数值

//        if(Touchkey_value_old> Touchkey_value_new)
//            Touchkey_value_new += 0x10000;

        Touchkey_value_delta = Touchkey_value_new - Touchkey_value_old;

//        if(bCap_ch == 0)
//        {
//            printf("\n old: %x   new: %x", Touchkey_value_old, Touchkey_value_new );
//            if(Touchkey_value_delta > 0x100)
//                printf("value: %x\n", Touchkey_value_delta);
//        }

        Touchkey_value_old = Touchkey_value_new;	///记录旧值
        temp = 6800L - Touchkey_value_delta*1;      ///1变化增大倍数

//        printf("\nTouchkey_value_delta: %d  ch: %d", Touchkey_value_delta, bCap_ch);puts("\n");
//        if(bCap_ch == 0)
//        {
//             printf("temp: %d\n", Touchkey_value_delta);//puts("\n");
//        }


        /*调用滤波算法*/
        ctm_irq(temp, bCap_ch);


        /*清除前一个通道状态，锁定PLL CNT*/
        set_port_out(bCap_ch);

        /*切换通道，开始充电，PLL CNT 输入Mux 切换*/
        bCap_ch++;
        bCap_ch %= g_touch_len;
        //bCap_ch = (bCap_ch >= g_touch_len) ? 0 : bCap_ch;

        set_port_out_H(bCap_ch);
        set_port_out(bCap_ch);
    }
    else
    {
    	bCapState = 0;
    	set_touch_io(bCap_ch);
    }
}



void tch_key_init(void)
{
    u8 i;

	ctm_key_var_init(&ctm_key_value);

  	my_memset((u8 *)&ctm_key_value, 0x0, sizeof(sCTM_KEY_VAR));
    /*触摸按键参数配置*/
    ctm_key_value.FLT0CFG = 0;
    ctm_key_value.FLT1CFG0 = 7;
    ctm_key_value.FLT1CFG1 = -80;
    ctm_key_value.FLT1CFG2 = (-(-10)) << 7; //1280

    ///调节灵敏度的主要参数
    ctm_key_value.PRESSCFG = -70;
    ctm_key_value.RELEASECFG0 = -50;
    ctm_key_value.RELEASECFG1 = -80;//-81;

    my_memset((u8 *)&(ctm_key_value.touch_init_cnt[0]), 0x10, TOUCH_KEY_CH_MAX);

    ctm_key_value.touch_key_state = 0xffff; //<按键默认释放

  	///初始化计数器配置：
  	PLCNTCON &= ~(0xc);	//
  	PLCNTCON |= BIT(2);	//选择PLL 96M时钟计数
    PLCNTCON |= BIT(1);	//使能使能计数器

    g_touch_len = TOUCH_KEY_CH_MAX;

    bCapState = 1;
    Touchkey_value_old = PLCNTVL;   ///获取计数值
    set_port_out_H(bCap_ch);
    set_port_out(bCap_ch);

#if 1       //如果外部有下拉电阻，可不是用芯片内部下拉
    for (i = 0; i < g_touch_len; i++)
    {
        set_port_pd(i);
    }
#endif

    puts("touch_key_init \n");
}

#endif
