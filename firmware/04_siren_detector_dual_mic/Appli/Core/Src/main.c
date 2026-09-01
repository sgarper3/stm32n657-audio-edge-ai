/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "app_x-cube-ai.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "arm_math.h"
#include "audio_features.h"
#include "multi_test_inputs_int8.h"
/* USER CODE END Includes */
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

#define AI_INPUT_SOURCE_HEADER  1
#define AI_INPUT_SOURCE_MIC     2

/* Select either AI_INPUT_SOURCE_HEADER or AI_INPUT_SOURCE_MIC. */
#define AI_INPUT_SOURCE         AI_INPUT_SOURCE_MIC

#define FRAME_SIZE              1024
#define HOP_SIZE                320
#define MEL_BINS                64
#define TIME_FRAMES             50
#define MEL_INPUT_SIZE          3200

#define AUDIO_BLOCK_SIZE        1024
#define AUDIO_WINDOW_SAMPLES    16000

#define AI_INPUT_SCALE          0.3137255013f
#define AI_INPUT_ZERO_POINT     127

/* =========================
   DUAL MICROPHONE MODE
   ========================= */
#define MIC_MODE_LEFT           1
#define MIC_MODE_RIGHT          2
#define MIC_MODE_AVERAGE        3

#define MIC_MODE                MIC_MODE_AVERAGE

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

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

/* =========================
   AI INPUT BUFFERS
   ========================= */

float feature_buffer[MEL_INPUT_SIZE];
float feature_buffer_ready[MEL_INPUT_SIZE];

int8_t feature_buffer_q_ready[MEL_INPUT_SIZE] __NON_CACHEABLE;

/* =========================
   MICROPHONE / DMA BUFFERS
   ========================= */

__attribute__((aligned(32)))
uint32_t pcm_buffer[AUDIO_BLOCK_SIZE];

__attribute__((aligned(32)))
uint32_t audio_block_a[AUDIO_BLOCK_SIZE / 2];

__attribute__((aligned(32)))
uint32_t audio_block_b[AUDIO_BLOCK_SIZE / 2];

volatile uint8_t block_a_ready = 0;
volatile uint8_t block_b_ready = 0;

__attribute__((aligned(32)))
int32_t audio_window[AUDIO_WINDOW_SAMPLES];

volatile uint32_t audio_window_index = 0;
volatile uint8_t audio_window_ready = 0;

static float window[FRAME_SIZE];

volatile int32_t last_left_sample = 0;
volatile int32_t last_right_sample = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_CACHEAXI_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_RAMCFG_Init(void);
static void MX_I2S3_Init(void);
static void SystemIsolation_Config(void);
/* USER CODE BEGIN PFP */

void load_multi_test_input_q(int sample_id);

void init_window(void);
void process_audio(uint32_t *samples, int len);
void compute_features_from_window(void);
void power_to_db_global(float *buffer);
void quantize_features_to_int8(void);

