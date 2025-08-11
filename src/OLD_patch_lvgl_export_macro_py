import os
import re

# Path to your LVGL source directory
LVGL_SRC_DIR = ".pio/libdeps/esp3/lvgl/src"

# Regex to match LV_EXPORT_CONST_INT(XYZ);
EXPORT_CONST_PATTERN = re.compile(r"\bLV_EXPORT_CONST_INT\s*\(\s*([A-Za-z0-9_]+)\s*\)\s*;")

def patch_file(filepath):
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # Replace all matches with LV_EXPORT_CONST_INT(XYZ, XYZ);
    patched_content = EXPORT_CONST_PATTERN.sub(r"LV_EXPORT_CONST_INT(\1, \1);", content)

    if patched_content != content:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(patched_content)
        print(f"✅ Patched: {filepath}")
    else:
        print(f"⏭️ No changes: {filepath}")

def patch_lvgl_directory(root_dir):
    for subdir, _, files in os.walk(root_dir):
        for file in files:
            if file.endswith(".h") or file.endswith(".c") or file.endswith(".cpp"):
                patch_file(os.path.join(subdir, file))

if __name__ == "__main__":
    print(f"🔍 Scanning LVGL source directory: {LVGL_SRC_DIR}")
    patch_lvgl_directory(LVGL_SRC_DIR)
    print("🎉 Done patching LV_EXPORT_CONST_INT macros.")
