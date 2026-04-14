"""
Generate placeholder RPG hit impact sounds using synthesis.
Produces 5 wav files: 3 normal hits + 2 critical hits.

Normal hits: short percussive thud with noise burst (~100ms)
Critical hits: sharper, higher-pitched crunch with more punch (~120ms)

These are functional placeholders — replace with professionally recorded
or sourced sounds (freesound.org, opengameart.org) for production.
"""

import wave
import struct
import math
import random
import os

SAMPLE_RATE = 44100
OUTPUT_DIR_NORMAL = r"C:\Sabri_MMO\client\SabriMMO\Content\SabriMMO\Audio\HitSounds\Normal"
OUTPUT_DIR_CRIT = r"C:\Sabri_MMO\client\SabriMMO\Content\SabriMMO\Audio\HitSounds\Critical"


def generate_samples(duration_ms, sample_rate=SAMPLE_RATE):
    """Return number of samples for given duration."""
    return int(sample_rate * duration_ms / 1000)


def envelope_percussive(t, attack_ms=2, decay_ms=80, total_ms=100):
    """Sharp attack, fast exponential decay. t in [0,1] normalized time."""
    time_ms = t * total_ms
    if time_ms < attack_ms:
        return time_ms / attack_ms  # Linear attack
    else:
        decay_t = (time_ms - attack_ms) / decay_ms
        return math.exp(-decay_t * 4.0)  # Fast exponential decay


def white_noise():
    """Random noise sample in [-1, 1]."""
    return random.uniform(-1.0, 1.0)


def generate_normal_hit(variant=0):
    """
    Normal hit: low-mid frequency thud + noise burst.
    Sounds like a blunt weapon impact.
    """
    duration_ms = 100 + variant * 10  # 100-120ms
    num_samples = generate_samples(duration_ms)
    samples = []

    # Parameters vary per variant for organic feel
    base_freq = 120 + variant * 15  # 120-150 Hz (low thud)
    noise_amount = 0.35 + variant * 0.05
    tone_amount = 0.65 - variant * 0.05

    for i in range(num_samples):
        t = i / num_samples  # Normalized time [0, 1]
        time_s = i / SAMPLE_RATE

        # Percussive envelope
        env = envelope_percussive(t, attack_ms=1, decay_ms=60 + variant * 10, total_ms=duration_ms)

        # Low frequency body (the "thud")
        tone = math.sin(2 * math.pi * base_freq * time_s)
        # Pitch drops slightly over time (impact characteristic)
        freq_drop = base_freq * (1.0 - t * 0.3)
        tone = math.sin(2 * math.pi * freq_drop * time_s)

        # Noise burst (the "crack" transient)
        noise = white_noise()

        # Mix
        sample = env * (tone * tone_amount + noise * noise_amount)

        # Soft clip
        sample = max(-0.95, min(0.95, sample))
        samples.append(sample)

    return samples


def generate_crit_hit(variant=0):
    """
    Critical hit: higher pitched, sharper attack, more noise.
    Sounds like a harder, more dramatic impact.
    """
    duration_ms = 130 + variant * 15  # 130-145ms (slightly longer)
    num_samples = generate_samples(duration_ms)
    samples = []

    base_freq = 200 + variant * 30  # Higher pitch than normal
    mid_freq = 400 + variant * 50  # Extra mid-frequency "crunch"

    for i in range(num_samples):
        t = i / num_samples
        time_s = i / SAMPLE_RATE

        # Sharper envelope — faster attack, slightly longer sustain
        env = envelope_percussive(t, attack_ms=1, decay_ms=80 + variant * 10, total_ms=duration_ms)

        # Initial transient boost (first 5ms is extra loud)
        transient = 1.0
        if t * duration_ms < 5:
            transient = 1.5

        # Low body
        freq1 = base_freq * (1.0 - t * 0.4)
        tone1 = math.sin(2 * math.pi * freq1 * time_s) * 0.4

        # Mid crunch (gives it the "critical" character)
        freq2 = mid_freq * (1.0 - t * 0.5)
        tone2 = math.sin(2 * math.pi * freq2 * time_s) * 0.25

        # More noise than normal hit
        noise = white_noise() * 0.35

        # Mix with transient boost
        sample = env * transient * (tone1 + tone2 + noise)

        # Soft clip
        sample = max(-0.95, min(0.95, sample))
        samples.append(sample)

    return samples


def write_wav(filepath, samples, sample_rate=SAMPLE_RATE):
    """Write mono 16-bit PCM WAV file."""
    with wave.open(filepath, 'w') as wav_file:
        wav_file.setnchannels(1)  # Mono
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(sample_rate)

        for sample in samples:
            # Convert float [-1, 1] to int16
            int_sample = int(sample * 32767)
            int_sample = max(-32768, min(32767, int_sample))
            wav_file.writeframes(struct.pack('<h', int_sample))


def main():
    os.makedirs(OUTPUT_DIR_NORMAL, exist_ok=True)
    os.makedirs(OUTPUT_DIR_CRIT, exist_ok=True)

    # Generate 3 normal hit variants
    for i in range(3):
        random.seed(42 + i * 7)  # Reproducible but different per variant
        samples = generate_normal_hit(variant=i)
        path = os.path.join(OUTPUT_DIR_NORMAL, f"Hit_Normal_{i+1:02d}.wav")
        write_wav(path, samples)
        print(f"Generated: {path} ({len(samples)} samples, {len(samples)/SAMPLE_RATE*1000:.0f}ms)")

    # Generate 2 critical hit variants
    for i in range(2):
        random.seed(100 + i * 13)
        samples = generate_crit_hit(variant=i)
        path = os.path.join(OUTPUT_DIR_CRIT, f"Hit_Crit_{i+1:02d}.wav")
        write_wav(path, samples)
        print(f"Generated: {path} ({len(samples)} samples, {len(samples)/SAMPLE_RATE*1000:.0f}ms)")

    print("\nDone! 5 placeholder hit sounds generated.")
    print("Import these into UE5 as SoundWave assets.")
    print("Replace with professionally sourced sounds for production.")


if __name__ == "__main__":
    main()
