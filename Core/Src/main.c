/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#include "ogg/ivorbiscodec.h"
#include "ogg/ivorbisfile.h"
void Serialprint(const char *c)
{
	 HAL_UART_Transmit(&huart1, c, strlen(c),100);
}
void Serialprintlong(long n)
{
	 char c[10];
	 sprintf(c, "%lu", n);
	 Serialprint(c);


}

void Serialprinthex(long n)
{
	 char c[10];
	 sprintf(c, "%4x", n);
	 Serialprint(c);


}

/* USER CODE BEGIN PV */

//Tx Transfer Half completed callbacks.


 char dma_run=0;

 /* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  /* USER CODE BEGIN 2 */




#include "myaudio.h"


Serialprint("Start.\n\r");


  OggVorbis_File vf;
  int current_section;



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint16_t *buf;
  uint16_t pcmout_audio[2048*4];

  buf=malloc(sizeof(pcmout_audio));
  char ifdma=0;
while (1)
  {
      FILE *f;
      f=fmemopen(file_ogg, sizeof(file_ogg), "r"); //open file from mem
//      f=fopen(file_ogg, "\play.ogg", "r"); //open file from file system
	  if(ov_open(f, &vf, NULL, 0) < 0) {
	      Serialprint("Input does not appear to be an Ogg bitstream.\n\r\r");
	      exit(1);
	  }
	    Serialprint("opn okm.\n\r\r");
	    char **ptr=ov_comment(&vf,-1)->user_comments;
	    vorbis_info *vi=ov_info(&vf,-1);

	    Serialprintlong(vi->channels);Serialprint(":channels\r\n");
	    Serialprintlong(vi->rate);	    Serialprint(":rate\r\n");
	    Serialprintlong( (long)ov_pcm_total(&vf,-1));
	    Serialprint(":pcm_total\r\n");
	    Serialprint((long)ov_comment(&vf,-1)->vendor);
	    Serialprint("\r\nOgg file OK\r\n");

	    {

	       Serialprint(":strart play\r\n");
	       uint16_t pcmout[1024];

	 	   long len_oggread=0;
	 	   while(1)
	 	     {
	 		      long len=0;
				  while ((sizeof(pcmout_audio)>>1)-len>=len_oggread)
				   {

					  if (len_oggread>0)
					  	{

						  if (vi->channels==2) //if stereo
						  {
							memcpy(pcmout_audio+len,pcmout,len_oggread*2);
							len+=len_oggread;
						  }else
							  {

							   long level=0;
							   for (long i=0;i<len_oggread;i++)
								{
								  if(level<pcmout[i])level=pcmout[i];

								}


							   for (long i=0;i<len_oggread;i++)
								{
								  pcmout_audio[i*2+len]=(0xffff/level*pcmout[i]);
								  pcmout_audio[i*2+1+len]=(0xffff/level*pcmout[i]);
								  /*
								  Serialprintlong(pcmout[i]);
								  Serialprint("::");
								  Serialprintlong(pcmout_audio[i*2+len]);
								  Serialprint("\r\n");*/
								}
							   len+=len_oggread*2;
							  }
					  	}
					  len_oggread=ov_read(&vf,pcmout,sizeof(pcmout)<<1,&current_section);
					  if (len_oggread>0)
					   {
						len_oggread=len_oggread>>1;;//if not stereo
					    if ((sizeof(pcmout_audio)>>1)-len<len_oggread)break;
					   }else  if(len_oggread==0)break;
				 }
               if(len>0)
                 {
            	  if (ifdma==1)
            	   {
    	 	        while (dma_run!=0)HAL_Delay(1);
    	 	        dma_run=1;
    	 	        memcpy(buf,pcmout_audio,len*2);
 	    	        HAL_I2S_Transmit_DMA(&hi2s2,buf,len);//sizeof(pcmout_audio)/2);
                 }else
            	  {
	 	              while (HAL_I2S_GetState(&hi2s2)!=HAL_I2S_STATE_READY)HAL_Delay(1);
	                  HAL_I2S_Transmit(&hi2s2,pcmout_audio,len,10);

                 }
            	  len=0;
                 }else break;
	 	     }
	 	ov_clear(&vf);
	 	fclose(f);

	    Serialprint(":end play\r\n");
	    HAL_Delay(1000);


	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_8K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */
void HAL_I2S_TxHalfCpltCallback (I2S_HandleTypeDef *hi2s)
{
	if(hi2s=&hi2s2)
		{
			dma_run+=2;
		}
   	}
//Tx Transfer completed callbacks
void HAL_I2S_TxCpltCallback (I2S_HandleTypeDef *hi2s)
{
	if(hi2s=&hi2s2)
		{
		   dma_run=0;
		}

   	}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n\r", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

