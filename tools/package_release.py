#!/usr/bin/env python3
# Copyright (c) 2026 Bruce Blay
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a clean, versioned M5Burner package; never read or flash a device."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[1]
    version = (root / 'VERSION').read_text().strip()
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise SystemExit('VERSION must be major.minor.patch')
    name = f'rill-chamber-{version}'
    out = root / 'dist' / name
    if out.exists():
        raise SystemExit(f'Release directory already exists: {out}')
    if subprocess.check_output(['git', 'status', '--porcelain'], cwd=root, text=True).strip():
        raise SystemExit('Commit source changes before packaging a release.')
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
    core = Path(os.environ.get('PLATFORMIO_CORE_DIR', Path.home() / '.platformio'))
    build = root / '.pio/build/sticks3'
    subprocess.run([sys.executable, '-m', 'platformio', 'run', '-d', str(root)], check=True)
    segments = [(0, build / 'bootloader.bin'), (0x8000, build / 'partitions.bin'),
                (0xe000, core / 'packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin'),
                (0x10000, build / 'firmware.bin')]
    # These limits match the committed factory-only partitions.csv.
    limits = [0x8000, 0x9000, 0x10000, 0x710000]
    for (offset, path), limit in zip(segments, limits):
        if offset + path.stat().st_size > limit:
            raise SystemExit(f'Segment exceeds partition: {path}')
    out.parent.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.chamber-', dir=out.parent) as temporary:
        staging = Path(temporary)
        image = staging / f'{name}-factory.bin'
        args = [sys.executable, str(core / 'packages/tool-esptoolpy/esptool.py'), '--chip', 'esp32s3',
                'merge_bin', '-o', str(image), '--flash_mode', 'dio', '--flash_freq', '80m',
                '--flash_size', '8MB', '--fill-flash-size', '8MB']
        for offset, path in segments:
            args.extend([hex(offset), str(path)])
        subprocess.run(args, check=True)
        data = image.read_bytes()
        if len(data) != 8 * 1024 * 1024 or data[0x9000:0xe000] != b'\xff' * 0x5000:
            raise SystemExit('Factory size or blank NVS check failed')
        for offset, path in segments[1:]:
            if data[offset:offset + path.stat().st_size] != path.read_bytes():
                raise SystemExit(f'Merged segment mismatch: {path}')
        app_end = 0x10000 + segments[-1][1].stat().st_size
        if data[app_end:] != b'\xff' * (len(data) - app_end):
            raise SystemExit('Unused flash is not blank')
        shutil.copy2(build / 'firmware.bin', staging / f'{name}-app.bin')
        shutil.copy2(build / 'firmware.elf', staging / f'{name}.elf')
        for relative in ['README.md', 'CHANGELOG.md', 'LICENSE', 'VERSION', 'docs', 'release']:
            source = root / relative
            target = staging / relative
            if source.is_dir():
                shutil.copytree(source, target)
            else:
                shutil.copy2(source, target)
        subprocess.run(['git', 'archive', '--format=tar.gz', f'--prefix={name}/',
                        '-o', str(staging / f'{name}-source.tar.gz'), commit], cwd=root, check=True)
        manifest = {
            'name': 'rill-chamber', 'version': version, 'status': 'prepared-not-submitted',
            'device': 'M5Stack StickS3', 'source_commit': commit,
            'factory_offset': '0x0', 'factory_size': len(data),
            'factory_sha256': sha(image), 'blank_nvs_verified': True,
            'unused_flash_blank_verified': True, 'app_partition_size': 0x700000,
            'platformio': subprocess.check_output([sys.executable, '-m', 'platformio', '--version'], text=True).strip(),
            'dependencies': {'espressif32': '6.12.0', 'arduino-esp32': '2.0.17', 'M5Unified': '0.2.21', 'M5GFX': '0.2.28'},
            'segments': [{'offset': hex(o), 'file': p.name, 'size': p.stat().st_size, 'sha256': sha(p)} for o, p in segments],
            'hardware_acceptance': 'not performed during release preparation',
        }
        (staging / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        files = sorted(p for p in staging.rglob('*') if p.is_file())
        (staging / 'SHA256SUMS').write_text(''.join(f'{sha(p)}  {p.relative_to(staging)}\n' for p in files))
        staging.rename(out)
    print(out)


if __name__ == '__main__':
    main()
