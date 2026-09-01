# Notebooks

This directory contains the Colab notebooks used for the machine-learning workflow of the project.

The notebooks are intentionally separated by stage so that the original pretrained model evaluation, the siren-detector training process, and the post-training deployment analysis can be inspected independently.

## Contents

```text
notebooks/
├── 01_base_model_evaluation.ipynb
├── 02_siren_transfer_learning.ipynb
├── 03_siren_post_training_evaluation.ipynb
```

## Recommended order

### 1. `01_base_model_evaluation.ipynb`

Evaluates the pretrained MiniResNetV1 audio-event-detection model distributed by STMicroelectronics.

The notebook:

- loads the pretrained `miniresnetv1_s1_64x50_tl` model;
- reproduces the expected 64 × 50 log-Mel input representation;
- evaluates the model on the ESC-10 subset of ESC-50;
- reports classification metrics and confusion matrices;
- compares a common float32 input between Colab and STM32.

This notebook performs **evaluation only**. It does not retrain the original ST model.

External resources:

- STMicroelectronics pretrained model:  
  https://github.com/STMicroelectronics/stm32ai-modelzoo/tree/main/audio_event_detection/miniresnetv1/ST_pretrainedmodel_public_dataset/esc10/miniresnetv1_s1_64x50_tl
- ESC-50 dataset:  
  https://github.com/karolpiczak/esc-50

The ESC-50 repository includes both the audio files and the metadata file `meta/esc50.csv`.

---

### 2. `02_siren_transfer_learning.ipynb`

Retrains the MiniResNetV1 backbone for binary siren detection using UrbanSound8K.

The notebook contains the complete transfer-learning workflow, including:

- dataset preparation and binary class mapping;
- log-Mel feature extraction;
- persistent feature caching;
- 10-fold training;
- checkpointing and persistent training state;
- storage of fold-specific models;
- final training using all available folds;
- export of the final trained model and reference inputs for STM32 validation.

The 10-fold procedure is used as a cross-validation scheme. In each iteration, one UrbanSound8K fold is excluded from weight updates and used as validation data for callbacks and checkpoint selection. Therefore, the excluded fold must **not** be interpreted as a fully independent test set.

UrbanSound8K can be obtained from:

https://urbansounddataset.weebly.com/download-urbansound8k.html

The dataset itself is not included in this repository.

---

### 3. `03_siren_post_training_evaluation.ipynb`

Performs the post-training analysis and deployment validation of the siren detector.

Main tasks include:

- fold-by-fold evaluation of the trained float32 models;
- diagnostic evaluation of the final all-data model;
- inspection of false positives and false negatives;
- export to H5 and TFLite float32;
- fully-int8 post-training quantization for STM32 / Neural-ART;
- comparison of Colab and STM32 outputs using common exported inputs;
- fold-wise float32 vs INT8 evaluation;
- analysis of the operating threshold;
- robustness evaluation with additive white Gaussian noise (AWGN).

For the fold-wise INT8 analysis, each fold model is quantized using representative samples drawn exclusively from its other nine training folds. The excluded fold is then used for the corresponding evaluation.

As in the training notebook, the excluded fold was used for validation callbacks and checkpoint selection during the original float32 training. The reported fold-based results are therefore cross-validation results rather than measurements on a completely independent holdout test set.

The final model trained with all folds is used for deployment. Its evaluation on the full cached dataset is retained only as an internal diagnostic and is not used as an estimate of generalization.

---


## Reproducibility notes

The repository intentionally does not include the ESC-50 or UrbanSound8K audio datasets.

The pretrained ST model used in the first stage is also referenced from its official source rather than redistributed here.

Generated sample-level prediction tables and large intermediate caches are not included because they can be regenerated from the notebooks and published model artifacts.

Compact training histories, summary metrics, quantization results, threshold-analysis outputs, and robustness results are available under `results/`.
