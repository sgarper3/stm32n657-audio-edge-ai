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

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"
#include "network.h"
#include "network_data.h"

/* =========================
   EXTERNAL INPUT FROM MAIN
   ========================= */
extern float feature_buffer_ready[3200];

volatile float ai_input_0 = 0.0f;
volatile float ai_input_1 = 0.0f;
volatile float ai_input_2 = 0.0f;
volatile float ai_input_100 = 0.0f;
volatile float ai_input_1000 = 0.0f;
volatile float ai_input_3199 = 0.0f;

volatile float ai_input_min = 0.0f;
volatile float ai_input_max = 0.0f;
/* IO buffers --------------------------------------------------------------- */

#if !defined(AI_NETWORK_INPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_in_1[AI_NETWORK_IN_1_SIZE_BYTES];
ai_i8* data_ins[AI_NETWORK_IN_NUM] = {
  data_in_1
};
#else
ai_i8* data_ins[AI_NETWORK_IN_NUM] = { NULL };
#endif

#if !defined(AI_NETWORK_OUTPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_out_1[AI_NETWORK_OUT_1_SIZE_BYTES];
ai_i8* data_outs[AI_NETWORK_OUT_NUM] = {
  data_out_1
};
#else
ai_i8* data_outs[AI_NETWORK_OUT_NUM] = { NULL };
#endif

/* Activations -------------------------------------------------------------- */
ai_handle data_activations0[] = {
  (ai_handle) 0x34100000
};

/* AI objects --------------------------------------------------------------- */
static ai_handle network = AI_HANDLE_NULL;
static ai_buffer* ai_input = NULL;
static ai_buffer* ai_output = NULL;

/* ERROR HANDLER ------------------------------------------------------------ */
static void ai_log_err(const ai_error err, const char *fct)
{
  printf("AI ERROR (%s) type=0x%02x code=0x%02x\r\n",
         fct ? fct : "?", err.type, err.code);

  while (1);
}

/* =========================
   BOOTSTRAP
   ========================= */
static int ai_boostrap(ai_handle *act_addr)
{
  ai_error err;

  err = ai_network_create_and_init(&network, act_addr, NULL);
  if (err.type != AI_ERROR_NONE)
  {
    ai_log_err(err, "ai_network_create_and_init");
    return -1;
  }

  ai_input  = ai_network_inputs_get(network, NULL);
  ai_output = ai_network_outputs_get(network, NULL);

  if (ai_input == NULL || ai_output == NULL)
  {
    printf("AI input/output descriptors are NULL\r\n");
    return -1;
  }

#if !defined(AI_NETWORK_INPUTS_IN_ACTIVATIONS)
  for (int i = 0; i < AI_NETWORK_IN_NUM; i++)
  {
    ai_input[i].data = AI_HANDLE_PTR(data_ins[i]);
  }
#endif

#if !defined(AI_NETWORK_OUTPUTS_IN_ACTIVATIONS)
  for (int i = 0; i < AI_NETWORK_OUT_NUM; i++)
  {
    ai_output[i].data = AI_HANDLE_PTR(data_outs[i]);
  }
#endif

  printf("AI input data ptr  = %p\r\n", ai_input[0].data);
  printf("AI output data ptr = %p\r\n", ai_output[0].data);

  if (ai_input[0].data == AI_HANDLE_NULL || ai_output[0].data == AI_HANDLE_NULL)
  {
    printf("AI input/output data buffer is NULL\r\n");
    return -1;
  }

  return 0;
}

/* =========================
   RUN NETWORK
   ========================= */
static int ai_run(void)
{
  ai_i32 batch = ai_network_run(network, ai_input, ai_output);

  if (batch != 1)
  {
    ai_log_err(ai_network_get_error(network), "ai_network_run");
    return -1;
  }

  return 0;
}

/* =========================
   INPUT FROM MAIN.C
   ========================= */
int acquire_and_process_data(ai_i8* data[])
{
  (void)data;

  float *in = (float*)ai_input[0].data;

  for (int i = 0; i < AI_NETWORK_IN_1_SIZE; i++)
  {
    in[i] = feature_buffer_ready[i];
  }

  ai_input_0    = in[0];
  ai_input_1    = in[1];
  ai_input_2    = in[2];
  ai_input_100  = in[100];
  ai_input_1000 = in[1000];
  ai_input_3199 = in[3199];

  ai_input_min = in[0];
  ai_input_max = in[0];

  for (int i = 1; i < AI_NETWORK_IN_1_SIZE; i++)
  {
    if (in[i] < ai_input_min) ai_input_min = in[i];
    if (in[i] > ai_input_max) ai_input_max = in[i];
  }

  return 0;
}

/* =========================
   POST PROCESS
   ========================= */
int post_process(ai_i8* data[])
{
  (void)data;

  float *out = (float*)ai_output[0].data;

  if (out == NULL)
  {
    printf("OUT NULL\r\n");
    return -1;
  }

  int max_idx = 0;
  float max_val = out[0];

  for (int i = 1; i < AI_NETWORK_OUT_1_SIZE; i++)
  {
    if (out[i] > max_val)
    {
      max_val = out[i];
      max_idx = i;
    }
  }

  printf("AI RESULT -> class=%d score=%f\r\n", max_idx, max_val);

  return 0;
}

/* =========================
   INIT
   ========================= */
void MX_X_CUBE_AI_Init(void)
{
  printf("\r\nAI INIT\r\n");

  if (ai_boostrap(data_activations0) != 0)
  {
    printf("AI INIT FAILED\r\n");
    while (1);
  }

  printf("AI INIT DONE\r\n");
}

/* =========================
   PROCESS
   ========================= */


void MX_X_CUBE_AI_Process(void)
{
  int res;

  printf("AI PROCESS START\r\n");

  res = acquire_and_process_data(data_ins);
  printf("ACQUIRE DONE res=%d\r\n", res);

  if (res != 0)
    return;

  printf("AI RUN START\r\n");

  res = ai_run();

  printf("AI RUN DONE res=%d\r\n", res);

  if (res != 0)
    return;

  printf("POST START\r\n");

  res = post_process(data_outs);

  printf("POST DONE res=%d\r\n", res);
}

#ifdef __cplusplus
}
#endif
