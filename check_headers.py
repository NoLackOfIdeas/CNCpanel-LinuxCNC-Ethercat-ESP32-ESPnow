import os
import re

# 🔍 Configuration
SOURCE_DIR = "lib/lvgl"  # Root directory to scan
INCLUDE_PATHS = [
    "lib/lvgl/src",
    "lib/lvgl/src/misc",
    "lib/lvgl/src/core",
    "lib/lvgl/src/indev",
    "lib/lvgl/src/others",
    "lib/lvgl/src/widgets",
    "lib/lvgl/src/libs",
    # Add more paths as needed
]

# 📦 Regex to match #include statements
INCLUDE_REGEX = re.compile(r'#include\s+[<"]([^">]+)[">]')

# 🧠 Cache for found headers
found_headers = set()

def find_header(header, current_dir):
    # Check relative to current file
    rel_path = os.path.normpath(os.path.join(current_dir, header))
    if os.path.isfile(rel_path):
        return True

    # Check in include paths
    for path in INCLUDE_PATHS:
        full_path = os.path.normpath(os.path.join(path, header))
        if os.path.isfile(full_path):
            return True

    return False

def scan_file(file_path):
    missing = []
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
        includes = INCLUDE_REGEX.findall(content)
        for inc in includes:
            if inc in found_headers:
                continue
            if not find_header(inc, os.path.dirname(file_path)):
                missing.append((file_path, inc))
            else:
                found_headers.add(inc)
    return missing

def scan_directory(root):
    all_missing = []
    for dirpath, _, filenames in os.walk(root):
        for filename in filenames:
            if filename.endswith((".c", ".cpp", ".h")):
                full_path = os.path.join(dirpath, filename)
                missing = scan_file(full_path)
                all_missing.extend(missing)
    return all_missing

# 🚀 Run the scan
missing_headers = scan_directory(SOURCE_DIR)

# 📋 Report
if missing_headers:
    print("❌ Missing headers found:")
    for file, header in missing_headers:
        print(f"  {file} → {header}")
else:
    print("✅ All headers resolved successfully.")
