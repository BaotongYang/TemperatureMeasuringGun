#include "Voltage_Driver.h"
#include "Led_Driver.h"
#include <stdio.h>

#define ADC1_DR_ADDRESS     ((uint32_t)0x40012400+0x4c)     // (uint32_t)&ADC1->DR
#define ADC_MAX_CNT     50

uint8_t AdcValueCnt = 0;
__IO uint16_t AdcConvertedValue[ADC_MAX_CNT] = {0};
__IO uint32_t ADC_ConvertedValue;
DMA_HandleTypeDef DMA_Handle;
ADC_HandleTypeDef ADC_Handle;
ADC_ChannelConfTypeDef ADC_Config;


static void Voltage_ADC_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    VOLTAGE_ADC_DMA1_CLK_ENABLE();
    VOLTAGE_ADC_CLK_ENABLE();
    // ʹ�� GPIO ʱ��
    VOLTAGE_ADC_GPIO_CLK_ENABLE();

    // ���� IO
    GPIO_InitStructure.Pin = VOLTAGE_ADC_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStructure.Pull = GPIO_NOPULL ; //������������
    HAL_GPIO_Init(VOLTAGE_ADC_GPIO_PORT, &GPIO_InitStructure);
}
#ifndef USE_ADC_IT
// DMA1 for ADC set config
static void Voltage_ADC_DMA_Config(void)
{
    //HAL_StatusTypeDef DMA_status = HAL_ERROR;
    
    VOLTAGE_ADC_DMA1_CLK_ENABLE();
    
    DMA_Handle.Instance         = VOLTAGE_ADC_DMA;
    DMA_Handle.Init.Direction   = DMA_PERIPH_TO_MEMORY;
    DMA_Handle.Init.PeriphInc   = DMA_PINC_DISABLE;
    DMA_Handle.Init.MemInc      = DMA_MINC_ENABLE;
    DMA_Handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    DMA_Handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    DMA_Handle.Init.Mode        = DMA_CIRCULAR;
    DMA_Handle.Init.Priority    = DMA_PRIORITY_VERY_HIGH;
    HAL_DMA_Init(&DMA_Handle);
    
    __HAL_LINKDMA(&ADC_Handle, DMA_Handle,DMA_Handle);
    
//    DMA_status = HAL_DMA_Start(&DMA_Handle, (uint32_t)ADC1_DR_ADDRESS, (uint32_t)&AdcConvertedValue[0], 50);
//    if (DMA_status != HAL_OK)
//    {
//        Led_Ctl( Red, ON );
//    }
    
}
#endif
static void Voltage_ADC_Mode_Config(void)
{
    RCC_PeriphCLKInitTypeDef ADC_CLKInit;
    // ����ADCʱ��
    ADC_CLKInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;			//ADC����ʱ��
    ADC_CLKInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;			  //��Ƶ����6ʱ��Ϊ72M/6=12MHz
    HAL_RCCEx_PeriphCLKConfig(&ADC_CLKInit);					      //����ADCʱ��

    ADC_Handle.Instance = VOLTAGE_ADC;
    ADC_Handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;             //�Ҷ���
    ADC_Handle.Init.ScanConvMode = DISABLE;                      //��ɨ��ģʽ
    ADC_Handle.Init.ContinuousConvMode = ENABLE;                 //����ת��
    ADC_Handle.Init.NbrOfConversion = 1;                         //1��ת���ڹ��������� Ҳ����ֻת����������1
    ADC_Handle.Init.DiscontinuousConvMode = DISABLE;             //��ֹ����������ģʽ
    ADC_Handle.Init.NbrOfDiscConversion = 0;                     //����������ͨ����Ϊ0
    ADC_Handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;       //�������
    HAL_ADC_Init(&ADC_Handle);                                 //��ʼ��

//---------------------------------------------------------------------------
    ADC_Config.Channel      = VOLTAGE_ADC_CHANNEL;
    ADC_Config.Rank         = 1;
    // ����ʱ����
    ADC_Config.SamplingTime = ADC_SAMPLETIME_55CYCLES_5 ;
    // ���� ADC ͨ��ת��˳��Ϊ1����һ��ת��������ʱ��Ϊ3��ʱ������
    HAL_ADC_ConfigChannel(&ADC_Handle, &ADC_Config);
#ifdef USE_ADC_IT
    HAL_ADC_Start_IT(&ADC_Handle);
#endif
}

// �����ж����ȼ�
static void Voltage_ADC_NVIC_Config(void)
{
#ifdef USE_ADC_IT
    HAL_NVIC_SetPriority(Voltage_ADC_IRQ, 0, 0);
    HAL_NVIC_EnableIRQ(Voltage_ADC_IRQ);
#else
    HAL_NVIC_SetPriority(Voltage_ADC_DMA_IRQ, 1, 0);
    HAL_NVIC_EnableIRQ(Voltage_ADC_DMA_IRQ);
#endif
}

void Voltage_Init(void)
{
    Voltage_ADC_GPIO_Config();
#ifndef USE_ADC_IT
    Voltage_ADC_DMA_Config();
#endif
    Voltage_ADC_Mode_Config();
    Voltage_ADC_NVIC_Config();
    HAL_ADCEx_Calibration_Start(&ADC_Handle); 
    HAL_ADC_Start_DMA(&ADC_Handle, (uint32_t *)&AdcConvertedValue[0], 50);
    //HAL_DMA_Start_IT(ADC_Handle.DMA_Handle, (uint32_t)ADC1_DR_ADDRESS, (uint32_t)&AdcConvertedValue[0], 50);
}

#ifdef USE_ADC_IT
/**
  * @brief  ת������жϻص�������������ģʽ��
  * @param  AdcHandle : ADC���
  * @retval ��
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
    /* ��ȡ��� */
    //ADC_ConvertedValue = HAL_ADC_GetValue(AdcHandle);
//    if (AdcValueCnt >= ADC_MAX_CNT)
//    {
//        AdcValueCnt = 0;
//    }
//    AdcConvertedValue[AdcValueCnt++] = HAL_ADC_GetValue(AdcHandle);
    //printf("AdcConvertedValue[%d] = %d \r\n",AdcValueCnt-1,AdcConvertedValue[AdcValueCnt-1]);
    HAL_ADC_Start_DMA(&ADC_Handle, (uint32_t *)&AdcConvertedValue[0], 50);
}
#endif
void AdcFalter(void)
{
    uint32_t sum;
    uint8_t  i;

    for (i = 0; i < ADC_MAX_CNT; i++)
    {
        sum += AdcConvertedValue[i];
        printf("AdcConvertedValue[%d] = %d \r\n",i,AdcConvertedValue[i]);
    }
    ADC_ConvertedValue = sum / ADC_MAX_CNT;
    printf("ADC_ConvertedValue = %d \r\n",ADC_ConvertedValue);
}

/***************************************
**�������ܣ���ȡ�ɼ���ѹֵ
**��	ʽ��V(sample) = V(REF) * Value/(0x0FFF + 1)
****************************************/
float Get_VoltageValue(void)
{
    float Value;

    AdcFalter();
//(float) ADC_ConvertedValue/4096*(float)3.3; // ��ȡת����ADֵ
    Value = (float)ADC_ConvertedValue * ((float)3.32 / 4096);
    printf("VoltageValue = %f \r\n",Value);

    return Value;
}


