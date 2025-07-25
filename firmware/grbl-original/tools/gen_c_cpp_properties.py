import re
import os
import json
from pathlib import Path

# Paths
script_dir = Path(__file__).resolve().parent
project_root = script_dir.parent
makefile_path = project_root / "Makefile"
output_dir = project_root / ".vscode"
output_file = output_dir / "c_cpp_properties.json"

# Parse Makefile variables
variables = {}
make_var_pattern = re.compile(r"^([A-Za-z0-9_]+)\s*[:?]?=\s*(.*)$")

with open(makefile_path) as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = make_var_pattern.match(line)
        if m:
            key, val = m.groups()
            # Expand other vars inside this line (basic)
            for v, vval in variables.items():
                val = val.replace(f"$({v})", vval)
            variables[key] = val.strip()

# Extract include paths from lines with -I
include_paths = []
cflags_line_pattern = re.compile(r"^\s*CFLAGS\s*(\+?=)\s*(.*)$")

with open(makefile_path) as f:
    for line in f:
        line = line.strip()
        m = cflags_line_pattern.match(line)
        if not m:
            continue
        _, rhs = m.groups()

        # Skip comments
        rhs = rhs.split("#")[0].strip()

        # Expand variables
        for key, val in variables.items():
            rhs = rhs.replace(f"$({key})", val)

        # Extract -I paths only
        for token in rhs.split():
            if token.startswith("-I"):
                path = token[2:]
                if "$(" in path or path.endswith("))"):
                    print(f"⚠️  Skipping unresolved Makefile include: {path}")
                    continue
                include_paths.append(path)


# Normalize, deduplicate, and convert to posix-style
resolved_paths = []
for path in sorted(set(include_paths)):
    full = (project_root / path).resolve()
    if full.exists():
        rel = full.relative_to(project_root)
        resolved_paths.append(str(rel).replace("\\", "/"))
    else:
        print(f"⚠️  Warning: include path not found: {path}")

resolved_paths.append("${workspaceFolder}/**")

# Final JSON config
config = {
    "configurations": [
        {
            "name": "STM32G0B1",
            "includePath": resolved_paths,
            "defines": [],
            "compilerPath": "arm-none-eabi-gcc",
            "cStandard": "gnu11",
            "cppStandard": "gnu++17",
            "intelliSenseMode": "gcc-arm",
            "browse": {
                "limitSymbolsToIncludedHeaders": True,
                "path": resolved_paths
            }
        }
    ],
    "version": 4
}

# Write output
output_dir.mkdir(exist_ok=True)
with open(output_file, "w") as out:
    json.dump(config, out, indent=2)

print(f"✅ Generated {output_file}")
