/**********************************************************************************************************************
 * \file ADC_Filtering.c
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

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "ADC_Filtering.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
/* ADC parameters */
#define CHANNELS_NUM                4       /* Number of used channels                                              */
#define GROUPS_NUM                  3       /* Number of used groups (group 0, 1, 2)                                */

/* Possible values for FIR/IIR filter coefficients:
 * --------------------------------------------------------------------------------------------------------
 * |          Average filter          | * |          FIR filter         | * |         IIR filter          |
 * ------------------------------------ * ------------------------------- * -------------------------------
 * | DMM | DRCTR | Accumulated values | * | DMM | DRCTR | COEFFICIENTS  | * | DMM | DRCTR | COEFFICIENTS  |
 * ------------------------------------ * ------------------------------- * -------------------------------
 * | 0x0 |  0x0  | Disabled           | * | 0x1 |  0x0  | a=2, b=1, c=0 | * | 0x1 |  0xE  | a=2, b=2      |
 * | 0x0 |  0x1  | 2 results average  | * | 0x1 |  0x1  | a=1, b=2, c=0 | * |>0x1 |  0xF  | a=3, b=4      |
 * | 0x0 |  0x2  | 3 results average  | * | 0x1 |  0x2  | a=2, b=0, c=1 | * |     |       |               |
 * |>0x0 |  0x3  | 4 results average  | * | 0x1 |  0x3  | a=1, b=1, c=1 | * |     |       |               |
 * | 0x0 |  0x2  | 5 results average  | * |>0x1 |  0x4  | a=1, b=0, c=2 | * |     |       |               |
 * | 0x0 |  0x2  | 6 results average  | * | 0x1 |  0x5  | a=3, b=1, c=0 | * |     |       |               |
 * | 0x0 |  0x2  | 7 results average  | * | 0x1 |  0x6  | a=2, b=2, c=0 | * |     |       |               |
 * | 0x0 |  0x2  | 8 results average  | * | 0x1 |  0x7  | a=1, b=3, c=0 | * |     |       |               |
 * | 0x0 |  0x2  | 9 results average  | * | 0x1 |  0x8  | a=3, b=0, c=1 | * |     |       |               |
 * | 0x0 |  0x2  | 10 results average | * | 0x1 |  0x9  | a=2, b=1, c=1 | * |     |       |               |
 * | 0x0 |  0x2  | 11 results average | * | 0x1 |  0xA  | a=1, b=2, c=1 | * |     |       |               |
 * | 0x0 |  0x2  | 12 results average | * | 0x1 |  0xB  | a=2, b=0, c=2 | * |     |       |               |
 * | 0x0 |  0x2  | 13 results average | * | 0x1 |  0xC  | a=1, b=1, c=2 | * |     |       |               |
 * | 0x0 |  0x2  | 14 results average | * | 0x1 |  0xD  | a=1, b=0, c=3 | * |     |       |               |
 * | 0x0 |  0x2  | 15 results average | * |     |       |               | * |     |       |               |
 * | 0x0 |  0x2  | 16 results average | * |     |       |               | * |     |       |               |
 * --------------------------------------------------------------------------------------------------------
 *
 * When selecting different coefficients, make sure to set the correct DIV_FACTOR.
 */

/* UART parameters */
#define ISR_PRIORITY_ASCLIN_TX      10      /* Priority of the interrupt ISR Transmit                               */
#define ISR_PRIORITY_ASCLIN_RX      20      /* Priority of the interrupt ISR Receive                                */
#define ISR_PRIORITY_ASCLIN_ER      30      /* Priority of the interrupt ISR Errors                                 */

