# Copyright (c) 2026 Bruce Blay
# SPDX-License-Identifier: GPL-3.0-or-later
"""Embed the release version from the same file used by the packager."""
from pathlib import Path
Import("env")
version = (Path(env["PROJECT_DIR"]) / "VERSION").read_text().strip()
env.Append(CPPDEFINES=[("RILL_VERSION", env.StringifyMacro(version))])
