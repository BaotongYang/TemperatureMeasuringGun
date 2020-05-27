#include "Run_task.h"
#include "Data_Structure.h"
#include "include.h"
#include "bmp.h"
#include "ST_string.h"

//�¶ȵĵ�λ ��
uint8_t TempCompany[][16]=
{
	0x06,0x09,0x09,0xE6,0xF8,0x0C,0x04,0x02,0x02,0x02,0x02,0x02,0x04,0x1E,0x00,0x00,
	0x00,0x00,0x00,0x07,0x1F,0x30,0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x10,0x00,0x00,/*"��",0*/

};

enum TEMP_LEVEL{
	TEMP_NORMAL = 0,
	TEMP_ONE_LEVEL_HIGH,
	TEMP_SECOND_LEVEL_HIGH,
	TEMP_THREE_LEVEL_HIGH,
	TEMP_FOUR_LEVEL_HIGH,
};

enum VOL_LEVEL{
	VOL_NORMAL = 0,
	VOL_LOW,
};

struct TempMeasuringGun_type{
	uint32_t TempCleanScreenFlag;  //�¶������ı�־
	uint32_t VolCleanScreenFlag;  //��ѹ�����ı�־
	uint8_t CollectionFlag;  //�ɼ����ݵı�־
	uint8_t TempWarn;		//�¶ȱ���
	uint8_t TempWarn_count;	//�¶ȱ���������
	uint8_t VolWarn;		//�¶ȱ���
	uint8_t VolWarn_count;	//�¶ȱ���������
	float Temperature;    //�¶����ݱ����������ͣ�
	float VBAT;
	char TempValue[30]; //�¶�ֵ���ַ�����
	char VoltageValueStr[30]; //��ѹֵ���ַ�����
};

