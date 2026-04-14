"""Generate a red RO Classic starburst for critical hit background.
Uses the original spike shape (tapered, soft edges) but with solid red color
and full opacity."""
import struct
import zlib
import os
import math

OUTPUT = r"C:\Sabri_MMO\client\SabriMMO\Content\SabriMMO\UI\Textures\T_CritStarburst.png"
SIZE = 128
NUM_SPIKES = 12
CENTER_GLOW_RADIUS = 0.18
SPIKE_LENGTH = 0.45
SPIKE_BASE_WIDTH = 0.14
SPIKE_TIP_WIDTH = 0.02

def write_png(filepath, width, height, pixels):
    def make_chunk(chunk_type, data):
        raw = chunk_type + data
        return struct.pack('>I', len(data)) + raw + struct.pack('>I', zlib.crc32(raw) & 0xFFFFFFFF)
    signature = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    raw_data = b''
    for y in range(height):
        raw_data += b'\x00'
        for x in range(width):
            idx = (y * width + x) * 4
            raw_data += bytes(pixels[idx:idx+4])
    compressed = zlib.compress(raw_data)
    with open(filepath, 'wb') as f:
        f.write(signature)
        f.write(make_chunk(b'IHDR', ihdr))
        f.write(make_chunk(b'IDAT', compressed))
        f.write(make_chunk(b'IEND', b''))

def generate_starburst():
    pixels = [0] * (SIZE * SIZE * 4)
    cx, cy = SIZE / 2.0, SIZE / 2.0
    half = SIZE / 2.0

    for y in range(SIZE):
        for x in range(SIZE):
            dx = (x - cx) / half
            dy = (y - cy) / half
            dist = math.sqrt(dx*dx + dy*dy)
            angle = math.atan2(dy, dx)

            intensity = 0.0

            # Center glow — soft radial gradient
            if dist < CENTER_GLOW_RADIUS:
                glow = 1.0 - (dist / CENTER_GLOW_RADIUS) ** 0.5
                intensity = max(intensity, glow)
            elif dist < CENTER_GLOW_RADIUS * 1.5:
                fade = 1.0 - (dist - CENTER_GLOW_RADIUS) / (CENTER_GLOW_RADIUS * 0.5)
                intensity = max(intensity, fade * 0.4)

            # Spikes — sharp triangular rays
            for s in range(NUM_SPIKES):
                spike_angle = s * 2 * math.pi / NUM_SPIKES
                diff = abs(angle - spike_angle)
                if diff > math.pi:
                    diff = 2 * math.pi - diff

                if dist < 0.01:
                    continue
                t = min(1.0, (dist - CENTER_GLOW_RADIUS * 0.5) / SPIKE_LENGTH) if dist > CENTER_GLOW_RADIUS * 0.5 else 0.0
                width_at_dist = SPIKE_BASE_WIDTH * (1.0 - t) + SPIKE_TIP_WIDTH * t

                if diff < width_at_dist:
                    spike_start = CENTER_GLOW_RADIUS * 0.5
                    spike_end = CENTER_GLOW_RADIUS * 0.5 + SPIKE_LENGTH

                    if dist >= spike_start and dist <= spike_end:
                        progress = (dist - spike_start) / SPIKE_LENGTH
                        brightness = (1.0 - progress ** 0.7) * 0.95
                        edge = 1.0 - (diff / width_at_dist) ** 2
                        intensity = max(intensity, brightness * edge)

            if intensity > 0.01:
                # Red-orange color, brighter yellow-white at center
                center_blend = max(0, 1.0 - dist * 3.0)

                r = int(min(255, (200 + 55 * center_blend) * intensity))
                g = int(min(255, (60 + 80 * center_blend) * intensity))
                b = int(min(255, (20 + 50 * center_blend) * intensity))

                # Full opacity wherever there's any intensity
                a = 255

                idx = (y * SIZE + x) * 4
                pixels[idx] = r
                pixels[idx+1] = g
                pixels[idx+2] = b
                pixels[idx+3] = a

    return pixels

def main():
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    pixels = generate_starburst()
    write_png(OUTPUT, SIZE, SIZE, pixels)
    print(f"Generated: {OUTPUT}")

if __name__ == "__main__":
    main()
