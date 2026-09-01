/* main.c - I2S/DMA audio acquisition and UART PCM streaming using INMP441. */

#include "main.h"
#include <string.h>

/* ================= PERIPHERALS ================= */

I2S_HandleTypeDef hi2s3;
DMA_NodeTypeDef Node_GPDMA1_Channel0 __NON_CACHEABLE;
DMA_QListTypeDef List_GPDMA1_Channel0;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

UART_HandleTypeDef huart1;

/* ================= AUDIO CONFIG ================= */

#define DMA_BUFFER_SIZE 512
uint32_t dma_buffer[DMA_BUFFER_SIZE];   /* Raw 32-bit I2S frames. */

/* Ping-pong buffers */
#define BLOCK_SIZE (DMA_BUFFER_SIZE / 2)

uint32_t audio_block_a[BLOCK_SIZE];
uint32_t audio_block_b[BLOCK_SIZE];

volatile uint8_t block_a_ready = 0;
volatile uint8_t block_b_ready = 0;

/* UART */
#define UART_CHUNK 128
int16_t uart_buffer[UART_CHUNK];
uint16_t uart_index = 0;

/* ================= PROTOTYPES ================= */

static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_I2S3_Init(void);
static void MX_USART1_UART_Init(void);
static void SystemIsolation_Config(void);

void process_samples(uint32_t *data, uint32_t length);

/* ================= MAIN ================= */

int main(void)
{
    HAL_Init();

    MX_GPIO_Init();
    MX_GPDMA1_Init();
    MX_I2S3_Init();
    MX_USART1_UART_Init();
    SystemIsolation_Config();

    /* Start continuous I2S reception using DMA. */
    HAL_I2S_Receive_DMA(&hi2s3, (uint16_t*)dma_buffer, DMA_BUFFER_SIZE);

    while (1)
    {
        if (block_a_ready)
        {
            block_a_ready = 0;
            process_samples(audio_block_a, BLOCK_SIZE);
        }

        if (block_b_ready)
        {
            block_b_ready = 0;
            process_samples(audio_block_b, BLOCK_SIZE);
        }
    }
}

/* ================= AUDIO PROCESS ================= */

void process_samples(uint32_t *data, uint32_t length)
{
    for (uint32_t i = 0; i < length; i += 2)
    {
        /* Extract the 24-bit INMP441 sample from the 32-bit I2S word. */
        int32_t sample = (int32_t)data[i];

        /* Sign-extend the 24-bit sample to 32 bits. */
        sample &= 0x00FFFFFF;
        if (sample & 0x00800000)
            sample |= 0xFF000000;

        /* Scale the 24-bit PCM sample before conversion to 16 bits. */
        sample = sample >> 10;

        int16_t sample16 = (int16_t)sample;

        uart_buffer[uart_index++] = sample16;

        if (uart_index >= UART_CHUNK)
        {
            HAL_UART_Transmit(&huart1,
                              (uint8_t*)uart_buffer,
                              UART_CHUNK * sizeof(int16_t),
                              HAL_MAX_DELAY);

            uart_index = 0;
        }
    }
}

/* ================= DMA CALLBACKS ================= */

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    memcpy(audio_block_a,
           dma_buffer,
           sizeof(audio_block_a));

    block_a_ready = 1;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    memcpy(audio_block_b,
           &dma_buffer[BLOCK_SIZE],
           sizeof(audio_block_b));

    block_b_ready = 1;
}

/* ================= INIT ================= */

static void MX_GPDMA1_Init(void)
{
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
}

static void MX_I2S3_Init(void)
{
    hi2s3.Instance = SPI3;
    hi2s3.Init.Mode = I2S_MODE_MASTER_RX;
    hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat = I2S_DATAFORMAT_32B;
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

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 921600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_UARTEx_DisableFifoMode(&huart1);
}

static void SystemIsolation_Config(void)
{
    __HAL_RCC_RIFSC_CLK_ENABLE();

    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_SPI3,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);

    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_USART1,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);

    HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0,
                                    DMA_CHANNEL_SEC | DMA_CHANNEL_PRIV |
                                    DMA_CHANNEL_SRC_SEC | DMA_CHANNEL_DEST_SEC);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