static void Run_Task(void)
{
	uint8_t msg = NO_MSG;
	float VoltageValue = 0.0;
	struct TempMeasuringGun_type board;
	
	OWL_BOARD_DEBUG("Run_Task\r\n");
	
	st_memset((unsigned char *)&board,0, sizeof(struct TempMeasuringGun_type));
	
	put_msg_Fifo(MSG_KEY_TYPE_LEFT);
	while(1)
	{
		msg = get_msg_Fifo();
		
		switch(msg)
		{
			case MSG_KEY_TYPE_UP:
				//�ϰ���:  ʵ�ֲ��¹��ܣ��ɼ�MLX90614�����ݣ�������ʾ����ʾ
				OWL_BOARD_DEBUG("MSG_KEY_TYPE_UP\r\n");
			
				if(board.CollectionFlag != 0)
				{
					if(board.TempCleanScreenFlag == 0)//����ģʽʱ��ֻ����һ��
					{
						OLED_DataClear();
						board.TempCleanScreenFlag++;
					}
					
					OLED_ShowCHinese16x16(5*16,2,0,TempCompany);
					
					board.Temperature = infrared_ReadTemp();  //��ȡ�¶�
					sprintf(board.TempValue,"%.1f", board.Temperature);     //������ת�����ַ���
					OLED_ShowString(40,2,(uint8_t *)board.TempValue,16);   //��ʾ�¶�
					
					//���ȷ�Ϊ�� 
					//���� ��37.2��38�棻 �еȶ��ȣ�38��1��39�棺
					//���ȣ�39��1��41�棻 ������ ��41������
					if(board.Temperature >= 37.2 && board.Temperature <= 38.0)
					{	//����Ԥ��
						board.TempWarn = TEMP_ONE_LEVEL_HIGH;
						board.TempWarn_count = 0;
					}
					else if(board.Temperature >= 38.1 && board.Temperature <= 39.0)
					{	//����Ԥ��
						board.TempWarn = TEMP_SECOND_LEVEL_HIGH;
						board.TempWarn_count = 0;
					}
					else if(board.Temperature >= 39.1 && board.Temperature <= 41.0)
					{	//����Ԥ��
						board.TempWarn = TEMP_THREE_LEVEL_HIGH;
						board.TempWarn_count = 0;
					}
					else if(board.Temperature > 41)
					{	//������Ԥ��
						board.TempWarn = TEMP_FOUR_LEVEL_HIGH;
						board.TempWarn_count = 0;
					}
					
					board.VolCleanScreenFlag = 0; //�����ѹ�����ı�־
				}
				break;
			case MSG_KEY_TYPE_DOWN:
				//�°���:  ʵ�ֲɼ���ѹ���ܣ�������ʾ����ʾ
				OWL_BOARD_DEBUG("MSG_KEY_TYPE_DOWN\r\n");
				if(board.CollectionFlag != 0)
				{
					if(board.VolCleanScreenFlag == 0)
					{
						OLED_DataClear() ;      //��������е���Ļ��Ϣ
						OLED_ShowChar(80,2,'V',16);
						board.VolCleanScreenFlag++;
					}
					
					VoltageValue = Get_VoltageValue();//������ѹ
					board.VBAT = VoltageValue*(10 + 10)/10;//﮵�ص�ѹ
					
					sprintf(board.VoltageValueStr,"%.2f", board.VBAT);      //������ת�����ַ���
					OLED_ShowString(40,2,(uint8_t *)board.VoltageValueStr,16);   //��ʾ�¶�
					
					if(board.VBAT < 3.0)
					{
						board.VolWarn = VOL_LOW;
						board.VolWarn_count = 0;
					}
				}
				break;
			case MSG_KEY_TYPE_LEFT:	//��������к��ˣ�����ʾ�׽���
				OWL_BOARD_DEBUG("MSG_KEY_TYPE_LEFT\r\n");
				OLED_Clear();  //����
				OLED_DrawBMP(0,0,128,8,Peacock);  //��ʾ�׽���
				
				board.CollectionFlag = 0;
				board.TempCleanScreenFlag = 0;
				board.VolCleanScreenFlag = 0;
				break;
			case MSG_KEY_TYPE_RIGHT:	//����ѡ�����
				OWL_BOARD_DEBUG("MSG_KEY_TYPE_RIGHT\r\n");
				OLED_Clear();  //����
				OLED_DrawBMP(0,0,128,8,BMP1);  //��ʾ�׽���
				OLED_ShowString(0,2,"(T):up (V):dowm ",16);
			
				board.CollectionFlag++;
				board.TempCleanScreenFlag = 0;
				board.VolCleanScreenFlag = 0;
				break;
			case MSG_60MS:
				if(board.TempWarn == TEMP_FOUR_LEVEL_HIGH)
				{
					if((board.TempWarn_count == 0)||(board.TempWarn_count%2 == 0))
					{
						Beep_VoiceRegulation(10);
						 Led_Ctl( Red, ON );
					}
					else
					{
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
					board.TempWarn_count++;
					if(board.TempWarn_count > 10)
					{
						board.TempWarn_count = 0;
						board.TempWarn = TEMP_NORMAL;
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
				}
				break;
			case MSG_100MS:
				if(board.TempWarn == TEMP_THREE_LEVEL_HIGH)
				{
					if((board.TempWarn_count == 0)||(board.TempWarn_count%2 == 0))
					{
						Beep_VoiceRegulation(10);
						 Led_Ctl( Red, ON );
					}
					else
					{
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
					board.TempWarn_count++;
					if(board.TempWarn_count > 10)
					{
						board.TempWarn_count = 0;
						board.TempWarn = TEMP_NORMAL;
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
				}
				break;
			case MSG_200MS:
//				OWL_BOARD_DEBUG("MSG_200MS\r\n");
				if(board.TempWarn == TEMP_SECOND_LEVEL_HIGH)
				{
					if((board.TempWarn_count == 0)||(board.TempWarn_count%2 == 0))
					{
						Beep_VoiceRegulation(10);
						 Led_Ctl( Red, ON );
					}
					else
					{
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
					board.TempWarn_count++;
					if(board.TempWarn_count > 10)
					{
						board.TempWarn_count = 0;
						board.TempWarn = TEMP_NORMAL;
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
				}
				break;
			case MSG_HALF_SECOND:
//				OWL_BOARD_DEBUG("MSG_HALF_SECOND\r\n");
				if(board.TempWarn == TEMP_ONE_LEVEL_HIGH)
				{
					if((board.TempWarn_count == 0)||(board.TempWarn_count%2 == 0))
					{
						Beep_VoiceRegulation(10);
						 Led_Ctl( Red, ON );
					}
					else
					{
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
					board.TempWarn_count++;
					if(board.TempWarn_count > 10)
					{
						board.TempWarn_count = 0;
						board.TempWarn = TEMP_NORMAL;
						Beep_VoiceRegulation(0);
						Led_Ctl( Red, OFF );
					}
				}
				
				if(board.VolWarn == VOL_LOW)
				{
					if((board.VolWarn_count == 0)||(board.VolWarn_count%2 == 0))
					{
						Beep_VoiceRegulation(10);
						 Led_Ctl( Green, ON );
					}
					else
					{
						Beep_VoiceRegulation(0);
						Led_Ctl( Green, OFF );
					}
					board.VolWarn_count++;
					if(board.VolWarn_count > 10)
					{
						board.VolWarn_count = 0;
						board.VolWarn = VOL_NORMAL;
						Beep_VoiceRegulation(0);
						Led_Ctl( Green, OFF );
					}
				}
				break;
			case MSG_800MS:
				break;
			default:
				break;
		}
	}
}

void Run_Start(void)
{
	printf("Run_Start\r\n");
	
	SeqQueue_Init();
	Clear_msg_Fifo();
	
	Run_Task();
}

