/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * Özet:
  * Bu program ADS1148 datasheetinin 65. sayfasındaki pseudocode takip edilerek
  * yazılmıştır. InitConfig fonksiyonu ile ADS1148 initialize edilditen sonra
  * DRDY falling edge sinyalleri external interrupt ile algılanıp çevrim
  * sonuçları okunur.
  *
  * Düzeltilmesi gereken hatalar:
  * 1)SPI ile okuma yapıldığı zaman sonuçlar 1 bit sağa shiftlenmiş halde
  * okunuyor. Bu problemi geçici olarak çözmek için tüm read fonksiyonlarının
  * sonunda 1 bit sola shiftleme operasyonu yaptım. Shift sorununun çözülmesi
  * durumunda shiftl() fonksiyonunun silinmesi yeterli olacaktır.
  *
  * 2)DRDY falling edge sinyalleri gelmesine ve external interrupta girilmesine
  * rağmen ADC çevrim sonuçlarını okumada başarısız oldum.
  *
  * 3)2.5V olması gereken referans gerilimini ölçtüğümde 0.8V olduğunu gördüm.
  * U10 çipi sorunlu olabilir.(kart 2 de bu hata düzeltildi)
  *
  * 4)R28 direncinin üzerine düşen gerilimde sıkıntı var.
  * INA826AIDGKR datasheetinde Figure 64'teki grafiğe uymuyor.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ads1148.h"
//#include "stm32fxx_hal_uart.h"
char buffer[32]; //uart ile yazdırmada kullanılıyor
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

/* USER CODE BEGIN PV */
uint8_t Register_Checkup[1];
uint8_t err = 0;
uint8_t iset1;
uint8_t iset2;
uint8_t rxData1[2];
uint8_t rxData2[2];
uint8_t no_exti = 1;
uint8_t DRDY_1_Falling_Edge;
uint8_t DRDY_2_Falling_Edge;
//volatile double DigitalPSInput = 0;  eğer jlink kullanıp live expressionsdan değer okunacaksa bu değer kullanılmalı

