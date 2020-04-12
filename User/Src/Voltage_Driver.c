#include "Voltage_Driver.h"


__IO uint32_t ADC_ConvertedValue;
DMA_HandleTypeDef DMA_Init_Handle;
ADC_HandleTypeDef ADC_Handle;
ADC_ChannelConfTypeDef ADC_Config;
uint16_t g_waRawADCVal[50];
uint16_t g_waAvgADCRawVal;

static void DMA_ADC_Config(void);
static void DMA_ADC_TransferCplt(DMA_HandleTypeDef * _hdma);

static void DMA_ADC_Config(void)
{
	HAL_StatusTypeDef  tErrCode;
  __HAL_RCC_DMA1_CLK_ENABLE();
	
	__HAL_LINKDMA(&ADC_Handle, DMA_Handle, DMA_Init_Handle);
  DMA_Init_Handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
  DMA_Init_Handle.Init.PeriphInc = DMA_PINC_DISABLE;
  DMA_Init_Handle.Init.MemInc = DMA_MINC_ENABLE;
  DMA_Init_Handle.Init.PeriphDataAlignment =DMA_PDATAALIGN_HALFWORD;
  DMA_Init_Handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  DMA_Init_Handle.Init.Mode = DMA_CIRCULAR;
  DMA_Init_Handle.Init.Priority = DMA_PRIORITY_MEDIUM;

  DMA_Init_Handle.Instance = DMA1_Channel1;

	HAL_DMA_DeInit(&DMA_Init_Handle);
	
	if (HAL_ERROR == HAL_DMA_Init(&DMA_Init_Handle))
	{
			while(1);
	}
	
  __HAL_DMA_ENABLE_IT(&DMA_Init_Handle, DMA_IT_TC);
	
	
	//�����ж����ȼ�
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);	

	//HAL_DMA_RegisterCallback(&DMA_Init_Handle, HAL_DMA_XFER_CPLT_CB_ID, DMA_ADC_TransferCplt);
	
	
	tErrCode = HAL_DMA_Start_IT(&DMA_Init_Handle, (uint32_t)&ADC_Handle.Instance->DR, (uint32_t)g_waRawADCVal, sizeof(g_waRawADCVal)/sizeof(g_waRawADCVal[0]));	
	// 
}


static void DMA_ADC_TransferCplt(DMA_HandleTypeDef * _hdma)
{
  uint8_t cnt;
  uint64_t ulADCRawSum = 0;
  uint8_t ucBufferLen = sizeof(g_waRawADCVal) / sizeof(g_waRawADCVal[0]);
  uint16_t wValMax = 0;
	uint16_t wValMin = 0xFFFF;	
	
  for (cnt = 0; cnt < ucBufferLen; cnt++)
  {
		ulADCRawSum += g_waRawADCVal[cnt];
		if (wValMax <= g_waRawADCVal[cnt])
			wValMax = g_waRawADCVal[cnt];
    if (wValMin >= g_waRawADCVal[cnt])
			wValMin = g_waRawADCVal[cnt];
  }
	ulADCRawSum = ulADCRawSum - wValMax - wValMin;
	
  g_waAvgADCRawVal = (uint16_t)(ulADCRawSum / (ucBufferLen - 2));
}

static void Voltage_ADC_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    VOLTAGE_ADC_CLK_ENABLE(); 
    // ʹ�� GPIO ʱ��
    VOLTAGE_ADC_GPIO_CLK_ENABLE();
          
    // ���� IO
    GPIO_InitStructure.Pin = VOLTAGE_ADC_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;	    
    GPIO_InitStructure.Pull = GPIO_NOPULL ; //������������
    HAL_GPIO_Init(VOLTAGE_ADC_GPIO_PORT, &GPIO_InitStructure);		
}

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

    //
		DMA_ADC_Config();
		
		//HAL_ADC_Start(&ADC_Handle);
    //HAL_ADC_Start_DMA(&ADC_Handle, (unsigned int*)g_waRawADCVal, 50);
    //HAL_ADC_Start_IT(&ADC_Handle);
}

// �����ж����ȼ�
static void Voltage_ADC_NVIC_Config(void)
{
  HAL_NVIC_SetPriority(Voltage_ADC_IRQ, 0, 0);
  HAL_NVIC_EnableIRQ(Voltage_ADC_IRQ);
}

void Voltage_Init(void)
{
	Voltage_ADC_GPIO_Config();
	Voltage_ADC_Mode_Config();
  //Voltage_ADC_NVIC_Config();

	HAL_ADC_Start_DMA(&ADC_Handle,(unsigned int*)g_waRawADCVal, sizeof(g_waRawADCVal)/sizeof(g_waRawADCVal[0]));
}

/**
  * @brief  ת������жϻص�������������ģʽ��
  * @param  AdcHandle : ADC���
  * @retval ��
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
  /* ��ȡ��� */
  //ADC_ConvertedValue = HAL_ADC_GetValue(AdcHandle);
  uint8_t cnt;
  uint64_t ulADCRawSum = 0;
  uint8_t ucBufferLen = sizeof(g_waRawADCVal) / sizeof(g_waRawADCVal[0]);
  
  for (cnt = 0; cnt < ucBufferLen; cnt++)
  {
    ulADCRawSum += g_waRawADCVal[cnt];
  }

  g_waAvgADCRawVal = (uint16_t)(ulADCRawSum / ucBufferLen);
	
	
}



/***************************************
**�������ܣ���ȡ�ɼ���ѹֵ
**��	ʽ��V(sample) = V(REF) * Value/(0x0FFF + 1)
****************************************/
float Get_VoltageValue(void)
{
	float Value;
 //(float) ADC_ConvertedValue/4096*(float)3.3; // ��ȡת����ADֵ
	Value = (float)g_waAvgADCRawVal * ((float)3.3 / 4096);

	return Value;
}

