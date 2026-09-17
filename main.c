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
volatile uint32_t system_ms = 0;//主循环之外的地方也会修改 那就要volatile

void SysTick_Init(){
    DL_SYSTICK_config(CPUCLK_FREQ / 1000U);
}

void SysTick_Handler(){
    system_ms++;
}

uint32_t millis(){
    return system_ms;
}


void LED_Init(){
    DL_GPIO_initDigitalOutput(IOMUX_PINCM31);
    DL_GPIO_clearPins(GPIOB,DL_GPIO_PIN_14);
    DL_GPIO_enableOutput(GPIOB,DL_GPIO_PIN_14);
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

bool pressed = false;

typedef enum {
    none = 0,
    sshort,
    llong,
    release
}Btn_Event;

Btn_Event event;


bool erace_Bounce(){

    // if((DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) == none))//这样的完全能理解 但是为什么
    // if ((DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) & DL_GPIO_PIN_21) == none){// & 21是什么操作
    
    //cur_state = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21); 我大概理解了 返回的数据 或者说 readpin替我做的是 把反馈回来的 需要按位运算的过程跳过了 上面的if就是把这个函数做过的给我看到了部分
    //cur_state = (uint8_t)DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21);//为什么还要！0  既然知道类型不匹配 我有只是需要确认是否按下 直接强制转换一下类型 这样只要不是0 那就是没有按下 接地了 那就一定是0
    //错误 强转和直接用错误的类型赋值一样 把uint32赋值给 uint16会导致32位数据被截断 强转也会 

    // pressed = ((DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) & DL_GPIO_PIN_21) == 0U);//按下是0 0!=0是0 所以pressed就是字面意思
static bool last_press = false , pressed = false;
// static bool cur_presse = false;错误
bool cur_presse = false;
static uint32_t change_time = 0;

cur_presse = ((DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) & DL_GPIO_PIN_21) == 0U);

    if(cur_presse != last_press){//对着答案反推很容易 只要状态不改变就不需要重新赋值change 然后只需要判断change是否持续10ms没有change就好了  但是确实想不到
        last_press = cur_presse;
        change_time = millis();
    }

    if((millis() - change_time) >= 10U){
        pressed = cur_presse;
    }

    return pressed;
}

volatile uint32_t lllong,ssshort,nnone;

Btn_Event Btn_state(){
    static bool pre_press = false,pressing = false;
    // uint32_t press_start_time = 0;错误
    static uint32_t press_start_time = 0;

                // static uint8_t lllong = 0 ;
                //             static uint8_t ssshort = 0 ; 依旧volatile
            

    //event = none; 不能加这个 加了程序就 长按没有反应<---错误
    event = none;

    pressed = erace_Bounce();

    if(pressed == 0 && pre_press == 0){
        event = none;
    }

    if(pressed == 1 && pre_press == 0){
        pre_press = pressed;
        press_start_time = millis();
        pressing = false;
    }

    if(pressed == 1 && pre_press == 1){
        if(((millis() - press_start_time) >= 600U) && !pressing){
            // press_start_time = millis();错误
            event = llong;
            pressing = true;
        }
    }

    if(pressed == 0 && pre_press == 1){
        pre_press = pressed;
        if(pressing){
            pressing = false;
            event = none;            
        }else{
            event = sshort;
        }
        // if(event == sshort){
        //     event = none;
        //     return sshort;
        // }
    }

    return event;
}


uint32_t cur_t = 0;

void T_led(){
    if((millis() - cur_t) >= 500U){
        cur_t = millis();
        DL_GPIO_togglePins(GPIOB, DL_GPIO_PIN_14);
    } 
}


void LED_State(){
   // uint8_t state = Btn_state();
    Btn_Event e = Btn_state();

    if(e == llong){
        lllong++;
    }
    if(e == sshort){
        ssshort++;
    }
    if(e == none){
        nnone++;
    }

    // switch(state){
    //     case none:
    //     //    T_led();
    //        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_14);
    //         // DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_14);        
    //     break;

    //     case sshort:
    //         //   DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_14); 
    //         DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_14);
    //     // T_led();
    //     break;

    //     case llong:
    //         // DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_14);
    //          T_led();
    //     break;

    //     case release:
    //     // DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_14);
    //         event = none;
    //     break;
    // }    
}



int main(void)
{
    SYSCFG_DL_init();
    SysTick_Init();

    LED_Init();
    Btn_Init();
    
 
    while (1) {
        LED_State();
    }
}
