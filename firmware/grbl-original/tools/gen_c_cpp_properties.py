import re
import os
import json
from pathlib import Path


def expand_foreach(expr, variables):
    """Recursively expand all $(foreach ...) expressions in a string."""
    foreach_pattern = re.compile(
        r"\$\(\s*foreach\s+(\w+)\s*,\s*((?:[^()$]|\$\([^()]*\))+?)\s*,\s*((?:[^()$]|\$\([^()]*\))+?)\s*\)"
    )

    def eval_expr(e):
        # Expand variables in expression
        for key, val in variables.items():
            e = e.replace(f"$({key})", val)
        return expand_foreach(e, variables)  # Recurse

    while True:
        m = foreach_pattern.search(expr)
        if not m:
            break
        var, list_expr, template = m.groups()

        list_expanded = eval_expr(list_expr).strip().split()
        template_expanded = eval_expr(template)

        substituted = ' '.join(template_expanded.replace(
            f"$({var})", item) for item in list_expanded)
        expr = expr[:m.start()] + substituted + expr[m.end():]

    return expr


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
    for lineno, line in enumerate(f, start=1):
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

        # Expand foreach
        rhs = expand_foreach(rhs, variables)

        # Extract -I paths only
        for token in rhs.split():
            if token.startswith("-I"):
                path = token[2:]
                if "$(" in path or path.endswith("))"):
                    print(
                        f"⚠️  Skipping unresolved Makefile include: {path} [line: {lineno}]")
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
