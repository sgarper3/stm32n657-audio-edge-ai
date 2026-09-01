import numpy as np
import soundfile as sf

SAMPLE_RATE = 16000
INPUT_FILE = "Audio"
OUTPUT_FILE = "audio.wav"

# Read signed 16-bit PCM samples captured from the UART stream.
audio = np.fromfile(INPUT_FILE, dtype=np.int16)

# Save the captured PCM stream as a WAV file.
sf.write(OUTPUT_FILE, audio, SAMPLE_RATE)

print(f"Saved {len(audio)} samples to {OUTPUT_FILE}")
print(f"Duration: {len(audio) / SAMPLE_RATE:.2f} s")