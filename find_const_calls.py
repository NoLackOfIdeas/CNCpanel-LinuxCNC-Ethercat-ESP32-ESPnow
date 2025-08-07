#!/usr/bin/env python3
import os, re

pattern = re.compile(r'\bLV_EXPORT_CONST_INT\s*\(')
extensions = {'.c', '.cpp', '.h', '.hpp'}

for root, _, files in os.walk('.'):
    for fname in files:
        if os.path.splitext(fname)[1] in extensions:
            path = os.path.join(root, fname)
            with open(path, encoding='utf-8', errors='ignore') as f:
                for lineno, line in enumerate(f, 1):
                    # Skip the macro definition itself
                    if fname == 'lv_conf_internal.h' and 'define LV_EXPORT_CONST_INT' in line:
                        continue

                    if pattern.search(line):
                        print(f"{path}:{lineno}: {line.strip()}")