static int8_t quantize_input_float_to_int8(float x);
static int32_t decode_i2s_24bit(uint32_t sample);

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

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_CACHEAXI_Init();
  MX_LPUART1_UART_Init();

  HAL_Delay(1000);

  printf("\r\n\r\n========================================\r\n");
  printf("BOOT: UART READY - Neural-ART INT8 Resnet_siren\r\n");
  printf("========================================\r\n");
  fflush(stdout);

  printf("BOOT: continuing initialization\r\n");
  fflush(stdout);

  /* Enable AXISRAM clocks */
  __HAL_RCC_AXISRAM1_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM2_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM3_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_ENABLE();
  __HAL_RCC_FLEXRAM_MEM_CLK_ENABLE();
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_ENABLE();

  /* Wake up AXISRAM3-6: mandatory for Neural-ART buffers in cpuRAM3-6 */
  RAMCFG_SRAM3_AXI->CR &= ~RAMCFG_CR_SRAMSD;
  RAMCFG_SRAM4_AXI->CR &= ~RAMCFG_CR_SRAMSD;
  RAMCFG_SRAM5_AXI->CR &= ~RAMCFG_CR_SRAMSD;
  RAMCFG_SRAM6_AXI->CR &= ~RAMCFG_CR_SRAMSD;

  __DSB();
  __ISB();

  MX_RAMCFG_Init();
  printf("BOOT: RAMCFG OK - AXISRAM3-6 awake\r\n");
  fflush(stdout);

  MX_I2S3_Init();
  printf("BOOT: I2S3 OK\r\n");
  fflush(stdout);

  SystemIsolation_Config();
  printf("BOOT: SystemIsolation OK\r\n");
  fflush(stdout);

  MX_X_CUBE_AI_Init();
  printf("BOOT: AI INIT OK\r\n");
  fflush(stdout);

  printf("CLOCK CPU=%lu Hz HCLK=%lu Hz\r\n",
         (unsigned long)HAL_RCC_GetCpuClockFreq(),
         (unsigned long)HAL_RCC_GetHCLKFreq());
  fflush(stdout);
  /* USER CODE BEGIN 2 */

  printf("Initializing buffers\r\n");
  fflush(stdout);

  memset(feature_buffer, 0, sizeof(feature_buffer));
  memset(feature_buffer_ready, 0, sizeof(feature_buffer_ready));
  memset(feature_buffer_q_ready, 0, sizeof(feature_buffer_q_ready));

  memset(pcm_buffer, 0, sizeof(pcm_buffer));
  memset(audio_block_a, 0, sizeof(audio_block_a));
  memset(audio_block_b, 0, sizeof(audio_block_b));
  memset(audio_window, 0, sizeof(audio_window));

  block_a_ready = 0;
  block_b_ready = 0;
  audio_window_ready = 0;
  audio_window_index = 0;

  init_window();

  #if AI_INPUT_SOURCE == AI_INPUT_SOURCE_HEADER

  printf("\r\nMODE: HEADER INT8 TEST - Resnet_siren\r\n");
  fflush(stdout);

  int sample_id = 17;

  printf("SINGLE INT8 TEST sample_id=%d\r\n", sample_id);
  printf("label=%d expected_keras=%f expected_tflite_int8_raw=%d expected_tflite_int8_prob=%f\r\n",
         test_labels[sample_id],
         expected_keras_float32[sample_id],
         expected_tflite_int8_raw[sample_id],
         expected_tflite_int8_prob[sample_id]);
  fflush(stdout);

  load_multi_test_input_q(sample_id);

  MX_X_CUBE_AI_Process();

  printf("SINGLE INT8 TEST DONE\r\n");
  fflush(stdout);

  #elif AI_INPUT_SOURCE == AI_INPUT_SOURCE_MIC

  printf("\r\nMODE: MICROPHONE REAL AUDIO - Resnet_siren\r\n");
  fflush(stdout);

  if (HAL_I2S_Receive_DMA(&hi2s3,
                          (uint16_t *)pcm_buffer,
                          AUDIO_BLOCK_SIZE) != HAL_OK)
  {
      Error_Handler();
  }

  printf("DMA STARTED\r\n");
  fflush(stdout);

  #else

  printf("ERROR: invalid AI_INPUT_SOURCE\r\n");
  fflush(stdout);
  Error_Handler();

  #endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  #if AI_INPUT_SOURCE == AI_INPUT_SOURCE_MIC

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

        printf("\r\nAUDIO WINDOW READY\r\n");
        printf("LAST I2S left=%ld right=%ld MIC_MODE=%d\r\n",
               (long)last_left_sample,
               (long)last_right_sample,
               MIC_MODE);
        fflush(stdout);

        uint32_t t0 = HAL_GetTick();

        compute_features_from_window();
        quantize_features_to_int8();

        uint32_t t1 = HAL_GetTick();

        printf("PREPROCESS + QUANT dt=%lu ms\r\n",
               (unsigned long)(t1 - t0));
        fflush(stdout);

        MX_X_CUBE_AI_Process();

        audio_window_index = 0;
        memset(audio_window, 0, sizeof(audio_window));

        block_a_ready = 0;
        block_b_ready = 0;

        if (HAL_I2S_Receive_DMA(&hi2s3,
                                (uint16_t *)pcm_buffer,
                                AUDIO_BLOCK_SIZE) != HAL_OK)
        {
            Error_Handler();
        }
    }

  #else

    static uint32_t last_tick = 0;
    static uint32_t heartbeat_count = 0;
    uint32_t now = HAL_GetTick();

    if ((now - last_tick) >= 10000)
    {
        last_tick = now;
        heartbeat_count++;

        printf("HEARTBEAT %lu - application alive, tick=%lu ms\r\n",
               (unsigned long)heartbeat_count,
               (unsigned long)now);
        fflush(stdout);
    }

  #endif

    HAL_Delay(1);

    /* USER CODE END 3 */
  }
}

/**
  * @brief CACHEAXI Initialization Function
  * @param None
  * @retval None
  */
