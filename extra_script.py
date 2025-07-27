# extra_script.py
Import("env")
import os

# --- Define the files that our script generates ---
GENERATED_FILES = [
    os.path.join("generated_config", "HMI_Slave.xml"),
    os.path.join("generated_config", "hmi.hal"),
    os.path.join("generated_config", "mapping_doku.md")
]

# --- Define the actions for our custom targets ---

# This is the action to run the main generator script
def generate_files_action(source, target, env):
    print("--- Running custom target to generate helper files... ---")
    env.Execute("python scripts/generate_mapping.py")

# This is the action to clean the generated files
def clean_files_action(source, target, env):
    print("--- Cleaning previously generated helper files... ---")
    for f in GENERATED_FILES:
        if os.path.exists(f):
            os.remove(f)
            print(f"Removed {f}")

# --- Register the custom targets with PlatformIO ---

# Create the custom targets and get their Node objects
generate_target = env.Alias("generate_files", None, generate_files_action)
clean_target = env.Alias("clean_files", None, clean_files_action)

# --- Force the targets to always run ---
# This tells the SCons build system to ignore its dependency cache for these targets
# and always execute their associated actions when called.
env.AlwaysBuild(generate_target)
env.AlwaysBuild(clean_target)