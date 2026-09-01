/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
  ******************************************************************************
  */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "main.h"

/* USER CODE BEGIN includes */
/* Quantized input features prepared in main.c. */
extern int8_t feature_buffer_q_ready[3200];

#define SIREN_THRESHOLD      0.80f
#define AI_INPUT_SIZE        3200
#define AI_OUTPUT_SIZE       1

/* INT8 output quantization parameters of the deployed model. */
#define AI_OUTPUT_SCALE       0.0039062500f
#define AI_OUTPUT_ZERO_POINT  -128

/* Runtime counters used for debugging and validation. */
volatile uint32_t ai_process_count = 0;
volatile uint32_t ai_acquire_ok_count = 0;
volatile uint32_t ai_run_ok_count = 0;
volatile uint32_t ai_post_ok_count = 0;
volatile uint32_t ai_error_count = 0;
volatile uint32_t ai_returned_to_main = 0;

/* Inference results exposed for debugging and validation. */
volatile float ai_prob_siren = 0.0f;
volatile int ai_siren_detected = 0;
volatile int ai_last_class = -1;
volatile float ai_last_score = 0.0f;
volatile int ai_last_res = 0;

/* USER CODE END includes */

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default)

uint8_t *buffer_in;
uint8_t *buffer_out;

void set_clk_sleep_mode(void)
{
#if defined (CPU_IN_SECURE_STATE)
  __HAL_RCC_DBG_CLK_SLEEP_ENABLE();
#endif

  __HAL_RCC_XSPIPHYCOMP_CLK_SLEEP_ENABLE();

  __HAL_RCC_AXISRAM1_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM2_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM3_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_FLEXRAM_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_SLEEP_ENABLE();

#if defined (CPU_IN_SECURE_STATE)
  __HAL_RCC_RIFSC_CLK_SLEEP_ENABLE();
  __HAL_RCC_RISAF_CLK_SLEEP_ENABLE();
  __HAL_RCC_IAC_CLK_SLEEP_ENABLE();
#endif

  __HAL_RCC_XSPI1_CLK_SLEEP_ENABLE();
  __HAL_RCC_XSPI2_CLK_SLEEP_ENABLE();
  __HAL_RCC_CACHEAXI_CLK_SLEEP_ENABLE();
  __HAL_RCC_NPU_CLK_SLEEP_ENABLE();

  __HAL_RCC_USART1_CLK_SLEEP_ENABLE();
}

/* USER CODE BEGIN 2 */

