import os, shutil, sys

def copy_internal_config(project_dir):
    master_cfg = os.path.join(project_dir, "include", "lv_conf_internal.h")
    if not os.path.isfile(master_cfg):
        print("ERROR: lv_conf_internal.h not found at", master_cfg)
        return

    lvgl_src = os.path.join(project_dir, "lib", "lvgl", "src")
    copied_dirs = []

    for root, _, files in os.walk(lvgl_src):
        for fn in files:
            if not fn.endswith((".c", ".h")):
                continue
            try:
                text = open(os.path.join(root, fn), "r", encoding="utf-8").read()
            except:
                continue
            if 'lv_conf_internal.h' in text:
                dst = os.path.join(root, "lv_conf_internal.h")
                shutil.copyfile(master_cfg, dst)
                copied_dirs.append(root)
                break  # one copy per folder

    print("→ Copied config to:")
    for d in copied_dirs:
        print("  ", d)

# PlatformIO hook
try:
    from SCons.Script import Import
    Import("env")
    copy_internal_config(env['PROJECT_DIR'])
except ImportError:
    # Manual run
    if len(sys.argv) > 1:
        project_dir = sys.argv[1]
    else:
        project_dir = os.getcwd()
    copy_internal_config(project_dir)
