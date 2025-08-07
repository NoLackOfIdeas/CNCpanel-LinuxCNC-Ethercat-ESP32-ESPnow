import os
import re

LVGL_SRC = os.path.join("lib", "lvgl", "src")
MATCH = re.compile(r'\bLV_EXPORT_CONST_INT\s*\(\s*([A-Za-z0-9_]+)\s*\)\s*;')

def patch_file(path):
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    changed = False
    for i, line in enumerate(lines):
        m = MATCH.search(line)
        if m:
            const = m.group(1)
            new_line = f"LV_EXPORT_CONST_INT({const}, {const});\n"
            lines[i] = new_line
            changed = True
            print(f"  → Patched: {path} :: {const}")

    if changed:
        with open(path, "w", encoding="utf-8") as f:
            f.writelines(lines)

def patch_lvgl_sources():
    print("🔧 Patching LV_EXPORT_CONST_INT calls...")
    for root, _, files in os.walk(LVGL_SRC):
        for fn in files:
            if fn.endswith((".h", ".c")):
                patch_file(os.path.join(root, fn))
    print("✅ Done.")

if __name__ == "__main__":
    patch_lvgl_sources()
