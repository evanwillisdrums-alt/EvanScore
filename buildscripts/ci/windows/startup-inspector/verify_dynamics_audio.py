"""Verify real MS Basic audio exported from the GUI-saved custom dynamics scores."""
from pathlib import Path
import wave

for folder in ('score', 'drum-score'):
    path = Path('checks', folder, 'custom-dynamics.wav')
    with wave.open(str(path), 'rb') as audio:
        assert audio.getnframes() > 0, f'No audio frames: {folder}'
        assert audio.getframerate() > 0 and audio.getnchannels() > 0
        width = audio.getsampwidth()
        frames = audio.readframes(audio.getnframes())
        silence = bytes([128]) if width == 1 else bytes([0])
        assert frames.strip(silence), f'Entire render is silent: {folder}'
        print(f'::notice title=Dynamics audio render::{folder}: '
              f'{audio.getnframes()} frames at {audio.getframerate()} Hz, non-silent MS Basic output')
