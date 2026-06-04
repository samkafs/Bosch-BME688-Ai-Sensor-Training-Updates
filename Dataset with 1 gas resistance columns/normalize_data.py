import pandas as pd
import numpy as np
import os

# --- Configurations ---
workspace_dir = r"c:\Users\Samkafs\Downloads\Training data\EXPERIMENT 1"
files_to_normalize = {
    "Clean Air": "CLEAN_AIR_EXPERIMENT 1.csv",
    "Incense": "INCENSE_HIGH.csv",
    "Perfume": "PERFUME.csv"
}

print("=================================================================")
print("           BME688 SENSOR DATA NORMALIZATION PIPELINE             ")
print("=================================================================\n")

# 1. Load Clean Air data to establish baseline
clean_air_path = os.path.join(workspace_dir, files_to_normalize["Clean Air"])
if not os.path.exists(clean_air_path):
    print(f"Error: Could not find clean air file at {clean_air_path}")
    exit(1)

print("Step 1: Analyzing Clean Air reference data to establish baseline...")
df_clean_ref = pd.read_csv(clean_air_path)

# Compute R0 baseline resistance (median is more robust to outliers than mean)
R0 = float(df_clean_ref['gas_resistance'].median())
R0_mean = float(df_clean_ref['gas_resistance'].mean())

print(f"  - Clean Air median gas resistance (R0): {R0:.2f} ohms")
print(f"  - Clean Air mean gas resistance: {R0_mean:.2f} ohms")
print(f"  * Using R0 = {R0:.2f} ohms as the baseline calibration constant.")

print("\nStep 2: Processing and normalising datasets...")

# 2. Process each file
for class_name, filename in files_to_normalize.items():
    file_path = os.path.join(workspace_dir, filename)
    if not os.path.exists(file_path):
        print(f"  [Warning] File not found: {filename}. Skipping...")
        continue
    
    # Load dataset
    df = pd.read_csv(file_path)
    
    # Save original gas resistance stats for debugging
    raw_min = df['gas_resistance'].min()
    raw_max = df['gas_resistance'].max()
    raw_mean = df['gas_resistance'].mean()
    
    # Create the normalized gas resistance: R_norm = R / R0
    df['gas_resistance_ratio'] = df['gas_resistance'] / R0
    
    # We will save two versions of the normalized file to give you maximum flexibility in NanoEdge AI:
    
    # Version A: Overwrite 'gas_resistance' column directly (Perfect for NanoEdge AI matching your MCU code schema)
    df_version_a = df.copy()
    df_version_a['gas_resistance'] = df_version_a['gas_resistance_ratio']
    df_version_a = df_version_a.drop(columns=['gas_resistance_ratio'])
    
    # Output path for Version A
    basename = os.path.splitext(filename)[0]
    out_path_a = os.path.join(workspace_dir, f"{basename}_NORMALIZED.csv")
    df_version_a.to_csv(out_path_a, index=False)
    
    # Version B: Keep raw column and add 'gas_resistance_ratio' as a new column
    out_path_b = os.path.join(workspace_dir, f"{basename}_WITH_RATIO.csv")
    df.to_csv(out_path_b, index=False)
    
    # Calculate normalized stats
    norm_mean = df['gas_resistance_ratio'].mean()
    norm_median = df['gas_resistance_ratio'].median()
    
    print(f"\n  • Processed '{class_name}' ({len(df)} samples):")
    print(f"    - Raw Gas Ohms:   Min = {raw_min} | Max = {raw_max} | Mean = {raw_mean:.2f}")
    print(f"    - Normalized:     Mean Ratio = {norm_mean:.4f} | Median Ratio = {norm_median:.4f}")
    print(f"    - Saved schema matching file: {os.path.basename(out_path_a)}")
    print(f"    - Saved raw + ratio file:     {os.path.basename(out_path_b)}")

print("\n=================================================================")
print("                   MICROCONTROLLER C IMPLEMENTATION              ")
print("=================================================================")
print("To achieve the exact same behavior in your STM32 microcontroller code,")
print("you should define your baseline resistance matching this training data:")
print("-----------------------------------------------------------------")
print(f"  #define BME688_GAS_BASELINE  {R0:.1f}f  // Baseline R0 in ohms")
print("-----------------------------------------------------------------")
print("Then, in your sensor loop:")
print("  float raw_gas = (float)read_gas_ohms();")
print("  float normalized_gas = raw_gas / BME688_GAS_BASELINE;")
print("  ")
print("  // Fill the input buffer in the exact order trained:")
print("  input_signal[0] = temperature;")
print("  input_signal[1] = humidity;")
print("  input_signal[2] = pressure;")
print("  input_signal[3] = normalized_gas;  // Ratio between 0.0 and 1.5")
print("=================================================================\n")
print("Normalization pipeline complete! Import the *_NORMALIZED.csv files")
print("directly into NanoEdge AI Studio.")
