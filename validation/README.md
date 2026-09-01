# Validation artifacts

This directory contains the common inputs and compact reference files used to validate numerical equivalence between the Colab/TensorFlow pipeline and the STM32 implementation.

These files are **not an evaluation dataset**. Their purpose is implementation-level verification.

## Suggested structure

```text
validation/
├── README.md
├── 01_audio_capture_uart/
├── inputs/
│   ├── test_input.h
│   ├── test_input.npy
│   ├── exported_sample_metadata.json
│   ├── multi_test_inputs.h
│   └── multi_test_inputs_int8.h
└── x_cube_ai_export_check.json
```

## Audio-capture validation

### `01_audio_capture_uart/`

Contains the artifacts used to validate the initial STM32 audio-capture stage, where PCM audio acquired through I2S/DMA is transmitted over UART and reconstructed on the host side.

This stage is kept separate from the neural-network equivalence tests because it validates the acquisition and transport pipeline before feature extraction and inference are introduced.

## Single reference input

### `inputs/test_input.npy`

Reference float32 log-Mel tensor used in Colab/TensorFlow/TFLite.

### `inputs/test_input.h`

C representation of the same input for STM32-side validation.

Using the same precomputed feature tensor on both platforms isolates the neural-network inference stage from the live audio-capture and preprocessing stages.

### `inputs/exported_sample_metadata.json`

Metadata describing the exported reference sample.

## Multiple reference inputs

### `inputs/multi_test_inputs.h`

Contains multiple float32 reference inputs together with expected reference outputs.

It is used for broader numerical comparison between the host-side model and the STM32 implementation.

### `inputs/multi_test_inputs_int8.h`

Quantized INT8 version of the multiple-reference-input set.

It includes the INT8 tensors and the corresponding expected outputs used for validating the deployed quantized model.

## Export verification

### `x_cube_ai_export_check.json`

Stores the host-side export comparison performed when converting the final Keras model to deployment-compatible formats.

The post-training notebook verifies the same reference input across:

- the original Keras model;
- the legacy H5 export;
- the TFLite float32 export.

The STM32 comparison then uses the same exported input to check agreement with the embedded implementation.

## Validation scope

The project uses validation at several levels:

1. **Common-input float32 validation**  
   The same log-Mel tensor is evaluated in Colab and STM32.

2. **Model-format equivalence**  
   Keras, H5, and TFLite float32 outputs are compared using the same input.

3. **INT8 deployment validation**  
   Quantized reference inputs and expected outputs are used to compare the TFLite INT8 model with the STM32 / Neural-ART implementation.

These checks are intended to demonstrate implementation equivalence. They are separate from the model-performance evaluation reported under `results/`.
