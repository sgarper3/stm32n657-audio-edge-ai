# Results

This directory contains the compact experimental outputs retained for reproducibility and inspection.

Large sample-level prediction tables and temporary diagnostic files are intentionally excluded. The corresponding values can be regenerated from the notebooks and published models.

## Structure

```text
results/
├── training_history/
├── kfold_results.csv
├── kfold_summary_stats.csv
├── quantization_10fold/
│   ├── tfm_exports/
│   ├── threshold_analysis/
│   ├── quantization_10fold_metrics_by_fold.csv
│   ├── quantization_10fold_oof_aggregate_metrics.csv
│   ├── quantization_10fold_parameters.csv
│   ├── quantization_10fold_summary_mean_std.csv
│   └── quantization_probability_error_summary.json
└── snr_robustness/
    └── full_oof/
```

## Training history

`training_history/` contains the saved Keras training histories for:

- folds 1–10;
- the final model trained using all folds.

These CSV files allow the training and validation curves to be regenerated without repeating the full training procedure.

## Cross-validation results

`kfold_results.csv` contains the metrics obtained for each UrbanSound8K fold.

`kfold_summary_stats.csv` contains the corresponding compact summary statistics.

The fold-based evaluation follows the training protocol used in the project: the excluded fold does not participate in weight updates, but it is used as validation data for callbacks and checkpoint selection. These results must therefore be interpreted as **cross-validation results**, not as results from a completely independent test set.

## Quantization results

`quantization_10fold/` contains the evaluation of post-training fully-int8 quantization.

For each fold:

1. the float32 fold model is loaded;
2. the representative dataset for PTQ is selected only from the other nine folds;
3. a fully-int8 TFLite model is generated;
4. float32 and INT8 outputs are compared on the excluded fold.

Relevant files include:

- `quantization_10fold_metrics_by_fold.csv`: metrics for float32 and INT8 by fold;
- `quantization_10fold_oof_aggregate_metrics.csv`: aggregate fold-based metrics;
- `quantization_10fold_parameters.csv`: quantization parameters and representative-set information;
- `quantization_10fold_summary_mean_std.csv`: mean and standard deviation across folds;
- `quantization_probability_error_summary.json`: numerical summary of the float32–INT8 probability differences.

### `tfm_exports/`

Contains compact figures and tables used to summarize the effect of quantization, including:

- float32 confusion matrix;
- INT8 confusion matrix;
- probability-error histogram;
- float32 vs INT8 mean/std comparison table.

### `threshold_analysis/`

Contains the operating-threshold study performed on the fold-wise INT8 predictions.

It includes:

- threshold sweep results;
- selected operating points;
- summary JSON;
- ROC curve;
- precision-recall curve;
- precision/recall/F1 vs threshold figure.

The deployed threshold is evaluated as an operating point of the INT8 detector; it is not inferred from the diagnostic evaluation of the final all-data model.

## SNR robustness

`snr_robustness/full_oof/` contains the complete robustness experiment with additive white Gaussian noise (AWGN).

Noise is injected in the time-domain audio before log-Mel feature extraction.

The evaluated conditions are:

- clean;
- 20 dB SNR;
- 10 dB SNR;
- 5 dB SNR;
- 0 dB SNR.

The retained files provide:

- aggregate metrics;
- metrics by fold;
- mean and standard deviation across folds;
- degradation relative to the clean condition;
- false-positive / false-negative figures;
- the main robustness figure;
- a compact methodology description;
- a clean-condition consistency check against the previous INT8 fold-based evaluation.

The large per-sample SNR prediction tables and the preliminary `quick_check` outputs are not included.

## Terminology note

Some generated filenames retain the term `oof` because that naming was used during development.

Operationally, these files contain predictions obtained from the model associated with the corresponding excluded fold. However, because that fold was also used for validation callbacks and checkpoint selection during training, the results should not be interpreted as coming from a completely independent holdout test set.
