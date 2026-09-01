# Audio Capture and UART Streaming — STM32N657

This project represents the first functional audio-acquisition stage of the Master's Thesis development.

It validates the acquisition of real audio from an INMP441 digital microphone using I2S and DMA on the STM32N657, followed by PCM transmission to a PC through UART.

## Features

- INMP441 digital microphone acquisition through I2S.
- DMA-based continuous audio reception.
- 16 kHz sampling rate.
- 24-bit signed PCM sample extraction.
- Conversion to 16-bit samples for UART transmission.
- Ping-pong buffering using DMA half-transfer and transfer-complete callbacks.
- Raw PCM streaming through UART at 921600 baud.

The transmitted data can be captured on a PC and converted to a standard audio format such as WAV for inspection and validation.

## Files

- `prueba_UART_2.ioc` — STM32CubeMX/STM32CubeIDE peripheral configuration.
- `main.c` — audio acquisition, PCM conversion and UART streaming implementation.

Unlike the later firmware stages in this repository, this project is intentionally provided as a minimal example rather than as a complete STM32CubeIDE project snapshot.

## Reproduction

1. Open the `.ioc` file using STM32CubeMX or STM32CubeIDE.
2. Generate the STM32 project.
3. Replace the generated application `main.c` with the version provided here.
4. Build and program the STM32N657.
5. Connect the INMP441 microphone and capture the binary UART stream at 921600 baud.

This stage was used to verify the complete microphone-to-PC audio path before introducing embedded feature extraction and neural-network inference.