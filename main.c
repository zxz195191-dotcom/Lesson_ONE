/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "ti_msp_dl_config.h"


// uint32_t system_ms = 0; 错误
volatile uint32_t system_ms = 0;//因为我其实一直都是 电容是用来滤波的 一样的感觉 volatile是防止编译器自作主张的 但其实就像没有用过示波器感受电容带来的差异一样 自作聪明其实很难给我直观感受 所以我不太能确定什么时候需要volatile

void SysTick_Init(){//并不是错误的理解成一次循环就是1ms 我应该在某个地方有提到 不清楚怎么实现1ms反馈 特意注释600ms也是告诉 我能理解这里放的应该是什么 不过没理解实现
    DL_SYSTICK_config(CPUCLK_FREQ / 1000U);
}

void SysTick_Handler(){
    system_ms++;
}

uint32_t millis(){
    return system_ms;
}

bool countdown(uint32_t Delay_ms,uint32_t start_time){
    start_time = millis();
    if(millis() - start_time >= Delay_ms) return true;
    else return false;
}

typedef enum{
    none = 0,
    press,
    sshort,
    llong,
}Btn_STATE;


void Enable(GPIO_Regs *gpio, uint32_t pins){
    DL_GPIO_clearPins(gpio,pins);
    DL_GPIO_enableOutput(gpio,pins);
}

void LED_Init(){
    DL_GPIO_initDigitalOutput(IOMUX_PINCM31);//就是说 这个引脚本身还有机会去被当作pwm tim之类的功能 现在我只是需要gpio 所以开始就声明清楚
    Enable(GPIOB,DL_GPIO_PIN_14);
}


    uint8_t cur_state = 0,pre_state = 0;
    uint8_t real_state = none;
    uint32_t press_time = 0;

uint8_t DeBounce(){

   // if((DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) == none))//这样的完全能理解 但是为什么
    // if ((DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) & DL_GPIO_PIN_21) == none){// & 21是什么操作
    
   // cur_state = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21); 我大概理解了 返回的数据 或者说 readpin替我做的是 把反馈回来的 需要按位运算的过程跳过了 上面的if就是把这个函数做过的给我看到了部分
//    cur_state = (uint8_t)DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21);//为什么还要！0  既然知道类型不匹配 我有只是需要确认是否按下 直接强制转换一下类型 这样只要不是0 那就是没有按下 接地了 那就一定是0
//错误 强转和直接用错误的类型赋值一样 把uint32赋值给 uint16会导致32位数据被截断 强转也会 
    cur_state = (DL_GPIO_readPins(GPIOB,DL_GPIO_PIN_21) != 0U);
   
    if(cur_state != pre_state){
        if(countdown(10,press_time) ){
            press_time = 0 ;
            uint8_t temp =  (uint8_t)DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21);

            if(cur_state == temp){                           
                real_state = temp;
            }else{
                real_state = pre_state;
            }
            
        }
    }

    pre_state = cur_state;
    return real_state;
}

uint32_t pressed_time = 0;
uint32_t release_time = 0;

uint8_t Btn_state(){
    uint8_t key_state = DeBounce();
    
    if(key_state){
        pressed_time = millis();
        if(pressed_time - release_time >= 600){
            key_state = llong;
        }
    }else{
        release_time = millis();
        if(release_time - pressed_time <= 600){
            key_state = sshort;
        }
    }

    return key_state;
}


void Btn_Init(){
    DL_GPIO_initDigitalInputFeatures(
    IOMUX_PINCM49,
    DL_GPIO_INVERSION_DISABLE,
    DL_GPIO_RESISTOR_PULL_UP,
    DL_GPIO_HYSTERESIS_DISABLE,
    DL_GPIO_WAKEUP_DISABLE
    );
}



volatile uint32_t cur_t = 0;


void T_led(){
    if((millis() - cur_t) >= 500U){
        cur_t = millis();
        DL_GPIO_togglePins(GPIOB, DL_GPIO_PIN_14);
    } 
}


void LED_State(){
    uint8_t state = Btn_state();

    switch(state){
        case none:
        T_led();
        break;

        case sshort:
            DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_14);
        // T_led();
        break;

        case llong:
            DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_14); 
            // T_led();
        break;
    }    
}
//git commit -m "目前试图用一个变量同时表示按下和状态两种不同类型的意义，导致冲突；关于 volatile，只要可能有主循环以外的因素改变这个数值，就需要 volatile"



int main(void)
{
    SYSCFG_DL_init();
    SysTick_Init();

    LED_Init();
    Btn_Init();
    

    cur_t = millis();
    while (1) {
        LED_State();
    }
}
