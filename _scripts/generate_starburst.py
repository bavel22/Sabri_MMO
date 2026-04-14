"""Generate a RO Classic-style starburst texture for critical hit background.
Sharp radial spikes with a glowing center — faithful to RO's critbg sprite."""
import struct
import zlib
import os
import math

OUTPUT = r"C:\Sabri_MMO\client\SabriMMO\Content\SabriMMO\UI\Textures\T_CritStarburst.png"
SIZE = 128
NUM_SPIKES = 12
CENTER_GLOW_RADIUS = 0.18  # fraction of half-size
SPIKE_LENGTH = 0.45         # fraction of half-size (tip distance from center)
SPIKE_BASE_WIDTH = 0.14     # radians half-width at base
SPIKE_TIP_WIDTH = 0.02      # radians half-width at tip

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

            alpha = 0.0

            # Center glow — soft radial gradient
            if dist < CENTER_GLOW_RADIUS:
                glow = 1.0 - (dist / CENTER_GLOW_RADIUS) ** 0.5
                alpha = max(alpha, glow)
            elif dist < CENTER_GLOW_RADIUS * 1.5:
                # Soft fade beyond center
                fade = 1.0 - (dist - CENTER_GLOW_RADIUS) / (CENTER_GLOW_RADIUS * 0.5)
                alpha = max(alpha, fade * 0.4)

            # Spikes — sharp triangular rays
            for s in range(NUM_SPIKES):
                spike_angle = s * 2 * math.pi / NUM_SPIKES
                diff = abs(angle - spike_angle)
                if diff > math.pi:
                    diff = 2 * math.pi - diff

                # Spike width tapers from base to tip
                if dist < 0.01:
                    continue
                t = min(1.0, (dist - CENTER_GLOW_RADIUS * 0.5) / SPIKE_LENGTH) if dist > CENTER_GLOW_RADIUS * 0.5 else 0.0
                width_at_dist = SPIKE_BASE_WIDTH * (1.0 - t) + SPIKE_TIP_WIDTH * t

                if diff < width_at_dist:
                    # Inside spike
                    spike_start = CENTER_GLOW_RADIUS * 0.5
                    spike_end = CENTER_GLOW_RADIUS * 0.5 + SPIKE_LENGTH

                    if dist >= spike_start and dist <= spike_end:
                        # Brightness: bright at base, fades at tip
                        progress = (dist - spike_start) / SPIKE_LENGTH
                        brightness = (1.0 - progress ** 0.7) * 0.95
                        # Edge softness
                        edge = 1.0 - (diff / width_at_dist) ** 2
                        alpha = max(alpha, brightness * edge)

            # Clamp and write pixel
            a = min(255, max(0, int(alpha * 255)))
            idx = (y * SIZE + x) * 4
            pixels[idx] = 255      # R
            pixels[idx+1] = 255    # G
            pixels[idx+2] = 255    # B
            pixels[idx+3] = a

    return pixels

def main():
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    pixels = generate_starburst()
    write_png(OUTPUT, SIZE, SIZE, pixels)
    print(f"Generated: {OUTPUT} ({SIZE}x{SIZE})")

if __name__ == "__main__":
    main()
