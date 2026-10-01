#!/usr/bin/env python3
# Copyright (c) 2026 Bruce Blay
# SPDX-License-Identifier: GPL-3.0-or-later
"""Make the vector release cover from the lab's Contrapunctus I note data.

Run with a sibling rill-sound checkout. Rasterize cover.svg to 1200x630 PNG
using an SVG renderer such as Sharp. The score artwork is combined with the shared 3D device render.
"""
import base64
import json
from pathlib import Path
from html import escape

root = Path(__file__).resolve().parents[1]
score = json.loads((root.parent / 'rill-sound/public/bach/contrapunctus-1.json').read_text())
svg = ['''<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="630" viewBox="0 0 1200 630">
<defs><clipPath id="disc"><circle cx="926" cy="280" r="290"/></clipPath></defs>
<rect width="1200" height="630" fill="#f3f0e7"/>
<g font-family="Helvetica,Arial,sans-serif" fill="#202b29">
<text x="54" y="65" font-size="24" font-weight="600" letter-spacing="-.6">Rill Sound</text>
<text x="48" y="230" font-size="100" letter-spacing="-5">Chamber</text>
<text x="54" y="438" font-size="23">Small devices.</text>
<text x="54" y="471" font-size="23">A whole ensemble.</text></g>
<g clip-path="url(#disc)"><rect x="630" width="600" height="570" fill="#0b0e0c"/>
''']
colors = ['#b8d98a', '#e4c77a', '#91bac1', '#cc9387']
for i, voice in enumerate(score['voices'][:4]):
    top = 78 + i * 111
    svg.append(f'<text x="750" y="{top}" font-family="Helvetica,Arial,sans-serif" font-size="13" fill="#8a9082" letter-spacing="2">{escape(voice["name"].upper())}</text>')
    for line in range(4):
        y = top + 17 + line * 18
        svg.append(f'<path d="M670 {y}H1215" stroke="#30362e" stroke-width="1"/>')
    # Beats 64–96: all four voices have entered. Pitch is vertical within each part.
    notes = [(t - 64, d, n) for t, d, n in voice['notes'] if 64 <= t < 96]
    if notes:
        low, high = min(n for _, _, n in notes), max(n for _, _, n in notes)
        for t, d, n in notes:
            x = 670 + t * 16
            y = top + 65 - (n - low) / max(1, high - low) * 48
            width = max(3, min(d, 32 - t) * 16 - 2)
            svg.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{width:.2f}" height="6" rx="2" fill="{colors[i]}"/>')
svg.append('</g>')
mock = root / 'release/device-transparent.png'
if mock.exists():
    encoded = base64.b64encode(mock.read_bytes()).decode()
    svg.append('<defs><filter id="device-shadow" x="-30%" y="-30%" width="160%" height="170%"><feDropShadow dx="0" dy="14" stdDeviation="12" flood-color="#000000" flood-opacity=".45"/></filter></defs>')
    svg.append(f'<image x="672" y="0" width="460" height="575" href="data:image/png;base64,{encoded}" filter="url(#device-shadow)"/>')
svg.append('''<rect y="552" width="1200" height="78" fill="#f3f0e7"/>
<path d="M54 552.5H1146" stroke="#202b29" stroke-opacity=".22"/>
<g font-family="Helvetica,Arial,sans-serif" font-size="19" fill="#202b29">
<text x="54" y="600">rillsound.com</text>
<text x="1146" y="600" text-anchor="end">Rill Chamber</text></g>
</svg>''')
(root / 'release/cover.svg').write_text('\n'.join(svg))
