/**********************************************************************************************************************
 * \file MPU_Memory_Protection.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are solely in the form of
 * machine-executable object code generated by a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *********************************************************************************************************************/

#ifndef MPU_MEMORY_PROTECTION_H_
#define MPU_MEMORY_PROTECTION_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxPort.h"
#include "IfxCpu_reg.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define DPR_GRANULARITY                 8                           /* Data Protection Range granularity in bytes   */
#define CPR_GRANULARITY                 32                          /* Code Protection Range granularity in bytes   */

/* Data Protection Ranges */
#define DATA_PROTECTION_RANGE_0         0
#define DATA_PROTECTION_RANGE_1         1
#define DATA_PROTECTION_RANGE_2         2
#define DATA_PROTECTION_RANGE_3         3
#define DATA_PROTECTION_RANGE_4         4
#define DATA_PROTECTION_RANGE_5         5
#define DATA_PROTECTION_RANGE_6         6
#define DATA_PROTECTION_RANGE_7         7
#define DATA_PROTECTION_RANGE_8         8
#define DATA_PROTECTION_RANGE_9         9
#define DATA_PROTECTION_RANGE_10        10
#define DATA_PROTECTION_RANGE_11        11
#define DATA_PROTECTION_RANGE_12        12
#define DATA_PROTECTION_RANGE_13        13
#define DATA_PROTECTION_RANGE_14        14
#define DATA_PROTECTION_RANGE_15        15
#define DATA_PROTECTION_RANGE_16        16
#define DATA_PROTECTION_RANGE_17        17

/* Code Protection Ranges */
#define CODE_PROTECTION_RANGE_0         0
#define CODE_PROTECTION_RANGE_1         1
#define CODE_PROTECTION_RANGE_2         2
#define CODE_PROTECTION_RANGE_3         3
#define CODE_PROTECTION_RANGE_4         4
#define CODE_PROTECTION_RANGE_5         5
#define CODE_PROTECTION_RANGE_6         6
#define CODE_PROTECTION_RANGE_7         7
#define CODE_PROTECTION_RANGE_8         8
#define CODE_PROTECTION_RANGE_9         9

/* Protection Sets */
#define PROTECTION_SET_0                0
#define PROTECTION_SET_1                1
#define PROTECTION_SET_2                2
#define PROTECTION_SET_3                3
#define PROTECTION_SET_4                4
#define PROTECTION_SET_5                5

/* LEDs */
#define LED_FIRST_HALF                  &MODULE_P13,0
#define LED_SECOND_HALF                 &MODULE_P13,1

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
/* MPU control functions */
void enable_memory_protection(void);
void define_data_protection_range(uint32 lowerBoundAddress, uint32 upperBoundAddress, uint8 range);
void define_code_protection_range(uint32 lowerBoundAddress, uint32 upperBoundAddress, uint8 range);
void enable_data_read(uint8 protectionSet, uint8 range);
void enable_data_write(uint8 protectionSet, uint8 range);
void enable_code_execution(uint8 protectionSet, uint8 range);

/* LED control functions */
void init_LEDs(void);
void switch_LED_ON(Ifx_P *ledPort, uint8 ledPinIndex);

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* Function to set the given Protection Set as active.
 * This function needs to be declared as inline because the Program Status Word (PSW) is one of the registers
 * automatically saved to the Context Save Area (CSA) when a function is called.
 * If this function was not declared as inline, the Upper Context (16 registers including the PSW) would be
 * automatically saved to the CSA and re-loaded when the function return, thus losing the change in the PSW.
 */
IFX_INLINE void set_active_protection_set(uint8 protectionSet)
{
    Ifx_CPU_PSW PSWRegisterValue;
    PSWRegisterValue.U = __mfcr(CPU_PSW);               /* Get the Program Status Word (PSW) register value         */
    PSWRegisterValue.B.PRS = protectionSet;             /* Set the PRS bitfield to enable the Protection Set        */
    __mtcr(CPU_PSW, PSWRegisterValue.U);                /* Set the Program Status Word (PSW) register               */
}

#endif /* MPU_MEMORY_PROTECTION_H_ */