#define ASC_TX_BUFFER_SIZE          64      /* UART transmission buffer size in bytes                               */
#define ASC_RX_BUFFER_SIZE          64      /* UART reception buffer size in bytes                                  */
#define ASC_PRESCALER               1       /* Division ratio of the predivider for UART communication              */
#define ASC_BAUDRATE                115200  /* Baud rate of the ASCLIN UART communication                           */

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void initEVADCModule(void);                 /* Function to initialize the EVADC module with default parameters      */
void initEVADCGroups(void);                 /* Function to initialize the EVADC group                               */
void initEVADCChannels(void);               /* Function to initialize the EVADC used channels                       */
void applyFiltering(void);                  /* Function to apply the filters to the EVADC channels                  */
void startEVADC(void);                      /* Function to start the scan                                           */

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
/* Variables for the EVADC */
IfxEvadc_Adc g_evadc;                                     /* Global variable for configuring the EVADC module       */
IfxEvadc_Adc_Group g_evadcGroup[GROUPS_NUM];              /* Global array for configuring the EVADC groups          */
IfxEvadc_Adc_Channel g_evadcChannel[CHANNELS_NUM];        /* Global array for configuring the EVADC channels        */

channel g_chn[] = { {&IfxEvadc_G0CH0_AN0_IN,  (IfxEvadc_ChannelResult) 0  },  /* AN0 pin   (Average filter channel) */
                    {&IfxEvadc_G0CH3_AN3_IN,  (IfxEvadc_ChannelResult) 15 },  /* AN3 pin   (IIR filter channel)     */
                    {&IfxEvadc_G1CH5_AN13_IN, (IfxEvadc_ChannelResult) 7  },  /* AN13 pin  (FIR filter channel)     */
                    {&IfxEvadc_G2CH5_AN21_IN, (IfxEvadc_ChannelResult) 1  }   /* AN21 pin  (No data modification)   */
};

/* Variables for UART */
IfxAsclin_Asc g_asc;                                    /* Global variable for configuring the ASCLIN module        */
IfxStdIf_DPipe g_stdInterface;                          /* Global variable for configuring the standard interface   */

/* The transfer buffers allocate memory for the data itself and for the FIFO runtime variables.
 * 8 more bytes have to be added to ensure a proper circular buffer handling independent from the address to which
 * the buffers have been located.
 */
