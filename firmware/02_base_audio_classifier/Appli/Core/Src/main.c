/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "app_x-cube-ai.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "arm_math.h"
#include "audio_features.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PD */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define FRAME_SIZE       1024
#define HOP_SIZE         320
#define MEL_BINS         64
#define TIME_FRAMES 	 50
#define MEL_INPUT_SIZE   3200

#define AUDIO_BLOCK_SIZE 1024

static float window[FRAME_SIZE];
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
CACHEAXI_HandleTypeDef hcacheaxi;

I2S_HandleTypeDef hi2s3;
DMA_NodeTypeDef Node_GPDMA1_Channel0 __NON_CACHEABLE;
DMA_QListTypeDef List_GPDMA1_Channel0;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

UART_HandleTypeDef hlpuart1;

RAMCFG_HandleTypeDef hramcfg_SRAM3;
RAMCFG_HandleTypeDef hramcfg_SRAM4;
RAMCFG_HandleTypeDef hramcfg_SRAM5;
RAMCFG_HandleTypeDef hramcfg_SRAM6;

/* USER CODE BEGIN PV */
#define AUDIO_WINDOW_SAMPLES 16000

__attribute__((aligned(32)))
int32_t audio_window[AUDIO_WINDOW_SAMPLES];

volatile uint32_t audio_window_index = 0;
volatile uint8_t audio_window_ready = 0;

__attribute__((aligned(32)))
uint32_t pcm_buffer[AUDIO_BLOCK_SIZE];

float feature_buffer[MEL_INPUT_SIZE];
float feature_buffer_ready[MEL_INPUT_SIZE];

__attribute__((aligned(32)))
uint32_t audio_block_a[AUDIO_BLOCK_SIZE / 2];

__attribute__((aligned(32)))
uint32_t audio_block_b[AUDIO_BLOCK_SIZE / 2];

volatile uint8_t block_a_ready = 0;
volatile uint8_t block_b_ready = 0;

/* USER CODE END PV */

static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_CACHEAXI_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_RAMCFG_Init(void);
static void MX_I2S3_Init(void);
static void SystemIsolation_Config(void);

/* USER CODE BEGIN PFP */
void compute_features_from_window(void);
void power_to_db_global(float *buffer);
void process_audio(uint32_t *samples, int len);
void init_window(void);
/* USER CODE END PFP */

int main(void)
{
  HAL_Init();

  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_CACHEAXI_Init();
  MX_LPUART1_UART_Init();
  MX_RAMCFG_Init();
  MX_I2S3_Init();

  SystemIsolation_Config();

  printf("BOOT\r\n");
  fflush(stdout);

  MX_X_CUBE_AI_Init();

  printf("AI INIT OK\r\n");
  fflush(stdout);

  memset(feature_buffer, 0, sizeof(feature_buffer));
  memset(feature_buffer_ready, 0, sizeof(feature_buffer_ready));

  block_a_ready = 0;
  block_b_ready = 0;

  init_window();

  if (HAL_I2S_Receive_DMA(&hi2s3,
                          (uint16_t*)pcm_buffer,
                          AUDIO_BLOCK_SIZE) != HAL_OK)
  {
      Error_Handler();
  }

  printf("DMA STARTED\r\n");
  fflush(stdout);

  while (1)
  {
      if (block_a_ready)
      {
          __disable_irq();
          block_a_ready = 0;
          __enable_irq();

          process_audio(audio_block_a, AUDIO_BLOCK_SIZE / 2);
      }

      if (block_b_ready)
      {
          __disable_irq();
          block_b_ready = 0;
          __enable_irq();

          process_audio(audio_block_b, AUDIO_BLOCK_SIZE / 2);
      }

      if (audio_window_ready)
      {
          __disable_irq();
          audio_window_ready = 0;
          __enable_irq();

          HAL_I2S_DMAStop(&hi2s3);

          compute_features_from_window();

          MX_X_CUBE_AI_Process();

          audio_window_index = 0;
          memset(audio_window, 0, sizeof(audio_window));

          block_a_ready = 0;
          block_b_ready = 0;

          if (HAL_I2S_Receive_DMA(&hi2s3,
                                  (uint16_t*)pcm_buffer,
                                  AUDIO_BLOCK_SIZE) != HAL_OK)
          {
              Error_Handler();
          }
      }

      HAL_Delay(1);
  }
}




/**
  * @brief CACHEAXI Initialization Function
  */
static void MX_CACHEAXI_Init(void)
{
  hcacheaxi.Instance = CACHEAXI;
  if (HAL_CACHEAXI_Init(&hcacheaxi) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPDMA1 Initialization Function
  */
static void MX_GPDMA1_Init(void)
{
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
}

/**
  * @brief I2S3 Initialization Function
  */
static void MX_I2S3_Init(void)
{
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_16K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.FirstBit = I2S_FIRSTBIT_MSB;
  hi2s3.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
  hi2s3.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
  hi2s3.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;

  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  */
static void MX_LPUART1_UART_Init(void)
{
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 921600;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;

  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RAMCFG Initialization Function
  */
static void MX_RAMCFG_Init(void)
{
  hramcfg_SRAM3.Instance = RAMCFG_SRAM3_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM3) != HAL_OK)
  {
    Error_Handler();
  }

  hramcfg_SRAM4.Instance = RAMCFG_SRAM4_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM4) != HAL_OK)
  {
    Error_Handler();
  }

  hramcfg_SRAM5.Instance = RAMCFG_SRAM5_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM5) != HAL_OK)
  {
    Error_Handler();
  }

  hramcfg_SRAM6.Instance = RAMCFG_SRAM6_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RIF Initialization Function
  */