static void clean_dcache_region(void *addr, uint32_t size)
{
  uint32_t start = (uint32_t)addr & ~((uint32_t)31);
  uint32_t end   = ((uint32_t)addr + size + 31) & ~((uint32_t)31);

  SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static void invalidate_dcache_region(void *addr, uint32_t size)
{
  uint32_t start = (uint32_t)addr & ~((uint32_t)31);
  uint32_t end   = ((uint32_t)addr + size + 31) & ~((uint32_t)31);

  SCB_InvalidateDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static int acquire_and_process_data(uint8_t *input_buffer)
{
  if (input_buffer == NULL)
  {
    ai_last_res = -10;
    ai_error_count++;
    return -1;
  }

  memcpy(input_buffer,
         feature_buffer_q_ready,
         AI_INPUT_SIZE * sizeof(int8_t));

  clean_dcache_region(input_buffer, AI_INPUT_SIZE * sizeof(int8_t));

  int8_t *in = (int8_t *)input_buffer;

  int8_t q_min = in[0];
  int8_t q_max = in[0];

  for (int i = 1; i < AI_INPUT_SIZE; i++)
  {
    if (in[i] < q_min) q_min = in[i];
    if (in[i] > q_max) q_max = in[i];
  }

  printf("INT8 INPUT q[0]=%d q[1]=%d q[2]=%d q[100]=%d q[1000]=%d q[3199]=%d q_min=%d q_max=%d\r\n",
         (int)in[0],
         (int)in[1],
         (int)in[2],
         (int)in[100],
         (int)in[1000],
         (int)in[3199],
         (int)q_min,
         (int)q_max);
  fflush(stdout);

  ai_acquire_ok_count++;
  return 0;
}

static int post_process(uint8_t *output_buffer)
{
  if (output_buffer == NULL)
  {
	  printf("AI ERROR: output buffer NULL\r\n");
    fflush(stdout);
    ai_last_res = -20;
    ai_error_count++;
    return -1;
  }

  invalidate_dcache_region(output_buffer, AI_OUTPUT_SIZE * sizeof(int8_t));

  int8_t *out = (int8_t *)output_buffer;
  int8_t raw_output = out[0];

  ai_prob_siren = AI_OUTPUT_SCALE *
                  ((float)((int32_t)raw_output - (int32_t)AI_OUTPUT_ZERO_POINT));

  printf("AI INT8 RAW output=%d dequant_prob=%f\r\n",
         (int)raw_output,
         ai_prob_siren);

  if (ai_prob_siren >= SIREN_THRESHOLD)
  {
    ai_siren_detected = 1;
    ai_last_class = 1;
    ai_last_score = ai_prob_siren;

    printf("AI RESULT INT8 -> SIREN prob=%f threshold=%f\r\n",
           ai_prob_siren, SIREN_THRESHOLD);
  }
  else
  {
    ai_siren_detected = 0;
    ai_last_class = 0;
    ai_last_score = 1.0f - ai_prob_siren;

    printf("AI RESULT INT8 -> NON_SIREN prob_siren=%f threshold=%f\r\n",
           ai_prob_siren, SIREN_THRESHOLD);
  }

  fflush(stdout);

  ai_post_ok_count++;
  return 0;
}

/* USER CODE END 2 */

void MX_X_CUBE_AI_Init(void)
{
  set_clk_sleep_mode();

  __HAL_RCC_NPU_CLK_ENABLE();
  __HAL_RCC_NPU_FORCE_RESET();
  __HAL_RCC_NPU_RELEASE_RESET();

  npu_cache_init();

  /* USER CODE BEGIN 5 */
  printf("\r\nAI INIT - Neural-ART INT8 ResNet siren detector\r\n");
  printf("AI INIT DONE\r\n");
  fflush(stdout);
  /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
  /* USER CODE BEGIN 6 */

  int res = 0;
  LL_ATON_RT_RetValues_t ll_aton_rt_ret = LL_ATON_RT_DONE;

  ai_process_count++;

  const LL_Buffer_InfoTypeDef *ibuffersInfos =
      NN_Interface_Default.input_buffers_info();

  const LL_Buffer_InfoTypeDef *obuffersInfos =
      NN_Interface_Default.output_buffers_info();

  buffer_in = (uint8_t *)LL_Buffer_addr_start(&ibuffersInfos[0]);
  buffer_out = (uint8_t *)LL_Buffer_addr_start(&obuffersInfos[0]);

  if (buffer_in == NULL || buffer_out == NULL)
  {
    printf("AI ERROR: buffer NULL\r\n");
    fflush(stdout);
    ai_last_res = -30;
    ai_error_count++;
    ai_returned_to_main = 1;
    return;
  }

  printf("AI BUFFERS input=%p output=%p\r\n", buffer_in, buffer_out);
  fflush(stdout);

  uint32_t t0, t1;

  t0 = HAL_GetTick();
  LL_ATON_RT_RuntimeInit();
  t1 = HAL_GetTick();

  uint32_t dt_runtime_init = t1 - t0;

  t0 = HAL_GetTick();
  LL_ATON_RT_Init_Network(&NN_Instance_Default);
  t1 = HAL_GetTick();

  uint32_t dt_network_init = t1 - t0;

  t0 = HAL_GetTick();
  res = acquire_and_process_data(buffer_in);
  t1 = HAL_GetTick();

  uint32_t dt_acquire = t1 - t0;

  if (res != 0)
  {
    ai_last_res = res;
    ai_error_count++;
    ai_returned_to_main = 1;
    return;
  }

  int8_t *dbg_in_before = (int8_t *)buffer_in;

  printf("CHECK BEFORE RUN q[0]=%d q[1]=%d q[2]=%d q[100]=%d q[1000]=%d q[3199]=%d\r\n",
         (int)dbg_in_before[0],
         (int)dbg_in_before[1],
         (int)dbg_in_before[2],
         (int)dbg_in_before[100],
         (int)dbg_in_before[1000],
         (int)dbg_in_before[3199]);
  fflush(stdout);

  uint32_t loop_count = 0;

  t0 = HAL_GetTick();

  do
  {
    ll_aton_rt_ret = LL_ATON_RT_RunEpochBlock(&NN_Instance_Default);
    loop_count++;

    if (ll_aton_rt_ret == LL_ATON_RT_WFE)
    {
      LL_ATON_OSAL_WFE();
    }

  } while (ll_aton_rt_ret != LL_ATON_RT_DONE);

  t1 = HAL_GetTick();

  uint32_t dt_run = t1 - t0;

  invalidate_dcache_region(buffer_in, AI_INPUT_SIZE * sizeof(int8_t));

  int8_t *dbg_in_after = (int8_t *)buffer_in;

  printf("CHECK AFTER RUN q[0]=%d q[1]=%d q[2]=%d q[100]=%d q[1000]=%d q[3199]=%d\r\n",
         (int)dbg_in_after[0],
         (int)dbg_in_after[1],
         (int)dbg_in_after[2],
         (int)dbg_in_after[100],
         (int)dbg_in_after[1000],
         (int)dbg_in_after[3199]);
  fflush(stdout);

  t0 = HAL_GetTick();
  res = post_process(buffer_out);
  t1 = HAL_GetTick();

  uint32_t dt_post = t1 - t0;

  t0 = HAL_GetTick();
  LL_ATON_RT_DeInit_Network(&NN_Instance_Default);
  t1 = HAL_GetTick();

  uint32_t dt_network_deinit = t1 - t0;

  t0 = HAL_GetTick();
  LL_ATON_RT_RuntimeDeInit();
  t1 = HAL_GetTick();

  uint32_t dt_runtime_deinit = t1 - t0;

  printf("AI TIMING INT8 acquire=%lu runtime_init=%lu network_init=%lu run=%lu network_deinit=%lu runtime_deinit=%lu post=%lu loops=%lu ret=%d\r\n",
         (unsigned long)dt_acquire,
         (unsigned long)dt_runtime_init,
         (unsigned long)dt_network_init,
         (unsigned long)dt_run,
         (unsigned long)dt_network_deinit,
         (unsigned long)dt_runtime_deinit,
         (unsigned long)dt_post,
         (unsigned long)loop_count,
         (int)ll_aton_rt_ret);
  fflush(stdout);

  if (res != 0)
  {
    ai_last_res = res;
    ai_error_count++;
    ai_returned_to_main = 1;
    return;
  }

  ai_run_ok_count++;
  ai_last_res = 0;
  ai_returned_to_main = 1;

  /* USER CODE END 6 */
}

#ifdef __cplusplus
}
#endif
