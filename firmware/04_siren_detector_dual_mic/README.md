# Dual-Microphone INT8 Siren Detector — STM32N657

This project represents the final embedded firmware implementation developed for the Master's Thesis.

It extends the previous single-microphone siren detector by acquiring two I2S microphone channels and combining them before embedded preprocessing and Neural-ART inference.

The validated configuration uses both microphones in average mode.

## 1. Features

The firmware includes:

- dual digital microphone acquisition through I2S;
- DMA-based audio buffering;
- configurable microphone selection;
- average combination of left and right microphone channels;
- 16 kHz audio processing;
- embedded log-Mel feature extraction;
- CMSIS-DSP processing;
- INT8 input quantization;
- siren-specific neural-network inference;
- Neural-ART/NPU acceleration;
- INT8 output dequantization;
- configurable siren-detection threshold;
- UART diagnostics and timing information.

The deployed model uses:

- input size: 3200 INT8 values;
- input quantization scale: `0.3137255013`;
- input zero point: `127`;
- output quantization scale: `0.00390625`;
- output zero point: `-128`;
- siren decision threshold: `0.80`.

## 2. Microphone configuration

The firmware supports three microphone modes:

```c
#define MIC_MODE_LEFT     1
#define MIC_MODE_RIGHT    2
#define MIC_MODE_AVERAGE  3
```

The validated final configuration is:

```c
#define MIC_MODE MIC_MODE_AVERAGE
```

In this mode, each audio sample is computed as the average of the left and right I2S channels before being added to the one-second processing window.

The individual channels remain available for debugging through the `last_left_sample` and `last_right_sample` diagnostic variables and UART output.

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

The processing pipeline is:

`Dual I2S audio → channel selection/average → centered framing → Hann window → FFT → power spectrum → Mel filtering → dB conversion → INT8 quantization → Neural-ART inference`

The sparse Mel-filter representation is stored in:

`mel_filters_sparse.h`

## 6. Main custom files

The main custom or manually modified files for this stage include:

- `Appli/Core/Src/main.c`
- `Appli/Core/Src/audio_features.c`
- `Appli/Core/Inc/audio_features.h`
- `Appli/Core/Inc/mel_filters_sparse.h`
- `Appli/X-CUBE-AI/App/app_x-cube-ai.c`

`main.c` implements dual-channel I2S acquisition, microphone combination, preprocessing and INT8 quantization.

`audio_features.c/.h` implement the embedded Mel-feature extraction.

`mel_filters_sparse.h` contains the sparse Mel-filter representation generated for the embedded preprocessing pipeline.

`app_x-cube-ai.c` implements the Neural-ART inference flow, cache handling, output dequantization, classification and timing diagnostics.

## 7. Building

The configuration used and validated during development was:

**Debug**

Both projects must be built:

1. `FSBL`
2. `Appli`

After importing the projects:

1. select the `Debug` configuration;
2. verify that `MIC_MODE` is set to `MIC_MODE_AVERAGE` for the validated final configuration;
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

The generated FSBL binary is:

`Resnet_siren_9_FSBL.bin`

Open a command prompt inside `FSBL/Debug/` and run:

```cmd
STM32_SigningTool_CLI.exe -bin Resnet_siren_9_FSBL.bin -nk -of 0x80000000 -t fsbl -o Resnet_siren_9_FSBL-trusted.bin -hv 2.3 -dump Resnet_siren_9_FSBL-trusted.bin -align
```

### Application

The generated application binary is:

`Resnet_siren_9_Appli.bin`

Open a command prompt inside `Appli/Debug/` and run:

```cmd
STM32_SigningTool_CLI.exe -bin Resnet_siren_9_Appli.bin -nk -of 0x80000000 -t fsbl -o Resnet_siren_9_Appli-trusted.bin -hv 2.3 -dump Resnet_siren_9_Appli-trusted.bin -align
```

The value `0x80000000` used by the signing tool is **not** the external-flash programming address.

## 9. Neural-network binary

Neural-ART generates the network data file as:

`network_atonbuf.xSPI2.raw`

For programming, a copy of this file is stored with a `.bin` extension as:

`network_atonbuf.xSPI2.bin`

Only the filename/extension is changed; the binary contents remain unchanged.

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

The `prebuilt/` directory contains the validated final configuration:

- `Resnet_siren_final_FSBL-trusted.bin`
- `Resnet_siren_Final_Appli-trusted.bin`
- `network_atonbuf.xSPI2.bin`

These binaries correspond to the real-time dual-microphone configuration using:

```c
#define MIC_MODE MIC_MODE_AVERAGE
```

Program them as follows:

- `Resnet_siren_final_FSBL-trusted.bin` at `0x70000000`
- `Resnet_siren_Final_Appli-trusted.bin` at `0x70100000`
- `network_atonbuf.xSPI2.bin` at `0x71000000`

The prebuilt files allow the validated final firmware to be reproduced without rebuilding or signing the source project.

## 12. Reproduction workflows

Two approaches are available.

**Build from source**

`Import → Verify MIC_MODE → Build → Sign → Program → Boot`

**Use prebuilt binaries**

`Program final FSBL + final application + network binary → Boot`

The source-build workflow is recommended when modifying the firmware.

The prebuilt binaries correspond to the validated final dual-microphone implementation used in the Master's Thesis.