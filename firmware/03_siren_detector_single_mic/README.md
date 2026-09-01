# INT8 Siren Detector — STM32N657

This project represents the first complete siren-specific embedded implementation developed for the Master's Thesis.

The firmware deploys an INT8 siren-detection model on the STM32N657 using Neural-ART acceleration. It supports both real-time microphone input and predefined INT8 test vectors, allowing the same embedded inference pipeline to be used for functional operation and controlled validation.

This stage precedes the final dual-microphone implementation.

## 1. Features

The firmware includes:

- single digital microphone acquisition through I2S;
- DMA-based audio buffering;
- 16 kHz audio processing;
- embedded log-Mel feature extraction;
- CMSIS-DSP processing;
- INT8 input quantization;
- siren-specific neural-network inference;
- Neural-ART/NPU acceleration;
- INT8 output dequantization;
- configurable siren-detection threshold;
- UART diagnostics and timing information;
- predefined INT8 test-vector input for validation.

The deployed model uses:

- input size: 3200 INT8 values;
- input quantization scale: `0.3137255013`;
- input zero point: `127`;
- output quantization scale: `0.00390625`;
- output zero point: `-128`;
- siren decision threshold: `0.80`.

## 2. Input modes

The firmware supports two input sources selected at compile time in `main.c`:

```c
#define AI_INPUT_SOURCE_HEADER  1
#define AI_INPUT_SOURCE_MIC     2

#define AI_INPUT_SOURCE AI_INPUT_SOURCE_MIC
```

### Microphone mode

Use:

```c
#define AI_INPUT_SOURCE AI_INPUT_SOURCE_MIC
```

The firmware acquires a one-second audio window from the microphone, computes the log-Mel representation, quantizes the features to INT8 and sends them to the Neural-ART inference pipeline.

### Header validation mode

Use:

```c
#define AI_INPUT_SOURCE AI_INPUT_SOURCE_HEADER
```

The firmware loads predefined INT8 tensors from:

`multi_test_inputs_int8.h`

These inputs include reference values generated during model validation and allow the STM32 inference output to be compared against the corresponding Keras/TFLite results using exactly the same quantized network input.

This mode isolates the embedded inference stage from microphone acquisition and acoustic preprocessing.

## 3. Project structure

The complete STM32CubeIDE project snapshot used during development is preserved.

Main components:

- `Appli/` — main STM32 application.
- `FSBL/` — First Stage Boot Loader.
- `Drivers/` — STM32, CMSIS and CMSIS-DSP dependencies.
- `Middlewares/` — STM32 AI and Neural-ART support.
- `Secure_nsclib/` — secure support files required by the project.
- `prebuilt/` — validated binaries ready for direct programming.
- `.ioc` — original STM32CubeMX configuration.

The `.project`, `.cproject` and `.settings` files are intentionally preserved because they contain the STM32CubeIDE configuration used for the validated build.

The directory structure should be kept unchanged because the STM32CubeIDE projects use relative paths to shared resources.

## 4. Importing the project

The recommended approach is to import the existing projects directly into STM32CubeIDE.

Regenerating the project from the `.ioc` file is **not required** for normal use.

1. Clone or download the repository.
2. Keep the complete directory structure unchanged.
3. Open STM32CubeIDE.
4. Select `File > Import > General > Existing Projects into Workspace`.
5. Import the project contained in `Appli/`.
6. Import the project contained in `FSBL/`.

The `.ioc` file is retained mainly for reference and inspection of the original STM32CubeMX configuration.

## 5. CMSIS-DSP and preprocessing

CMSIS-DSP is used by the embedded preprocessing implementation, particularly for the FFT operations required to generate the log-Mel representation.

The required CMSIS-DSP files are included under:

`Drivers/CMSIS/DSP/`

The project preserves the Cortex-M55 DSP build configuration used during development.

The preprocessing pipeline is:

`PCM audio → centered framing → Hann window → FFT → power spectrum → Mel filtering → dB conversion → INT8 quantization`

The sparse Mel-filter representation is stored in:

`mel_filters_sparse.h`

## 6. Main custom files

The main custom or manually modified files for this stage include:

- `Appli/Core/Src/main.c`
- `Appli/Core/Src/audio_features.c`
- `Appli/Core/Inc/audio_features.h`
- `Appli/Core/Inc/mel_filters_sparse.h`
- `Appli/Core/Inc/multi_test_inputs_int8.h`
- `Appli/X-CUBE-AI/App/app_x-cube-ai.c`

`main.c` implements microphone acquisition, preprocessing, quantization and input-source selection.

`audio_features.c/.h` implement the embedded Mel-feature extraction.

