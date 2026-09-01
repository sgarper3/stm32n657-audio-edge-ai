## Validation example

A short UART capture and the corresponding WAV conversion are provided under:

`validation/01_audio_capture_uart/`

The example demonstrates the complete acquisition path from the INMP441
microphone through STM32 I2S/DMA and UART to an audible WAV file.

The conversion utility is available at:

`tools/uart_raw_to_wav.py`