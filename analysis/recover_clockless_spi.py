#!/usr/bin/env python3
"""Offline virtual-clock experiment for the supplied broken Saleae recording.

Uses a healthy SPI2 recording to calibrate the burst schedule; validates it
against that recording's real clock, then anchors it to surviving clock1 in the
broken capture. It does not generate an electrical clock or change firmware.
"""
import argparse
import bisect
import collections
import csv
import json
from pathlib import Path
import statistics


def load(path):
    with path.open() as f:
        rows = csv.reader(f)
        next(rows)
        return [(float(r[0]), *map(int, r[1:])) for r in rows]


def frames(rows, select, clock):
    result = []
    current = None
    for before, after in zip(rows, rows[1:]):
        if before[select + 1] != after[select + 1]:
            if after[select + 1] == 0:
                current = [after[0], []]
            elif current is not None:
                result.append((current[0], after[0], current[1]))
                current = None
        if current is not None and before[clock + 1] == 0 and after[clock + 1] == 1:
            current[1].append(after[0])
    return result


def sampler(rows, channel):
    times = [r[0] for r in rows]
    return lambda t: rows[max(0, bisect.bisect_right(times, t) - 1)][channel + 1]


def pack(bits):
    return bytes(sum(bits[i + j] << j for j in range(8)) for i in range(0, 120, 8))


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--raw', type=Path, default=root/'captures/broken-capture/raw/digital.csv')
    parser.add_argument('--reference', type=Path, default=root/'captures/title-test-01-0123456789AB-raw/digital.csv')
    parser.add_argument('--output', type=Path, default=root/'captures/broken-capture/clockless-recovery.json')
    args = parser.parse_args()
    healthy = load(args.reference)
    reference_frames = [f for f in frames(healthy, 7, 3) if len(f[2]) == 120]
    if not reference_frames:
        raise ValueError('Reference has no complete 120-clock SPI2 frames')
    template = [statistics.median(f[2][i] - f[2][0] for f in reference_frames) for i in range(120)]
    reference_data = sampler(healthy, 2)
    matches = sum(pack([reference_data(t) for t in f[2]]) ==
                  pack([reference_data(f[2][0] + offset + 6e-6) for offset in template])
                  for f in reference_frames)
    broken = load(args.raw)
    observed = [f for f in frames(broken, 0, 1) if len(f[2]) == 128]
    recovered = {}
    for channel in (6, 7):
        value = sampler(broken, channel)
        decoded = [pack([value(f[2][0] + offset + 6e-6) for offset in template]) for f in observed]
        # Examine the whole middle region, not just selected sample instants.
        flat_low = 0
        for f in observed:
            start, end = f[2][0] + .00165, f[2][0] + .0108
            if value(start) == 0 and not any(start < b[0] < end and a[channel+1] != b[channel+1]
                                           for a, b in zip(broken, broken[1:])):
                flat_low += 1
        phase_results = []
        for phase_us in range(0, 21):
            packets = [pack([value(f[2][0] + offset + phase_us*1e-6) for offset in template]) for f in observed]
            phase_results.append({'phase_us': phase_us,
                                  'incrementing_headers': sum((b[0]-a[0]) % 256 == 1 for a, b in zip(packets, packets[1:]))})
        recovered[str(channel)] = {
            'complete_frames': len(decoded),
            'incrementing_headers': sum((b[0]-a[0]) % 256 == 1 for a, b in zip(decoded, decoded[1:])),
            'mode_counts': dict(collections.Counter(p[1] for p in decoded)),
            'payloads_hex': dict(collections.Counter(p[2:14].hex() for p in decoded)),
            'entire_middle_region_flat_low_frames': flat_low,
            'fixed_cs1_offset_trials': [
                {'first_sample_after_select_us': delay*1e6,
                 'incrementing_headers': sum((b[0]-a[0]) % 256 == 1 for a, b in zip(packets, packets[1:])),
                 'first_packets': [p.hex() for p in packets[:3]]}
                for delay in (.003012, .00332825)
                for packets in [[pack([value(f[0]+delay+offset) for offset in template]) for f in observed]]
            ],
            'phase_sweep': phase_results,
            'packets': [{'start_s': f[0], 'hex': data.hex()} for f, data in zip(observed, decoded)],
        }
    result = {
        'method': 'LSB-first, calibrated 120-sample burst template, first working clock1 rising edge + 6 us',
        'reference_real_clock_frames': len(reference_frames),
        'reference_virtual_clock_exact_matches': matches,
        'reference_template_offsets_us': [v*1e6 for v in template],
        'broken_first_clock1_after_select_us': {
            'min': min(f[2][0]-f[0] for f in observed)*1e6,
            'median': statistics.median(f[2][0]-f[0] for f in observed)*1e6,
            'max': max(f[2][0]-f[0] for f in observed)*1e6,
        },
        'channels': recovered,
        'limits': 'No live acquisition, physical wire identification, analog voltage, CRC validation, or proof of the intended payload. Zero-filled text is not a recovered readable title.',
    }
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(f'Reference validation: {matches}/{len(reference_frames)} complete frames match the real clock')
    for channel, info in recovered.items():
        print(f'Channel {channel}: {info["complete_frames"]} frames, {info["incrementing_headers"]} incrementing header transitions')
        print('First packets:', [p['hex'] for p in info['packets'][:3]])
        print('Payloads:', info['payloads_hex'])
    print('Saved', args.output)


if __name__ == '__main__':
    main()