`mel_filters_sparse.h` contains the sparse Mel-filter representation generated for the embedded preprocessing pipeline.

`multi_test_inputs_int8.h` contains the predefined quantized validation inputs and their associated reference results.

`app_x-cube-ai.c` implements the Neural-ART inference flow, cache handling, output dequantization, classification and timing diagnostics.

## 7. Building

The configuration used and validated during development was:

**Debug**

Both projects must be built:

1. `FSBL`
2. `Appli`

After importing the projects:

1. select the desired `AI_INPUT_SOURCE` in `main.c`;
2. select the `Debug` configuration;
3. build `FSBL`;
4. build `Appli`.

The generated `Debug/` directories are not stored in the repository because they contain regenerable build artifacts.

STM32CubeIDE recreates them automatically when the projects are built.

## 8. Generating trusted images

On STM32N6, the FSBL and application binaries generated by STM32CubeIDE must be converted into **trusted images** before programming.

The validated tool version is:

**STM32CubeProgrammer v2.21.0**

The signing utility is:

`STM32_SigningTool_CLI.exe`

It is recommended to add the directory containing this executable to the Windows system `PATH`.

With STM32CubeProgrammer v2.21 or later, include the `-align` option.

### FSBL

Open a command prompt inside:

`FSBL/Debug/`

Use the FSBL `.bin` generated by STM32CubeIDE without renaming it before signing.

The command format is:

```cmd
STM32_SigningTool_CLI.exe -bin <generated_FSBL.bin> -nk -of 0x80000000 -t fsbl -o <FSBL-trusted.bin> -hv 2.3 -dump <FSBL-trusted.bin> -align
```

### Application

Open a command prompt inside:

`Appli/Debug/`

Use the application `.bin` generated by STM32CubeIDE for the selected input mode.

The command format is:

```cmd
STM32_SigningTool_CLI.exe -bin <generated_Appli.bin> -nk -of 0x80000000 -t fsbl -o <Appli-trusted.bin> -hv 2.3 -dump <Appli-trusted.bin> -align
```

Changing `AI_INPUT_SOURCE` changes the compiled application, so the application must be rebuilt and signed again when switching between header-validation mode and microphone mode.

The value `0x80000000` used by the signing tool is **not** the external-flash programming address.

## 9. Neural-network binary

Neural-ART generates the network data file as:

`network_atonbuf.xSPI2.raw`

For programming, a copy of this file is stored with a `.bin` extension as:

`network_atonbuf.xSPI2.bin`

Only the filename/extension is changed; the binary contents remain unchanged.

This file must correspond to the same deployed network used by the application.

## 10. Programming the board

The following flash addresses are used:

| Component | Flash address |
|---|---:|
| Trusted FSBL | `0x70000000` |
| Trusted application | `0x70100000` |
| `network_atonbuf.xSPI2.bin` | `0x71000000` |

Program the three files using STM32CubeProgrammer v2.21.0.

After programming:

1. disconnect the programming session;
2. configure the board for the boot mode used to execute the application from external flash;
3. reset or power-cycle the board.

> **TODO:** document the exact jumper/boot configuration used on the development board.

## 11. Prebuilt binaries

The `prebuilt/` directory contains validated binaries for both supported application modes:

- `Resnet_siren_single_mic_FSBL-trusted.bin`
- `Resnet_siren_Header_Appli-trusted.bin`
- `Resnet_siren_Microphone_Appli-trusted.bin`
- `network_atonbuf.xSPI2.bin`

The FSBL and network binary are common to both modes.

### Header validation mode

Program:

- `Resnet_siren_single_mic_FSBL-trusted.bin` at `0x70000000`
- `Resnet_siren_Header_Appli-trusted.bin` at `0x70100000`
- `network_atonbuf.xSPI2.bin` at `0x71000000`

This configuration executes the predefined INT8 validation input.

### Microphone mode

Program:

- `Resnet_siren_single_mic_FSBL-trusted.bin` at `0x70000000`
- `Resnet_siren_Microphone_Appli-trusted.bin` at `0x70100000`
- `network_atonbuf.xSPI2.bin` at `0x71000000`

This configuration executes the real-time single-microphone siren detector.

The prebuilt files allow either validated configuration to be reproduced without rebuilding or signing the source project.

## 12. Reproduction workflows

Two approaches are available.

**Build from source**

`Import → Select input mode → Build → Sign → Program → Boot`

**Use prebuilt binaries**

For validation:

`FSBL + Header application + network binary → Program → Boot`

For real audio:

`FSBL + Microphone application + network binary → Program → Boot`

The source-build workflow is recommended when modifying the firmware.

The prebuilt binaries are intended to reproduce the validated configurations directly.