uint8 g_AscTxBuffer[ASC_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];
uint8 g_AscRxBuffer[ASC_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* Function to initialize the EVADC module */
void initADC(void)
{
    initEVADCModule();                                                  /* Initialize the EVADC module              */
    initEVADCGroups();                                                  /* Initialize the EVADC groups              */
    initEVADCChannels();                                                /* Initialize the used channels             */
    applyFiltering();                                                   /* Configure the Data Modification registers*/

    /* Start the scan */
    startEVADC();
}

/* Function to start the scan */
void startEVADC(void)
{
    /* Initialize all the used groups with the same settings */
    uint8 grp;
    for(grp = 0; grp < GROUPS_NUM; grp++)
    {
        /* Start the queue */
        IfxEvadc_Adc_startQueue(&g_evadcGroup[grp], IfxEvadc_RequestSource_queue0);
    }
}

/* Function to initialize the EVADC module with default parameters */
void initEVADCModule(void)
{
    IfxEvadc_Adc_Config adcConf;                                        /* Define a configuration structure         */
    IfxEvadc_Adc_initModuleConfig(&adcConf, &MODULE_EVADC);             /* Fill it with default values              */
    IfxEvadc_Adc_initModule(&g_evadc, &adcConf);                        /* Apply the default configuration          */
}

/* Function to initialize the EVADC groups */
void initEVADCGroups(void)
{
    /* Initialize the groups */
    IfxEvadc_Adc_GroupConfig adcGroupConf;                              /* Define a configuration structure         */
    IfxEvadc_Adc_initGroupConfig(&adcGroupConf, &g_evadc);              /* Fill it with default values              */

    /* Enable queue 0 source */
    adcGroupConf.arbiter.requestSlotQueue0Enabled = TRUE;

    /* Enable the gate in "always" mode (no edge detection) */
    adcGroupConf.queueRequest[0].triggerConfig.gatingMode = IfxEvadc_GatingMode_always;

    /* Set the group 0 as master group */
    adcGroupConf.master = IfxEvadc_GroupId_0;

    /* Initialize all the used groups with the same settings */
    uint8 grp;
    for(grp = 0; grp < GROUPS_NUM; grp++)
    {
        adcGroupConf.groupId = (IfxEvadc_GroupId)grp;                   /* Select the group                         */

        /* Apply the configuration to the group, with group 0 as a master:
         * make sure that the index of the group in the array corresponds to the used group IDs
         */
        IfxEvadc_Adc_initGroup(&g_evadcGroup[grp], &adcGroupConf);
    }

}

/* Function to initialize the used EVADC channels */
void initEVADCChannels(void)
{
    IfxEvadc_Adc_ChannelConfig adcChannelConf;                  /* Configuration structure                          */

    uint16 chnNum;
    for(chnNum = 0; chnNum < CHANNELS_NUM; chnNum++)            /* The channels included in g_chn are initialized   */
    {
        /* Fill the configuration with default values */
        IfxEvadc_Adc_initChannelConfig(&adcChannelConf, &g_evadcGroup[g_chn[chnNum].analogInput->groupId]);

        /* Set the channel ID and the corresponding result register */
        adcChannelConf.channelId = g_chn[chnNum].analogInput->channelId;
        adcChannelConf.resultRegister = g_chn[chnNum].resultRegister;

        /* Apply the channel configuration */
        IfxEvadc_Adc_initChannel(&g_evadcChannel[chnNum], &adcChannelConf);

        /* Add channel to queue with refill option enabled */
        IfxEvadc_Adc_addToQueue(&g_evadcChannel[chnNum], IfxEvadc_RequestSource_queue0, IFXEVADC_QUEUE_REFILL);
    }
}

/* Function to apply the filters to the EVADC channels */
void applyFiltering(void)
{
    /* Accumulate 4 result values within the result register before generating a final result for AN0 */
    EVADC_G0_RCR0.B.DMM = IfxEvadc_DataModificationMode_standardDataReduction;  /* Set the Data Modification Mode bit field to Standard Data Reduction */
    EVADC_G0_RCR0.B.DRCTR = IfxEvadc_DataReductionControlMode_3;                /* Configure the Result Register 0 of Group 0 to accumulate 4 conversions */

    /* Apply a 1st order Infinite Impulse Response Filter (IIR) to the result register G0RES15 (AN3) */
    EVADC_G0_RCR15.B.DMM = IfxEvadc_DataModificationMode_resultFilteringMode;   /* Set the Data Modification Mode bit field to Result filtering mode */
    EVADC_G0_RCR15.B.DRCTR = IfxEvadc_DataReductionControlMode_15;              /* Configure the Result Register 15 of Group 0 to apply an IIR filter (a=3, b=4) */

    /* Apply a 3rd order Finite Impulse Response Filter (FIR) to the result register G0RES7 (AN13) */
    EVADC_G1_RCR7.B.DMM = IfxEvadc_DataModificationMode_resultFilteringMode;    /* Set the Data Modification Mode bit field to Result filtering mode */
    EVADC_G1_RCR7.B.DRCTR = IfxEvadc_DataReductionControlMode_4;                /* Configure the Result Register 7 of Group 1 to apply a FIR filter (a=1, b=0, c=2) */
}

/* Function to read the EVADC measurements */
uint16 readADCValue(uint8 channel)
{
    Ifx_EVADC_G_RES conversionResult;                                   /* Variable to store the conversion result  */
    conversionResult.U = 0;                                             /* Initialize the value to 0                */

    /* Read ADC conversion until a valid one is read.
     * Since the AN0 pin is using the Standard Data Reduction mode, it is needed to check both the Valid Flag (VF == 1)
     * and the Data Reduction Counter (DRC == 0) bit fields to be sure that the read measurement is correct.
     */
    while(conversionResult.B.VF != 1 || conversionResult.B.DRC != 0)
    {
        conversionResult = IfxEvadc_Adc_getResult(&g_evadcChannel[channel]);
    }

    return conversionResult.B.RESULT;
}

/* ASCLIN TX Interrupt Service Routine */
IFX_INTERRUPT(asclinTxISR, 0, ISR_PRIORITY_ASCLIN_TX);

void asclinTxISR(void)
{
    IfxStdIf_DPipe_onTransmit(&g_stdInterface);
}

/* ASCLIN RX Interrupt Service Routine */
IFX_INTERRUPT(asclinRxISR, 0, ISR_PRIORITY_ASCLIN_RX);

void asclinRxISR(void)
{
    IfxStdIf_DPipe_onReceive(&g_stdInterface);
}

/* ASCLIN Error Interrupt Service Routine */
IFX_INTERRUPT(asclinErISR, 0, ISR_PRIORITY_ASCLIN_ER);

void asclinErISR(void)
{
    IfxStdIf_DPipe_onError(&g_stdInterface);
}

/* Function that returns if new data is available */
boolean isDataAvailable(void)
{
    /* If new data is available, return true */
    if(IfxAsclin_Asc_getReadCount(&g_asc) > 0)
    {
        return TRUE;
    }
    else /* Else, return false */
    {
        return FALSE;
    }
}

/* Function to receive data over ASC */
void receiveData(char *data, Ifx_SizeT length)
{
    /* Receive data */
    IfxAsclin_Asc_read(&g_asc, data, &length, TIME_INFINITE);
}

/* Function to initialize the ASCLIN module */
void initUART(void)
{
    IfxAsclin_Asc_Config ascConf;

    IfxAsclin_Asc_initModuleConfig(&ascConf, &MODULE_ASCLIN0);

    /* Set the desired baud rate */
    ascConf.baudrate.prescaler = ASC_PRESCALER;
    ascConf.baudrate.baudrate = ASC_BAUDRATE;
    ascConf.baudrate.oversampling = IfxAsclin_OversamplingFactor_16;            /* Set the oversampling factor      */

    /* Configure the sampling mode */
    ascConf.bitTiming.medianFilter = IfxAsclin_SamplesPerBit_three;             /* Set the number of samples per bit*/
    ascConf.bitTiming.samplePointPosition = IfxAsclin_SamplePointPosition_8;    /* Set the first sample position    */

    /* ISR priorities and interrupt target */
    ascConf.interrupt.txPriority = ISR_PRIORITY_ASCLIN_TX;
    ascConf.interrupt.rxPriority = ISR_PRIORITY_ASCLIN_RX;
    ascConf.interrupt.erPriority = ISR_PRIORITY_ASCLIN_ER;
    ascConf.interrupt.typeOfService = IfxSrc_Tos_cpu0;

    /* FIFO configuration */
    ascConf.txBuffer = g_AscTxBuffer;
    ascConf.txBufferSize = ASC_TX_BUFFER_SIZE;
    ascConf.rxBuffer = g_AscRxBuffer;
    ascConf.rxBufferSize = ASC_RX_BUFFER_SIZE;

    /* Pin configuration */
    const IfxAsclin_Asc_Pins pins = {
            NULL, IfxPort_InputMode_pullUp,                                     /* CTS port pin not used            */
            &IfxAsclin0_RXA_P14_1_IN, IfxPort_InputMode_pullUp,                 /* RX port pin                      */
            NULL, IfxPort_OutputMode_pushPull,                                  /* RTS port pin not used            */
            &IfxAsclin0_TX_P14_0_OUT, IfxPort_OutputMode_pushPull,              /* TX port pin                      */
            IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
    ascConf.pins = &pins;

    /* Initialize the module */
    IfxAsclin_Asc_initModule(&g_asc, &ascConf);

    /* Initialize the Standard Interface */
    IfxAsclin_Asc_stdIfDPipeInit(&g_stdInterface, &g_asc);
}