static void SystemIsolation_Config(void)
{
  __HAL_RCC_RIFSC_CLK_ENABLE();

  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU,
                                        RIF_ATTRIBUTE_PRIV | RIF_ATTRIBUTE_SEC);

  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_SPI3,
                                        RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);

  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LPUART1,
                                        RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);

  if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0,
                                      DMA_CHANNEL_SEC |
                                      DMA_CHANNEL_PRIV |
                                      DMA_CHANNEL_SRC_SEC |
                                      DMA_CHANNEL_DEST_SEC) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_11, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_12, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_10, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_11, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_7, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_5, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_6, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */

PUTCHAR_PROTOTYPE
{
    uint8_t c = (uint8_t)ch;
    HAL_UART_Transmit(&hlpuart1, &c, 1, 0xFFFF);
    return ch;
}

int _write(int fd, char *ptr, int len)
{
    (void)fd;
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}



void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance != SPI3)
        return;

    memcpy(audio_block_a,
           &pcm_buffer[0],
           sizeof(audio_block_a));

    block_a_ready = 1;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance != SPI3)
        return;

    memcpy(audio_block_b,
           &pcm_buffer[AUDIO_BLOCK_SIZE / 2],
           sizeof(audio_block_b));

    block_b_ready = 1;
}

void compute_features_from_window(void)
{
    static float frame[FRAME_SIZE];

    memset(feature_buffer, 0, sizeof(feature_buffer));
    memset(feature_buffer_ready, 0, sizeof(feature_buffer_ready));

    /*
    * Apply librosa-style centered framing by virtually zero-padding
    * FRAME_SIZE/2 samples at both ends of the 1-second audio window.
    *
    * For frame t:
    * input index = t * HOP_SIZE + j - FRAME_SIZE/2
    */
    for (int t = 0; t < TIME_FRAMES; t++)
    {
        int start = t * HOP_SIZE - (FRAME_SIZE / 2);

        for (int j = 0; j < FRAME_SIZE; j++)
        {
            int idx = start + j;

            float sample = 0.0f;

            if (idx >= 0 && idx < AUDIO_WINDOW_SAMPLES)
            {
                sample =
                    (float)audio_window[idx] *
                    (1.0f / 8388608.0f);        /* Normalize signed 24-bit PCM to approximately [-1, 1). */
            }

            frame[j] = sample * window[j];
        }

        compute_features(frame, feature_buffer, t);
    }

    power_to_db_global(feature_buffer);

    float min_val = 1e9f;
    float max_val = -1e9f;

    for (int i = 0; i < MEL_INPUT_SIZE; i++)
    {
        if (feature_buffer[i] < min_val)
            min_val = feature_buffer[i];

        if (feature_buffer[i] > max_val)
            max_val = feature_buffer[i];
    }

    printf("LogMel min = %.2f  max = %.2f\r\n", min_val, max_val);

    memcpy(feature_buffer_ready,
           feature_buffer,
           sizeof(feature_buffer_ready));
}



void power_to_db_global(float *buffer)
{
    float max_power = 1.0e-10f;

    for (int m = 0; m < MEL_BINS; m++)
    {
        for (int t = 0; t < TIME_FRAMES; t++)
        {
            float v = buffer[m * TIME_FRAMES + t];

            if (isfinite(v) && v > max_power)
                max_power = v;
        }
    }

    float ref_db = 10.0f * log10f(max_power);
    float max_db = -1000.0f;

    for (int m = 0; m < MEL_BINS; m++)
    {
        for (int t = 0; t < TIME_FRAMES; t++)
        {
            int i = m * TIME_FRAMES + t;

            float v = buffer[i];

            if (!isfinite(v) || v < 1.0e-10f)
                v = 1.0e-10f;

            float db = 10.0f * log10f(v) - ref_db;

            if (!isfinite(db))
                db = -100.0f;

            buffer[i] = db;

            if (db > max_db)
                max_db = db;
        }
    }

    float min_db = max_db - 80.0f;

    for (int m = 0; m < MEL_BINS; m++)
    {
        for (int t = 0; t < TIME_FRAMES; t++)
        {
            int i = m * TIME_FRAMES + t;

            if (buffer[i] < min_db)
                buffer[i] = min_db;
        }
    }
}


void process_audio(uint32_t *samples, int len)
{
    for (int i = 0; i < len; i += 2)
    {
        int32_t s = (int32_t)samples[i];

        /* Sign-extend the 24-bit PCM sample to 32 bits. */
        s &= 0x00FFFFFF;
        if (s & 0x00800000)
            s |= 0xFF000000;

        if (audio_window_index < AUDIO_WINDOW_SAMPLES)
        {
            audio_window[audio_window_index++] = s;

            if (audio_window_index >= AUDIO_WINDOW_SAMPLES)
            {
                audio_window_ready = 1;
                return;
            }
        }
    }
}

void init_window(void)
{
    for (int i = 0; i < FRAME_SIZE; i++)
    {
    	window[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / FRAME_SIZE));
    }
}

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