void uprintf(char *str)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 100);
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	volatile signed short int AnalogValue;
	volatile signed short int BinaryMagnitude;
	volatile double StepSize = 0.0000761413574; // el ile hesaplandı
	volatile double DigitalDifInput = 0;
	volatile double DigitalPSInput = 0;
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
  MX_CAN_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  InitConfig();
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  //__HAL_UART_ENABLE_IT(&huart1, HUART_IT_RXNE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(DRDY_1_Falling_Edge)
	   	  {
	  		  ADS1148ReadDout(rxData1, 1);
	  		  DRDY_1_Falling_Edge = 0;
	   		  AnalogValue = (signed int) ((rxData1[0] << 8) + rxData1[1]);
	   		  BinaryMagnitude = AnalogValue + 32768;
	   		  DigitalDifInput = (BinaryMagnitude * StepSize) - 2.5;
	  		  DigitalPSInput = -5 * DigitalDifInput + 1 + 0.21; // 0.21 = error compensation
	   		  sprintf(buffer, "Value: %f ", (float)DigitalPSInput);
	  		  uprintf(buffer);
	   		  HAL_Delay(200);
	  	  }

	   if(DRDY_2_Falling_Edge)
	   	  {
	   		  ADS1148ReadDout(rxData2, 2);
	  		  DRDY_2_Falling_Edge = 0;
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

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//_____________________________________________________________________________________________________
void ADS1148AssertCS1( int fAssert)
{
	if (fAssert)													//fAssert=0 ise CS low, fAssert=1
	{																//ise CS high
		HAL_GPIO_WritePin(GPIOB, SS_IN_1_Pin, GPIO_PIN_SET);
	} else
		HAL_GPIO_WritePin(GPIOB, SS_IN_1_Pin, GPIO_PIN_RESET);
}
//_____________________________________________________________________________________________________
void ADS1148AssertCS2( int fAssert)
{
	if (fAssert)													//fAssert=0 ise CS low, fAssert=1
	{																//ise CS high
		HAL_GPIO_WritePin(GPIOB, SS_IN_2_Pin, GPIO_PIN_SET);
	} else
		HAL_GPIO_WritePin(GPIOB, SS_IN_2_Pin, GPIO_PIN_RESET);
}
//_____________________________________________________________________________________________________
void ADS1148SlaveSelect(int slave)
{
	if(slave == 1)
	{
		ADS1148AssertCS1(0);
		ADS1148AssertCS2(1);
	}
	else if(slave == 2)
	{
		ADS1148AssertCS1(1);
		ADS1148AssertCS2(0);
	}
}
//_____________________________________________________________________________________________________
void ADS1148SlaveOff(void)
{
	ADS1148AssertCS1(1);
	ADS1148AssertCS2(1);
}
//_____________________________________________________________________________________________________
void shiftl(uint8_t *object, size_t size)
{
   uint8_t *byte;
   for ( byte = object; size--; ++byte )
   {
	   uint8_t bit = 0;
      if ( size )
      {
         bit = byte[1] & (1 << 7) ? 1 : 0;
      }
      *byte <<= 1;
      *byte  |= bit;
   }
}
//_____________________________________________________________________________________________________
void ADS1148SendByte(uint8_t Byte[])
{
	if(HAL_SPI_Transmit(&hspi1, Byte, 1, 10) != HAL_OK)
	{
		 /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
}
//_____________________________________________________________________________________________________
void ADS1148ReceiveByte(void)
{
	uint8_t Result[5];
	if(HAL_SPI_Receive(&hspi1, Result, 1, 1) != HAL_OK)
	{
		 /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
}
//_____________________________________________________________________________________________________
void ADS1148SendWakeup(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_WAKEUP);
	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
void ADS1148SendSleep(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_SLEEP);
	ADS1148SlaveOff();
	/*Cihazın sleep te kalması için CS low tutulmalıdır*/
	return;
}
//_____________________________________________________________________________________________________
void ADS1148SendSync(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_SYNC);
	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
void ADS1148SendResetCommand(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_RESET);
	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
void ADS1148ReadRegister(uint8_t StartAddress, int NumRegs, uint8_t STORE[], uint8_t slave)
{
	uint8_t REC_RR[4];
	uint8_t CMD_RR1 = (ADS1148_CMD_RREG[0] | (StartAddress & 0x0f));
	uint8_t CMD_RR2 = (NumRegs-1) & 0x0f;
	uint8_t CMD_RR[4] = {CMD_RR1, CMD_RR2, 0Xff, 0xff};

	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	if(HAL_SPI_TransmitReceive(&hspi1, CMD_RR, REC_RR, 4, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);

	shiftl(REC_RR, 4);
	STORE[0] = REC_RR[2];

	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
void ADS1148WriteCommand(uint8_t StartAddress, int NumRegs, uint8_t slave)
{
	uint8_t CMD_WS1 = (ADS1148_CMD_WREG[0] | (StartAddress & 0x0f));
	uint8_t CMD_WS2 = (NumRegs-1) & 0x0f;
	uint8_t CMD_WS[2] = {CMD_WS1, CMD_WS2};

	ADS1148SlaveSelect(slave);

	//komut byte'ını gönder
	if(HAL_SPI_Transmit(&hspi1, CMD_WS, 2, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
}
//_____________________________________________________________________________________________________
void ADS1148ReadDout(uint8_t Data_RD[], uint8_t slave)
{
	uint8_t STORE[3];
	uint8_t CMD_RD1 = ADS1148_CMD_RDATA[0];
	uint8_t CMD_RD[3] = {CMD_RD1, 0xff, 0xff};
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	if(HAL_SPI_TransmitReceive(&hspi1, CMD_RD, STORE, 3, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	//CS leri tekrar high yap
	ADS1148SlaveOff();

	shiftl(STORE, 3);
	Data_RD[0] = STORE[1];
	Data_RD[1] = STORE[2];
}
//_____________________________________________________________________________________________________
void ADS1148SendSDATAC(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_SDATAC);
	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
void ADS1148SendSYSOCAL(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_SYSOCAL);
	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
void ADS1148SendSYSGCAL(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_SYSGCAL);
	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
void ADS1148SendSELFOCAL(uint8_t slave)
{
	//slave seçimi
	ADS1148SlaveSelect(slave);
	//komut byte'ını gönder
	ADS1148SendByte(ADS1148_CMD_SELFOCAL);
	//tüm slave'leri kapat
	ADS1148SlaveOff();
	return;
}
//_____________________________________________________________________________________________________
//_____________________________________________________________________________________________________
/*REGISTER KONFİGÜRASYON FONKSİYONLARI: Bu fonksiyonlar konfigürasyon registerlarını ayarlamak için kul
-lanılır*/
int ADS1148SetBurnOutSource(int BurnOut, uint8_t slave)
{
	uint8_t Temp1[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_0_MUX0, 0x01, Temp1, slave);
	Temp1[0] &= 0x3f;
	switch(BurnOut) {
		case 0:
			Temp1[0] |= ADS1148_BCS_OFF;
			break;
		case 1:
			Temp1[0] |= ADS1148_BCS_500nA;
			break;
		case 2:
			Temp1[0] |= ADS1148_BCS_2uA;
			break;
		case 3:
			Temp1[0] |= ADS1148_BCS_10uA;
			break;
		default:
			dError = ADS1148_ERROR;
			Temp1[0] |= ADS1148_BCS_OFF;
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_0_MUX0, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp1, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}

int ADS1148SetChannel(uint8_t chSelBit, int pMux, uint8_t slave)
{
	uint8_t Temp2[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_0_MUX0, 0x01, Temp2, slave);
	if (pMux==1) {
		Temp2[0] &= 0xf8;
		Temp2[0] |= chSelBit;

	} else {
		Temp2[0] &= 0xc7;
		Temp2[0] |= chSelBit;
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_0_MUX0, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp2, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();

	return dError;
}


//_____________________________________________________________________________________________________
/*
int ADS1148SetChannel(int vMux, int pMux, uint8_t slave)
{
	uint8_t Temp2[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_0_MUX0, 0x01, Temp2, slave);
	if (pMux==1) {
		Temp2[0] &= 0xf8;
		switch(vMux) {
			case 0:
				Temp2[0] |= ADS1148_AINN0;
				break;
			case 1:
				Temp2[0] |= ADS1148_AINN1;
				break;
			case 2:
				Temp2[0] |= ADS1148_AINN2;
				break;
			case 3:
				Temp2[0] |= ADS1148_AINN3;
				break;
			case 4:
				Temp2[0] |= ADS1148_AINN4;
				break;
			case 5:
				Temp2[0] |= ADS1148_AINN5;
				break;
			case 6:
				Temp2[0] |= ADS1148_AINN6;
				break;
			case 7:
				Temp2[0] |= ADS1148_AINN7;
				break;
			default:
				Temp2[0] |= ADS1148_AINN0;
				dError = ADS1148_ERROR;
		}

	} else {
		Temp2[0] &= 0xc7;
		switch(vMux) {
			case 0:
				Temp2[0] |= ADS1148_AINP0;
				break;
			case 1:
				Temp2[0] |= ADS1148_AINP1;
				break;
			case 2:
				Temp2[0] |= ADS1148_AINP2;
				break;
			case 3:
				Temp2[0] |= ADS1148_AINP3;
				break;
			case 4:
				Temp2[0] |= ADS1148_AINP4;
				break;
			case 5:
				Temp2[0] |= ADS1148_AINP5;
				break;
			case 6:
				Temp2[0] |= ADS1148_AINP6;
				break;
			case 7:
				Temp2[0] |= ADS1148_AINP7;
				break;
			default:
				Temp2[0] |= ADS1148_AINP0;
				dError = ADS1148_ERROR;
		}
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_0_MUX0, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp2, 1, 20) != HAL_OK)
	{
	  // Transmission Hatası Durumunda err 1 olur
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();

	return dError;
}
*/
//_____________________________________________________________________________________________________
int ADS1148SetBias(unsigned char vBias, uint8_t slave)
{
	uint8_t Temp3[1];
	Temp3[0] = ADS1148_VBIAS_OFF;
	if (vBias & 0x80)
		Temp3[0] |=  ADS1148_VBIAS7;
	if (vBias & 0x40)
		Temp3[0] |=  ADS1148_VBIAS6;
	if (vBias & 0x20)
		Temp3[0] |=  ADS1148_VBIAS5;
	if (vBias & 0x10)
		Temp3[0] |=  ADS1148_VBIAS4;
	if (vBias & 0x08)
		Temp3[0] |=  ADS1148_VBIAS3;
	if (vBias & 0x04)
		Temp3[0] |=  ADS1148_VBIAS2;
	if (vBias & 0x02)
		Temp3[0] |=  ADS1148_VBIAS1;
	if (vBias & 0x01)
		Temp3[0] |=  ADS1148_VBIAS0;
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_1_VBIAS, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp3, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return ADS1148_NO_ERROR;
}
//_____________________________________________________________________________________________________
// YAZ--> Mux1
int ADS1148SetIntRef(int sRef, uint8_t slave)
{
	uint8_t Temp4[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_2_MUX1, 0x01, Temp4, slave);
	Temp4[0] &= 0x1f;
	switch(sRef) {
		case 0:
			Temp4[0] |= ADS1148_INT_VREF_OFF;
			break;
		case 1:
			Temp4[0] |= ADS1148_INT_VREF_ON;
			break;
		case 2:
		case 3:
			Temp4[0] |= ADS1148_INT_VREF_CONV;
			break;
		default:
			Temp4[0] |= ADS1148_INT_VREF_OFF;
			dError = ADS1148_ERROR;

	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_2_MUX1, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp4, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}
//_____________________________________________________________________________________________________
int ADS1148SetVoltageReference(int VoltageRef, uint8_t slave)
{
	uint8_t Temp5[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_2_MUX1, 0x01, Temp5, slave);
	Temp5[0] &= 0xe7;
	switch(VoltageRef) {
		case 0:
			Temp5[0] |= ADS1148_REF0;
			break;
		case 1:
			Temp5[0] |= ADS1148_REF1;
			break;
		case 2:
			Temp5[0] |= ADS1148_INT;
			break;
		case 3:
			Temp5[0] |= ADS1148_INT_REF0;
			break;
		default:
			Temp5[0] |= ADS1148_REF0;
			dError = ADS1148_ERROR;
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_2_MUX1, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp5, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}
//_____________________________________________________________________________________________________
int ADS1148SetSystemMonitor(int Monitor, uint8_t slave)
{
	uint8_t Temp6[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_2_MUX1, 0x01, Temp6, slave);
	Temp6[0] &= 0x78;
	switch(Monitor) {
		case 0:
			Temp6[0] |= ADS1148_MEAS_NORM;
			break;
		case 1:
			Temp6[0] |= ADS1148_MEAS_OFFSET;
			break;
		case 2:
			Temp6[0] |= ADS1148_MEAS_GAIN;
			break;
		case 3:
			Temp6[0] |= ADS1148_MEAS_TEMP;
			break;
		case 4:
			Temp6[0] |= ADS1148_MEAS_REF1;
			break;
		case 5:
			Temp6[0] |= ADS1148_MEAS_REF0;
			break;
		case 6:
			Temp6[0] |= ADS1148_MEAS_AVDD;
			break;
		case 7:
			Temp6[0] |= ADS1148_MEAS_DVDD;
			break;
		default:
			Temp6[0] |= ADS1148_MEAS_NORM;
			dError = ADS1148_ERROR;
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_2_MUX1, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp6, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}
//_____________________________________________________________________________________________________
// YAZ--> SYS0
int ADS1148SetGain(int Gain, uint8_t slave)
{
	uint8_t Temp7[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_3_SYS0, 0x01, Temp7, slave);
	Temp7[0] &= 0x0f;
	switch(Gain) {
		case 0:
			Temp7[0] |= ADS1148_GAIN_1;
			break;
		case 1:
			Temp7[0] |= ADS1148_GAIN_2;
			break;
		case 2:
			Temp7[0] |= ADS1148_GAIN_4;
			break;
		case 3:
			Temp7[0] |= ADS1148_GAIN_8;
			break;
		case 4:
			Temp7[0] |= ADS1148_GAIN_16;
			break;
		case 5:
			Temp7[0] |= ADS1148_GAIN_32;
			break;
		case 6:
			Temp7[0] |= ADS1148_GAIN_64;
			break;
		case 7:
			Temp7[0] |= ADS1148_GAIN_128;
			break;
		default:
			Temp7[0] |= ADS1148_GAIN_1;
			dError = ADS1148_ERROR;
		}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_3_SYS0, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp7, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
		return dError;
}
//_____________________________________________________________________________________________________
int ADS1148SetDataRate(int DataRate, uint8_t slave)
{
	uint8_t Temp8[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_3_SYS0, 0x01, Temp8, slave);
	Temp8[0] &= 0x70;
	switch(DataRate) {
		case 0:
			Temp8[0] |= ADS1148_DR_5;
			break;
		case 1:
			Temp8[0] |= ADS1148_DR_10;
			break;
		case 2:
			Temp8[0] |= ADS1148_DR_20;
			break;
		case 3:
			Temp8[0] |= ADS1148_DR_40;
			break;
		case 4:
			Temp8[0] |= ADS1148_DR_80;
			break;
		case 5:
			Temp8[0] |= ADS1148_DR_160;
			break;
		case 6:
			Temp8[0] |= ADS1148_DR_320;
			break;
		case 7:
			Temp8[0] |= ADS1148_DR_640;
			break;
		case 8:
			Temp8[0] |= ADS1148_DR_1000;
			break;
		case 9:
			Temp8[0] |= ADS1148_DR_2000;
			break;
		default:
			Temp8[0] |= ADS1148_DR_5;
			dError = ADS1148_ERROR;
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_3_SYS0, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp8, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}
//_____________________________________________________________________________________________________
// YAZ--> IDAC0
int ADS1148SetDRDYMode(int DRDYMode, uint8_t slave)
{
	uint8_t Temp9[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_10_IDAC0, 0x01, Temp9, slave);
	Temp9[0] &= 0xf7;
	switch(DRDYMode) {
		case 0:
			Temp9[0] |= ADS1148_DRDY_OFF;
			break;
		case 1:
			Temp9[0] |= ADS1148_DRDY_ON;
			break;
		default:
			Temp9[0] |= ADS1148_DRDY_OFF;
			dError = ADS1148_ERROR;
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_10_IDAC0, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp9, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}
//_____________________________________________________________________________________________________
int ADS1148SetCurrentDACOutput(int CurrentOutput, uint8_t slave)
{
	uint8_t Temp10[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_10_IDAC0, 0x01, Temp10, slave);
	Temp10[0] &= 0xf8;
	switch(CurrentOutput) {
		case 0:
			Temp10[0] |= ADS1148_IDAC_OFF;
			break;
		case 1:
			Temp10[0] |= ADS1148_IDAC_50;
			break;
		case 2:
			Temp10[0] |= ADS1148_IDAC_100;
			break;
		case 3:
			Temp10[0] |= ADS1148_IDAC_250;
			break;
		case 4:
			Temp10[0] |= ADS1148_IDAC_500;
			break;
		case 5:
			Temp10[0] |= ADS1148_IDAC_750;
			break;
		case 6:
			Temp10[0] |= ADS1148_IDAC_1000;
			break;
		case 7:
			Temp10[0] |= ADS1148_IDAC_1500;
			break;
		default:
			Temp10[0] |= ADS1148_IDAC_OFF;
			dError = ADS1148_ERROR;
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_10_IDAC0, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp10, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}
//_____________________________________________________________________________________________________
// YAZ--> IDAC1
int ADS1148SetIDACRouting(int IDACroute, int IDACdir, uint8_t slave)// IDACdir (0 = I1DIR, 1 = I2DIR)
{
	uint8_t Temp11[1];
	int dError = ADS1148_NO_ERROR;
	ADS1148ReadRegister(ADS1148_11_IDAC1, 0x01, Temp11, slave);
	if (IDACdir>0){
		Temp11[0] &= 0xf0;
		switch(IDACroute) {
			case 0:
				Temp11[0] |= ADS1148_IDAC2_A0;
				break;
			case 1:
				Temp11[0] |= ADS1148_IDAC2_A1;
				break;
			case 2:
				Temp11[0] |= ADS1148_IDAC2_A2;
				break;
			case 3:
				Temp11[0] |= ADS1148_IDAC2_A3;
				break;
			case 4:
				Temp11[0] |= ADS1148_IDAC2_A4;
				break;
			case 5:
				Temp11[0] |= ADS1148_IDAC2_A5;
				break;
			case 6:
				Temp11[0] |= ADS1148_IDAC2_A6;
				break;
			case 7:
				Temp11[0] |= ADS1148_IDAC2_A7;
				break;
			case 8:
				Temp11[0] |= ADS1148_IDAC2_EXT1;
				break;
			case 9:
				Temp11[0] |= ADS1148_IDAC2_EXT2;
				break;
			case 10:
				Temp11[0] |= ADS1148_IDAC2_EXT1;
				break;
			case 11:
				Temp11[0] |= ADS1148_IDAC2_EXT2;
				break;
			case 12:
				Temp11[0] |= ADS1148_IDAC2_OFF;
				break;
			default:
				Temp11[0] |= ADS1148_IDAC2_OFF;
				dError = ADS1148_ERROR;
		}

	} else {
		Temp11[0] &= 0x0f;
		switch(IDACroute) {
			case 0:
				Temp11[0] |= ADS1148_IDAC1_A0;
				break;
			case 1:
				Temp11[0] |= ADS1148_IDAC1_A1;
				break;
			case 2:
				Temp11[0] |= ADS1148_IDAC1_A2;
				break;
			case 3:
				Temp11[0] |= ADS1148_IDAC1_A3;
				break;
			case 4:
				Temp11[0] |= ADS1148_IDAC1_A4;
				break;
			case 5:
				Temp11[0] |= ADS1148_IDAC1_A5;
				break;
			case 6:
				Temp11[0] |= ADS1148_IDAC1_A6;
				break;
			case 7:
				Temp11[0] |= ADS1148_IDAC1_A7;
				break;
			case 8:
				Temp11[0] |= ADS1148_IDAC1_EXT1;
				break;
			case 9:
				Temp11[0] |= ADS1148_IDAC1_EXT2;
				break;
			case 10:
				Temp11[0] |= ADS1148_IDAC1_EXT1;
				break;
			case 11:
				Temp11[0] |= ADS1148_IDAC1_EXT2;
				break;
			case 12:
				Temp11[0] |= ADS1148_IDAC1_OFF;
				break;
			default:
				Temp11[0] |= ADS1148_IDAC1_OFF;
				dError = ADS1148_ERROR;
		}
	}
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_11_IDAC1, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp11, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return dError;
}
//_____________________________________________________________________________________________________
// YAZ--> GPIOCFG
int ADS1148SetGPIOConfig(unsigned char cdata, uint8_t slave)
{
	uint8_t Temp13[1];
	Temp13[0] = 0x00;
	if (cdata & 0x80)
		Temp13[0] |=  ADS1148_GPIO_7;
	if (cdata & 0x40)
		Temp13[0] |=  ADS1148_GPIO_6;
	if (cdata & 0x20)
		Temp13[0] |=  ADS1148_GPIO_5;
	if (cdata & 0x10)
		Temp13[0] |=  ADS1148_GPIO_4;
	if (cdata & 0x08)
		Temp13[0] |=  ADS1148_GPIO_3;
	if (cdata & 0x04)
		Temp13[0] |=  ADS1148_GPIO_2;
	if (cdata & 0x02)
		Temp13[0] |=  ADS1148_GPIO_1;
	if (cdata & 0x01)
		Temp13[0] |=  ADS1148_GPIO_0;
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_12_GPIOCFG, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp13, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return ADS1148_NO_ERROR;
}
//_____________________________________________________________________________________________________
// YAZ--> GPIODIR
int ADS1148SetGPIODir(unsigned char cdata, uint8_t slave)
{
	uint8_t Temp14[1];
	Temp14[0] = 0x00;
	if (cdata & 0x80)
		Temp14[0] |=  ADS1148_IO_7;
	if (cdata & 0x40)
		Temp14[0] |=  ADS1148_IO_6;
	if (cdata & 0x20)
		Temp14[0] |=  ADS1148_IO_5;
	if (cdata & 0x10)
		Temp14[0] |=  ADS1148_IO_4;
	if (cdata & 0x08)
		Temp14[0] |=  ADS1148_IO_3;
	if (cdata & 0x04)
		Temp14[0] |=  ADS1148_IO_2;
	if (cdata & 0x02)
		Temp14[0] |=  ADS1148_IO_1;
	if (cdata & 0x01)
		Temp14[0] |=  ADS1148_IO_0;
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_13_GPIODIR, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp14, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();

	return ADS1148_NO_ERROR;
}
//_____________________________________________________________________________________________________
// YAZ--> GPIODAT
int ADS1148SetGPIO(unsigned char cdata, uint8_t slave)
{
	uint8_t Temp15[1];
	Temp15[0] = 0x00;
	if (cdata & 0x80)
		Temp15[0] |=  ADS1148_OUT_7;
	if (cdata & 0x40)
		Temp15[0] |=  ADS1148_OUT_6;
	if (cdata & 0x20)
		Temp15[0] |=  ADS1148_OUT_5;
	if (cdata & 0x10)
		Temp15[0] |=  ADS1148_OUT_4;
	if (cdata & 0x08)
		Temp15[0] |=  ADS1148_OUT_3;
	if (cdata & 0x04)
		Temp15[0] |=  ADS1148_OUT_2;
	if (cdata & 0x02)
		Temp15[0] |=  ADS1148_OUT_1;
	if (cdata & 0x01)
		Temp15[0] |=  ADS1148_OUT_0;
	// Yeni Değeri ADC registerına yaz
	ADS1148WriteCommand(ADS1148_14_GPIODAT, 1, slave);

	ADS1148SlaveSelect(slave);

	if(HAL_SPI_Transmit(&hspi1, Temp15, 1, 20) != HAL_OK)
	{
	  /* Transmission Hatası Durumunda err 1 olur */
		 err=1;
	}
	HAL_Delay(10);
	//CS leri tekrar high yap
	ADS1148SlaveOff();
	return ADS1148_NO_ERROR;
}
//_____________________________________________________________________________________________________
int ADS1148SetStart(int nStart)
{
	if (nStart)														//nStart=0 ise START low, nStart=1
																	//ise START high
	{
		  HAL_GPIO_WritePin(GPIOC, IN_START_1_Pin, GPIO_PIN_SET); 	//start pin 1 SET
		  HAL_GPIO_WritePin(GPIOB, IN_START_2_Pin, GPIO_PIN_SET); 	//start pin 2 SET
	}
	else
	{
		  HAL_GPIO_WritePin(GPIOC, IN_START_1_Pin, GPIO_PIN_RESET); //start pin 1 RESET
		  HAL_GPIO_WritePin(GPIOB, IN_START_2_Pin, GPIO_PIN_RESET); //start pin 2 RESET
	}
	HAL_Delay(16);

	return ADS1148_NO_ERROR;
}
//_____________________________________________________________________________________________________
void InitConfig(void)
{
	HAL_NVIC_DisableIRQ(EXTI0_IRQn);
	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
	HAL_Delay(16);

	ADS1148SlaveOff();

	HAL_GPIO_WritePin(GPIOC, RESET_IN_Pin, GPIO_PIN_RESET); 		//RESET pin RESET
	HAL_Delay(10);
	HAL_GPIO_WritePin(GPIOC, RESET_IN_Pin, GPIO_PIN_SET); 			//RESET pin SET
	HAL_Delay(10);

	ADS1148SetStart(1);

	ADS1148SendSDATAC(1);
	ADS1148SendSDATAC(2);
//_____________________________________________________________________________________________________
//_____________________________________________________________________________________________________
	/*REGISTER YAPILANDIRMA: Bu kısımda registerlara yazılması gereken değerler yazılmaktadır*/

	//negative channel-->AIN2/positive channel 1-->AIN4/positive channel 2-->AIN6
/*
	ADS1148SetChannel(4, 0, 1);					 -- Bu şekilede de yazılabilir.
	ADS1148SetChannel(2, 1, 1);
	ADS1148SetChannel(2, 1, 2);
	ADS1148SetChannel(6, 0, 2);
*/
	ADS1148SetChannel(ADS1148_AINP4, 0, 1);      //pmux0 positive inputu seciyor
	ADS1148SetChannel(ADS1148_AINN2, 1, 1);
	ADS1148SetChannel(ADS1148_AINN2, 1, 2);
	ADS1148SetChannel(ADS1148_AINP6, 0, 2);

	//REFP1 and REFN1 reference inputs selected
	ADS1148SetVoltageReference(0, 1);
	ADS1148SetVoltageReference(0, 2);

	//DR = 20 SPS
	ADS1148SetDataRate(2, 1);
	ADS1148SetDataRate(2, 2);

	//all input
	ADS1148SetGPIODir(0xff, 1);
	ADS1148SetGPIODir(0xff, 2);

//_____________________________________________________________________________________________________
//_____________________________________________________________________________________________________
	/*REGISTER DEĞER DOĞRULAMA: Bu kısım registerlara yazılan değerlerin okunup doğrulanması içindir."R
	-egister_Checkup" değişkeni satır sonlarında yazan değerlerle aynı olmalıdır (R: Read only bit)*/

	//Slave 1/Burn-out current source off/AIN4/AIN2
	//Slave 2/Burn-out current source off/AIN6/AIN2
	ADS1148ReadRegister(ADS1148_0_MUX0, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x22-->0010 0010
	ADS1148ReadRegister(ADS1148_0_MUX0, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x32-->0011 0010

	//Bias voltage is disabled for all pins
	ADS1148ReadRegister(ADS1148_1_VBIAS, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_1_VBIAS, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x00-->0000 0000

	//Internal oscillator in use/Internal reference is always off/REFP1 and REFN1 reference inputs sele
	//-cted/Normal operation
	ADS1148ReadRegister(ADS1148_2_MUX1, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x08-->R000 1000
	ADS1148ReadRegister(ADS1148_2_MUX1, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x08-->R000 1000

	//PGA = 1/DR = 20 SPS
	ADS1148ReadRegister(ADS1148_3_SYS0, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x02-->R000 0010
	ADS1148ReadRegister(ADS1148_3_SYS0, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x02-->R000 0010

	//Offset Calibration Register is in its default state: 000000h
	ADS1148ReadRegister(ADS1148_4_OFC0, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_4_OFC0, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_5_OFC1, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_5_OFC1, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_6_OFC2, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_6_OFC2, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x00-->0000 0000

	//Full-Scale Calibration Register is in its default state: 400000h
	ADS1148ReadRegister(ADS1148_7_FSC0, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_7_FSC0, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_8_FSC1, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_8_FSC1, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_9_FSC2, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x40-->0100 0000
	ADS1148ReadRegister(ADS1148_9_FSC2, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x40-->0100 0000

	//DOUT/DRDY pin functions only as Data Out/IDAC off
	ADS1148ReadRegister(ADS1148_10_IDAC0, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0x90-->RRRR 0000
	ADS1148ReadRegister(ADS1148_10_IDAC0, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0x90-->RRRR 0000

	//Disconnected/Disconnected
	ADS1148ReadRegister(ADS1148_11_IDAC1, 1, Register_Checkup, 1);	//REGISTER DEĞERİ: 0xff-->1111 1111
	ADS1148ReadRegister(ADS1148_11_IDAC1, 1, Register_Checkup, 2);	//REGISTER DEĞERİ: 0xff-->1111 1111

	//All GPIO Disabled
	ADS1148ReadRegister(ADS1148_12_GPIOCFG, 1, Register_Checkup, 1);//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_12_GPIOCFG, 1, Register_Checkup, 2);//REGISTER DEĞERİ: 0x00-->0000 0000

	//All input
	ADS1148ReadRegister(ADS1148_13_GPIODIR, 1, Register_Checkup, 1);//REGISTER DEĞERİ: 0xff-->1111 1111
	ADS1148ReadRegister(ADS1148_13_GPIODIR, 1, Register_Checkup, 2);//REGISTER DEĞERİ: 0xff-->1111 1111

	//All low
	ADS1148ReadRegister(ADS1148_14_GPIODAT, 1, Register_Checkup, 1);//REGISTER DEĞERİ: 0x00-->0000 0000
	ADS1148ReadRegister(ADS1148_14_GPIODAT, 1, Register_Checkup, 2);//REGISTER DEĞERİ: 0x00-->0000 0000
//_____________________________________________________________________________________________________

	ADS1148SendSync(1);
	ADS1148SendSync(2);

	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	no_exti = 0;

	return;
}
//_____________________________________________________________________________________________________
//_____________________________________________________________________________________________________
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == IN_DRDY_1_Pin && no_exti == 0)
	{
		DRDY_1_Falling_Edge = 1;
	}

	else if(GPIO_Pin == IN_DRDY_2_Pin && no_exti == 0)
	{
		DRDY_2_Falling_Edge = 1;
	}
	else
	{
		asm("nop");
	}
}

//_____________________________________________________________________________________________________
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
