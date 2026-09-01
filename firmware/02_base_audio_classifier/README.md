# Base Audio Classifier — STM32N657

This project represents an intermediate milestone in the development of the embedded audio AI system implemented for the Master's Thesis.

At this stage, a complete acoustic-event classification pipeline was deployed on the STM32N657 platform, combining real-time audio acquisition, embedded log-Mel feature extraction and neural-network inference.

This implementation precedes the siren-specific transfer-learning, INT8 quantization and Neural-ART/NPU acceleration stages developed later in the project.

## 1. Features

The firmware includes:

- digital microphone acquisition through I2S;
- DMA-based audio buffering;
- 16 kHz audio processing;
- log-Mel feature extraction on the STM32N657;
- CMSIS-DSP processing;
- deployment of a pre-trained acoustic-event classification model;
- inference using X-CUBE-AI;
- UART output for debugging and result inspection.

The base neural network used in this stage was obtained from an STMicroelectronics example. It was not trained specifically for this Master's Thesis.

> **TODO:** add the exact STMicroelectronics repository, model file and version used.

## 2. Project structure

The complete STM32CubeIDE project snapshot used during development is preserved.

Main components:

- `Appli/` — main STM32 application.
- `FSBL/` — First Stage Boot Loader.
- `Drivers/` — STM32, CMSIS and CMSIS-DSP dependencies.
- `Middlewares/` — STM32 AI middleware.
- `Secure_nsclib/` — secure support files required by this project.
- `prebuilt/` — validated binaries ready for programming.
- `Base_AI_Model_MIC_4.ioc` — original STM32CubeMX configuration.

The `.project`, `.cproject` and `.settings` files are intentionally preserved because they contain the STM32CubeIDE configuration used for the validated build.

The directory structure should be kept unchanged, since the STM32CubeIDE projects use relative paths to shared resources.

## 3. Importing the project

The recommended approach is to import the existing projects directly into STM32CubeIDE.

Regenerating the project from the `.ioc` file is **not required** for normal use.

1. Clone or download the repository.
2. Keep the complete directory structure unchanged.
3. Open STM32CubeIDE.
4. Select `File > Import > General > Existing Projects into Workspace`.
5. Import the project contained in `Appli/`.
6. Import the project contained in `FSBL/`.

The `.ioc` file is retained mainly for reference and inspection of the original STM32CubeMX configuration.

## 4. CMSIS-DSP configuration

CMSIS-DSP is required by the embedded audio preprocessing implementation, particularly for the FFT operations used to generate the log-Mel features.

The required CMSIS-DSP files are already included under:

`Drivers/CMSIS/DSP/`

They do not need to be copied manually into `Appli/`.

The validated `Debug` configuration preserves the relevant Cortex-M55 DSP settings, including:

- `ARM_MATH_CM55`
- `../../Drivers/CMSIS/DSP/Include`
- `../../Drivers/CMSIS/DSP/Source`
- `../../Drivers/CMSIS/DSP/PrivateInclude`
- `-flax-vector-conversions`

These settings are stored in the supplied STM32CubeIDE project configuration.

For this reason, importing the existing project is preferable to creating a new CubeIDE project and manually recreating the CMSIS-DSP integration.

## 5. Main custom files

The main custom or manually modified files for this stage are:

- `Appli/Core/Src/main.c`
- `Appli/Core/Src/audio_features.c`
- `Appli/Core/Inc/audio_features.h`
- `Appli/Core/Inc/mel_filters_sparse.h`

`audio_features.c/.h` implement the embedded acoustic preprocessing.

`mel_filters_sparse.h` contains the sparse Mel-filter representation used by the preprocessing pipeline.

The repository also contains STM32Cube-generated code, ST middleware, CMSIS/CMSIS-DSP components and X-CUBE-AI-generated files required to preserve the validated project.

## 6. Building

The configuration used and validated during development was:

**Debug**

Both projects must be built:

1. `FSBL`
2. `Appli`

After importing the projects:

1. select the `Debug` configuration;
2. build `FSBL`;
3. build `Appli`.

The resulting raw binaries are generated inside the corresponding `Debug` directories.

The repository does not preserve the complete `Debug` build folders because they mainly contain regenerable compilation artifacts.

## 7. Generating trusted images

On STM32N6, the FSBL and application binaries must be converted into **trusted images** before flashing.

The procedure below follows the instructions provided by STMicroelectronics technical support for this project.

### Requirements

Use:

**STM32CubeProgrammer v2.21.0**

The required signing utility is:

`STM32_SigningTool_CLI.exe`

It is recommended to add the directory containing this executable to the Windows system `PATH`.

With STM32CubeProgrammer v2.21 or later, the `-align` option must be included.

### FSBL

Open a command prompt inside `FSBL/Debug/`.

For the binary used during development:

```text
Base_AI_Model_FSBL.bin
```

run:

```cmd
STM32_SigningTool_CLI.exe -bin Base_AI_Model_FSBL.bin -nk -of 0x80000000 -t fsbl -o FSBL-trusted.bin -hv 2.3 -dump FSBL-trusted.bin -align
```

### Application

Open a command prompt inside `Appli/Debug/`.

For the binary used during development:

```text
Base_AI_Model_Appli.bin
```

run the same trusted-image generation procedure using the application binary:

```cmd
STM32_SigningTool_CLI.exe -bin Base_AI_Model_Appli.bin -nk -of 0x80000000 -t fsbl -o Appli-trusted.bin -hv 2.3 -dump Appli-trusted.bin -align
```

If STM32CubeIDE generates different binary names, replace the input filenames accordingly.

The value `0x80000000` used with the signing tool is **not** the address used later to program the binaries into external flash.

## 8. Programming the board

Three files are required:

| Binary | Flash address |
|---|---:|
| `FSBL-trusted.bin` | `0x70000000` |
| `Appli-trusted.bin` | `0x70100000` |
| `network_data.bin` | `0x71000000` |

Program them using STM32CubeProgrammer v2.21.0.

After programming:

1. disconnect the programming session;
2. configure the board for the boot mode used to execute the application from external flash;
3. reset or power-cycle the board.

> **TODO:** document the exact jumper/boot configuration used on the development board.

## 9. Prebuilt binaries

The `prebuilt/` directory contains the validated binaries corresponding to this project:

- `FSBL-trusted.bin`
- `Appli-trusted.bin`
- `network_data.bin`

They can be programmed directly using the addresses listed in the previous section, without rebuilding or signing the project first.

This provides two possible reproduction workflows:

**Build from source**

`Import → Build FSBL + Appli → Sign → Program → Boot`

**Use validated binaries**

`Program prebuilt binaries → Boot`

The first option is recommended when modifying the firmware. The second is useful for reproducing the validated implementation as directly as possible.