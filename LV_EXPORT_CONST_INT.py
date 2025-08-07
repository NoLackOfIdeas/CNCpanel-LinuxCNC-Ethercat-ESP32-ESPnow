#!/usr/bin/env python3
import os
import re

# Pattern to match occurrences of LV_EXPORT_CONST_INT(
pattern = re.compile(r'\bLV_EXPORT_CONST_INT\s*\(')
# File extensions to scan
extensions = {'.c', '.cpp', '.h', '.hpp'}

for root, _, files in os.walk('.'):
    for fname in files:
        ext = os.path.splitext(fname)[1]
        if ext in extensions:
            fpath = os.path.join(root, fname)
            try:
                with open(fpath, 'r', encoding='utf-8', errors='ignore') as f:
                    for lineno, line in enumerate(f, start=1):
                        if pattern.search(line):
                            print(f"{fpath}:{lineno}: {line.strip()}")
            except OSError:
                pass