static void MX_CACHEAXI_Init(void)
{

  /* USER CODE BEGIN CACHEAXI_Init 0 */

  /* USER CODE END CACHEAXI_Init 0 */

  /* USER CODE BEGIN CACHEAXI_Init 1 */

  /* USER CODE END CACHEAXI_Init 1 */
  hcacheaxi.Instance = CACHEAXI;
  if (HAL_CACHEAXI_Init(&hcacheaxi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CACHEAXI_Init 2 */

  /* USER CODE END CACHEAXI_Init 2 */

}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
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
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
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
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief RAMCFG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RAMCFG_Init(void)
{

  /* USER CODE BEGIN RAMCFG_Init 0 */

  /* USER CODE END RAMCFG_Init 0 */

  /* USER CODE BEGIN RAMCFG_Init 1 */

  /* USER CODE END RAMCFG_Init 1 */

  /** Initialize RAMCFG SRAM3
  */
  hramcfg_SRAM3.Instance = RAMCFG_SRAM3_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initialize RAMCFG SRAM4
  */
  hramcfg_SRAM4.Instance = RAMCFG_SRAM4_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initialize RAMCFG SRAM5
  */
  hramcfg_SRAM5.Instance = RAMCFG_SRAM5_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initialize RAMCFG SRAM6
  */
  hramcfg_SRAM6.Instance = RAMCFG_SRAM6_AXI;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RAMCFG_Init 2 */

  /* USER CODE END RAMCFG_Init 2 */

}

/**
  * @brief RIF Initialization Function
  * @param None
  * @retval None
  */
  static void SystemIsolation_Config(void)
{

  /* USER CODE BEGIN RIF_Init 0 */

  /* USER CODE END RIF_Init 0 */

  /* set all required IPs as secure privileged */
  __HAL_RCC_RIFSC_CLK_ENABLE();

  RIMC_MasterConfig_t master_conf;

  master_conf.MasterCID = RIF_CID_1;
  master_conf.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;

  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_NPU, &master_conf);

  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU,
                                        RIF_ATTRIBUTE_PRIV | RIF_ATTRIBUTE_SEC);

  /* RIF-Aware IPs Config */

  /* set up GPDMA configuration */
  /* set GPDMA1 channel 0 used by I2S3 */
  if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0,DMA_CHANNEL_SEC|DMA_CHANNEL_PRIV|DMA_CHANNEL_SRC_SEC|DMA_CHANNEL_DEST_SEC)!= HAL_OK )
  {
    Error_Handler();
  }

  /* set up GPIO configuration */
  HAL_GPIO_ConfigPinAttributes(GPIOA,GPIO_PIN_11,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB,GPIO_PIN_12,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOC,GPIO_PIN_10,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOC,GPIO_PIN_11,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD,GPIO_PIN_7,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOE,GPIO_PIN_5,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOE,GPIO_PIN_6,GPIO_PIN_SEC|GPIO_PIN_NPRIV);

  /* USER CODE BEGIN RIF_Init 1 */

  /* USER CODE END RIF_Init 1 */
  /* USER CODE BEGIN RIF_Init 2 */

  /* USER CODE END RIF_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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

void load_multi_test_input_q(int sample_id)
{
    if (sample_id < 0 || sample_id >= NUM_TEST_INPUTS)
    {
        printf("ERROR: invalid sample_id=%d\r\n", sample_id);
        fflush(stdout);
        Error_Handler();
    }

    if (TEST_INPUT_SIZE != 3200)
    {
        printf("ERROR: TEST_INPUT_SIZE=%lu, expected=3200\r\n",
               (unsigned long)TEST_INPUT_SIZE);
        fflush(stdout);
        Error_Handler();
    }

    memcpy(feature_buffer_q_ready,
           test_inputs_q[sample_id],
           sizeof(int8_t) * TEST_INPUT_SIZE);

    int8_t q_min = feature_buffer_q_ready[0];
    int8_t q_max = feature_buffer_q_ready[0];

    for (int i = 1; i < TEST_INPUT_SIZE; i++)
    {
        if (feature_buffer_q_ready[i] < q_min) q_min = feature_buffer_q_ready[i];
        if (feature_buffer_q_ready[i] > q_max) q_max = feature_buffer_q_ready[i];
    }

    printf("MULTI INT8 INPUT LOADED sample_id=%d\r\n", sample_id);
    printf("q[0]=%d q[1]=%d q[2]=%d q[100]=%d q[1000]=%d q[3199]=%d q_min=%d q_max=%d\r\n",
           (int)feature_buffer_q_ready[0],
           (int)feature_buffer_q_ready[1],
           (int)feature_buffer_q_ready[2],
           (int)feature_buffer_q_ready[100],
           (int)feature_buffer_q_ready[1000],
           (int)feature_buffer_q_ready[3199],
           (int)q_min,
           (int)q_max);
    fflush(stdout);
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

static int32_t decode_i2s_24bit(uint32_t sample)
{
    int32_t s = (int32_t)sample;

    s &= 0x00FFFFFF;

    if (s & 0x00800000)
        s |= 0xFF000000;

    return s;
}

void process_audio(uint32_t *samples, int len)
{
    for (int i = 0; i < len; i += 2)
    {
        int32_t left  = decode_i2s_24bit(samples[i]);
        int32_t right = decode_i2s_24bit(samples[i + 1]);

        last_left_sample = left;
        last_right_sample = right;

        int32_t s = 0;

#if MIC_MODE == MIC_MODE_LEFT

        s = left;

#elif MIC_MODE == MIC_MODE_RIGHT

        s = right;

#elif MIC_MODE == MIC_MODE_AVERAGE

        s = (int32_t)(((int64_t)left + (int64_t)right) / 2);

#else

#error "Invalid MIC_MODE"

#endif

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

void compute_features_from_window(void)
{
    static float frame[FRAME_SIZE];

    memset(feature_buffer, 0, sizeof(feature_buffer));
    memset(feature_buffer_ready, 0, sizeof(feature_buffer_ready));

    /*
     * Apply librosa-style centered framing by virtually zero-padding
     * FRAME_SIZE/2 samples at both ends of the 1-second audio window.
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
                    (1.0f / 8388608.0f);
            }

            frame[j] = sample * window[j];
        }

        compute_features(frame, feature_buffer, t);
    }

    power_to_db_global(feature_buffer);

    memcpy(feature_buffer_ready,
           feature_buffer,
           sizeof(feature_buffer_ready));

    float min_v = feature_buffer_ready[0];
    float max_v = feature_buffer_ready[0];

    for (int i = 1; i < MEL_INPUT_SIZE; i++)
    {
        if (feature_buffer_ready[i] < min_v) min_v = feature_buffer_ready[i];
        if (feature_buffer_ready[i] > max_v) max_v = feature_buffer_ready[i];
    }

    printf("FEATURES READY float min=%f max=%f\r\n", min_v, max_v);
    printf("features[0]=%f features[1]=%f features[2]=%f features[100]=%f features[1000]=%f features[3199]=%f\r\n",
           feature_buffer_ready[0],
           feature_buffer_ready[1],
           feature_buffer_ready[2],
           feature_buffer_ready[100],
           feature_buffer_ready[1000],
           feature_buffer_ready[3199]);
    fflush(stdout);
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

static int8_t quantize_input_float_to_int8(float x)
{
    int32_t q = (int32_t)lroundf((x / AI_INPUT_SCALE) + (float)AI_INPUT_ZERO_POINT);

    if (q > 127)  q = 127;
    if (q < -128) q = -128;

    return (int8_t)q;
}

void quantize_features_to_int8(void)
{
    int8_t q_min;
    int8_t q_max;

    for (int i = 0; i < MEL_INPUT_SIZE; i++)
    {
        feature_buffer_q_ready[i] =
            quantize_input_float_to_int8(feature_buffer_ready[i]);
    }

    q_min = feature_buffer_q_ready[0];
    q_max = feature_buffer_q_ready[0];

    for (int i = 1; i < MEL_INPUT_SIZE; i++)
    {
        if (feature_buffer_q_ready[i] < q_min) q_min = feature_buffer_q_ready[i];
        if (feature_buffer_q_ready[i] > q_max) q_max = feature_buffer_q_ready[i];
    }

    printf("FEATURES QUANTIZED INT8 q[0]=%d q[1]=%d q[2]=%d q[100]=%d q[1000]=%d q[3199]=%d q_min=%d q_max=%d\r\n",
           (int)feature_buffer_q_ready[0],
           (int)feature_buffer_q_ready[1],
           (int)feature_buffer_q_ready[2],
           (int)feature_buffer_q_ready[100],
           (int)feature_buffer_q_ready[1000],
           (int)feature_buffer_q_ready[3199],
           (int)q_min,
           (int)q_max);
    fflush(stdout);
}

void init_window(void)
{
    for (int i = 0; i < FRAME_SIZE; i++)
    {
        window[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / FRAME_SIZE));
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
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
