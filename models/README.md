# Models

This directory contains the trained and deployment-ready model artifacts used in the siren-detection stage of the project.

The repository separates the **final model used for deployment** from the **fold-specific models used for experimental validation**.

## Structure

```text
models/
├── README.md
├── miniresnetv1_s1_64x50_siren_final.keras
├── miniresnetv1_s1_64x50_siren_final_float32.tflite
├── miniresnetv1_s1_64x50_siren_final_int8.tflite
├── miniresnetv1_s1_64x50_siren_final_int8_quantization.json
├── model_release_info.json
├── fold_models/
│   ├── miniresnet_siren_fold1.keras
│   ├── miniresnet_siren_fold2.keras
│   ├── ...
│   └── miniresnet_siren_fold10.keras
└── tflite_int8_models/
    ├── miniresnet_siren_fold1_fully_int8.tflite
    ├── miniresnet_siren_fold2_fully_int8.tflite
    ├── ...
    └── miniresnet_siren_fold10_fully_int8.tflite
```

## Final trained model

### `miniresnetv1_s1_64x50_siren_final.keras`

Final Keras model trained after the fold-based validation stage.

This model is trained using all available UrbanSound8K folds and is the reference model from which the deployment formats are generated.

Because all folds are used during this final training step, evaluation of this model on the same cached training data is treated only as an internal diagnostic and not as an independent estimate of generalization.

## Deployment models

### `miniresnetv1_s1_64x50_siren_final_float32.tflite`

Float32 TFLite export of the final Keras model.

It is used primarily for deployment-format validation and for numerical comparison with the STM32 implementation using common reference inputs.

### `miniresnetv1_s1_64x50_siren_final_int8.tflite`

Fully quantized INT8 TFLite model intended for deployment on the STM32N657 using the ST Edge AI / Neural-ART toolchain.

This is the main deployment artifact for the siren detector.

### `miniresnetv1_s1_64x50_siren_final_int8_quantization.json`

Stores the quantization parameters associated with the final INT8 model, including input/output scale and zero-point information.

These values are used to verify consistency between the TFLite model and the embedded implementation.

## Model metadata

### `model_release_info.json`

Compact metadata describing the released siren detector, including:

- model task;
- class labels;
- input and output shapes;
- input representation;
- source architecture;
- dataset;
- validation protocol;
- recommended operating threshold.

## Fold-specific float32 models

### `fold_models/`

Contains the ten Keras models produced during the UrbanSound8K fold-based validation procedure.

For fold `k`:

```text
miniresnet_siren_foldk.keras
```

the corresponding model was trained with weight updates using the other nine folds, while fold `k` was used as validation data for callbacks and checkpoint selection.

These models are retained so that the fold-by-fold evaluation can be reproduced without retraining.

They are also used as the float32 source models for the fold-wise INT8 quantization study.

## Fold-specific INT8 models

### `tflite_int8_models/`

Contains fully-int8 TFLite versions of the ten fold-specific Keras models.

For each fold:

1. the corresponding float32 fold model is loaded;
2. PTQ calibration uses representative samples selected exclusively from the other nine folds;
3. the resulting INT8 model is evaluated on the excluded fold.

These models support the experiments reported under:

```text
results/quantization_10fold/
results/snr_robustness/
```

They are **experimental validation artifacts**, not the model deployed as the final application model.

## Base pretrained model

The original MiniResNetV1 model used as the starting point for transfer learning is not redistributed in this repository.

It is obtained from the official STMicroelectronics STM32AI Model Zoo:

https://github.com/STMicroelectronics/stm32ai-modelzoo/tree/main/audio_event_detection/miniresnetv1/ST_pretrainedmodel_public_dataset/esc10/miniresnetv1_s1_64x50_tl

The corresponding source file used during development was:

```text
miniresnetv1_s1_64x50_tl.keras
```

## Related resources

The training and post-training workflows are documented in:

```text
notebooks/02_siren_transfer_learning.ipynb
notebooks/03_siren_post_training_evaluation.ipynb
```

Validation inputs and export-equivalence artifacts are available under:

```text
validation/
```

Compact experimental results are available under:

```text
results/
```
