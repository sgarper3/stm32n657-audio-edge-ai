# Embedded Audio AI on STM32N657

Embedded audio-AI development on the STM32N657 platform, from audio acquisition and preprocessing to neural-network deployment and on-device siren detection.

This repository documents the implementation developed for a Master's Thesis in Telecommunication Engineering. The work follows an incremental validation strategy: each stage is verified independently before being integrated into the final embedded pipeline.

## Project overview

The project explores the complete workflow required to deploy an audio-classification neural network on an STM32N657 microcontroller:

1. audio acquisition through digital microphones;
2. PCM transport and host-side verification;
3. embedded log-Mel feature extraction;
4. deployment of a pretrained MiniResNetV1 audio classifier;
5. transfer learning for binary siren detection;
6. post-training INT8 quantization;
7. deployment using ST Edge AI / Neural-ART;
8. numerical comparison between host and STM32 inference;
9. operating-threshold analysis;
10. robustness evaluation under additive white Gaussian noise (AWGN);
11. integration of the final dual-microphone prototype.

A central objective of the project is not only to obtain a working detector, but also to verify the equivalence of the processing and inference stages across Colab/TensorFlow, TFLite and STM32.

## Repository structure

```text
stm32n657-audio-edge-ai/
├── firmware/
│   ├── 01_audio_capture_uart/
│   ├── 02_base_audio_classifier/
│   ├── 03_siren_detector_single_mic/
│   └── 04_siren_detector_dual_mic/
│
├── notebooks/
│   ├── 01_base_model_evaluation.ipynb
│   ├── 02_siren_transfer_learning.ipynb
│   ├── 03_siren_post_training_evaluation.ipynb
│
├── models/
│   ├── miniresnetv1_s1_64x50_siren_final.keras
│   ├── miniresnetv1_s1_64x50_siren_final_float32.tflite
│   ├── miniresnetv1_s1_64x50_siren_final_int8.tflite
│   ├── miniresnetv1_s1_64x50_siren_final_int8_quantization.json
│   ├── model_release_info.json
│   ├── fold_models/
│   └── tflite_int8_models/
│
├── results/
│   ├── training_history/
│   ├── quantization_10fold/
│   └── snr_robustness/
│
├── validation/
│   ├── 01_audio_capture_uart/
│   └── inputs/
│
└── tools/
```

Each major directory contains its own README with more detailed information:

- [`firmware/`](firmware/) — embedded implementation and incremental firmware stages;
- [`notebooks/`](notebooks/) — model evaluation, transfer learning and post-training analysis;
- [`models/`](models/) — final deployment models and fold-specific validation models;
- [`results/`](results/) — compact experimental results and figures;
- [`validation/`](validation/) — common inputs and artifacts used for implementation-level verification.

## Development stages

### 1. Audio acquisition

The first firmware stage validates microphone acquisition independently from the neural network.

Audio is acquired on the STM32 through I2S/DMA, exported as PCM through UART and reconstructed on the host side. This provides a known-good reference before introducing embedded preprocessing or inference.

### 2. Base audio classifier

A pretrained MiniResNetV1 model from the STMicroelectronics STM32AI Model Zoo is evaluated and deployed as the initial neural-network baseline.

The model is evaluated on the ESC-10 subset of ESC-50 and used to establish the first Colab-to-STM32 inference comparison.

### 3. Siren detector

The MiniResNetV1 architecture is adapted through transfer learning for binary classification:

```text
non_siren / siren
```

UrbanSound8K is used for the siren-detection experiments.

The training workflow uses the ten official UrbanSound8K folds. In each iteration, one fold is excluded from weight updates and used as validation data for callbacks and checkpoint selection. The resulting metrics are therefore interpreted as cross-validation results rather than as measurements on a completely independent holdout test set.

A final model is subsequently trained using all available folds for deployment.

### 4. INT8 deployment

The final siren model is converted to a fully quantized INT8 TFLite model for deployment on the STM32N657 using the ST Edge AI / Neural-ART toolchain.

A separate fold-wise quantization experiment evaluates the numerical and classification impact of post-training quantization. For each fold model, the representative PTQ dataset is selected exclusively from its other nine training folds.

### 5. Final embedded prototype

The final firmware integrates:

```text
digital microphones
        ↓
I2S / DMA acquisition
        ↓
embedded log-Mel preprocessing
        ↓
INT8 MiniResNet siren detector
        ↓
Neural-ART accelerated inference
        ↓
siren / non-siren decision
```

The final implementation also includes the dual-microphone acquisition stage used in the completed prototype.

## Validation strategy

The project uses incremental validation at several levels.

### Audio pipeline

The raw acquisition stage is validated before neural-network integration by exporting PCM audio through UART and reconstructing it on the host.

### Float32 inference equivalence

A common precomputed log-Mel input is evaluated in both the host environment and STM32. This isolates neural-network inference from acquisition and preprocessing differences.

### Model-format equivalence

The final Keras model is exported to deployment-compatible formats and checked using the same reference input.

Comparisons include:

```text
Keras
H5
TFLite float32
STM32
```

### INT8 equivalence

Quantized reference inputs are used to compare the TFLite INT8 implementation with the STM32 / Neural-ART deployment.

### Robustness

The fold-specific INT8 models are also evaluated under controlled additive white Gaussian noise at:

```text
clean
20 dB
10 dB
5 dB
0 dB
```

Noise is injected in the time-domain audio before log-Mel feature extraction.

Detailed outputs are available under [`results/`](results/).

## Notebooks

The recommended order is:

```text
01_base_model_evaluation.ipynb
        ↓
02_siren_transfer_learning.ipynb
        ↓
03_siren_post_training_evaluation.ipynb
```

The notebooks contain the complete machine-learning workflow, while compact generated results are retained separately under `results/`.

See [`notebooks/README.md`](notebooks/README.md) for execution details, external resources and methodological notes.

## External datasets and pretrained model

Large third-party datasets and the original pretrained ST model are not redistributed in this repository.

### MiniResNetV1 pretrained model

STMicroelectronics STM32AI Model Zoo:

https://github.com/STMicroelectronics/stm32ai-modelzoo/tree/main/audio_event_detection/miniresnetv1/ST_pretrainedmodel_public_dataset/esc10/miniresnetv1_s1_64x50_tl

### ESC-50 / ESC-10

Official ESC-50 repository:

https://github.com/karolpiczak/esc-50

The ESC-10 subset and the corresponding metadata are derived from this dataset.

### UrbanSound8K

Official download page:

https://urbansounddataset.weebly.com/download-urbansound8k.html

## Reproducibility

The repository intentionally includes:

- trained deployment models;
- the ten fold-specific float32 models;
- the corresponding fold-specific INT8 models;
- compact cross-validation metrics;
- training histories;
- quantization summaries;
- threshold-analysis outputs;
- SNR robustness summaries and figures;
- common Colab/STM32 validation inputs.

Large datasets, feature caches, per-sample prediction tables and temporary Colab artifacts are intentionally excluded.

This keeps the repository compact while preserving the artifacts needed to inspect or reproduce the main validation steps without repeating every expensive computation.

## Academic context

This repository contains the experimental implementation associated with a Master's Thesis focused on embedded artificial intelligence for audio-event detection on the STM32N657 platform.

The emphasis of the work is on end-to-end embedded implementation and traceable validation across the complete processing chain rather than on neural-network training alone.
