import re
import os
import json
from pathlib import Path

# ---------- helpers ----------


def expand_foreach(expr, variables):
    """
    Recursively expand all $(foreach var,list,template) using values from `variables`.
    Supports nested $(foreach ...) and $(VAR) tokens inside list/template.
    """
    foreach_pattern = re.compile(
        r"\$\(\s*foreach\s+(\w+)\s*,\s*((?:[^()$]|\$\([^()]*\))+?)\s*,\s*((?:[^()$]|\$\([^()]*\))+?)\s*\)"
    )

    def eval_expr(e):
        # shallow $(VAR) replacements first
        e2 = e
        for key, val in variables.items():
            e2 = e2.replace(f"$({key})", val)
        # then recurse for nested foreach
        return expand_foreach(e2, variables)

    while True:
        m = foreach_pattern.search(expr)
        if not m:
            break
        var, list_expr, template = m.groups()
        list_expanded = eval_expr(list_expr).strip().split()
        template_expanded = eval_expr(template)
        substituted = " ".join(template_expanded.replace(
            f"$({var})", item) for item in list_expanded)
        expr = expr[:m.start()] + substituted + expr[m.end():]
    return expr


def parse_make_vars(makefile_path: Path, board_override: str | None):
    """
    Parse simple VAR :=/=?= lines from the Makefile (no shell eval),
    doing one-pass substitutions with already-known vars.
    If board_override is provided, it *forces* BOARD_NAME to that value.
    """
    variables: dict[str, str] = {}
    if board_override:
        variables["BOARD_NAME"] = board_override  # preseed

    make_var_pattern = re.compile(r"^([A-Za-z0-9_]+)\s*[:?]?=\s*(.*)$")

    with open(makefile_path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            m = make_var_pattern.match(line)
            if not m:
                continue

            key, val = m.groups()
            val = val.strip()

            # If we’re forcing a board, ignore Makefile’s BOARD_NAME assignment
            if key == "BOARD_NAME" and board_override:
                # keep the forced one
                continue

            # one-pass $(VAR) expansions using what we’ve seen so far (including forced BOARD_NAME)
            for k2, v2 in variables.items():
                val = val.replace(f"$({k2})", v2)

            variables[key] = val

    # Ensure we have BOARD_NAME set
    if board_override and variables.get("BOARD_NAME") != board_override:
        variables["BOARD_NAME"] = board_override

    return variables


def extract_include_paths(makefile_path: Path, variables: dict[str, str]) -> tuple[str, list[str]]:
    """
    Collect -I paths from all CFLAGS lines after expanding $(VARS) and $(foreach ...).
    Returns (board_name, include_paths).
    """
    cflags_line_pattern = re.compile(r"^\s*CFLAGS\s*(\+?=)\s*(.*)$")

    include_paths: list[str] = []

    with open(makefile_path, "r", encoding="utf-8") as f:
        for lineno, raw in enumerate(f, start=1):
            line = raw.strip()
            m = cflags_line_pattern.match(line)
            if not m:
                continue

            _, rhs = m.groups()
            rhs = rhs.split("#")[0].strip()  # strip comments

            # Expand $(VAR) tokens using our variables snapshot
            for key, val in variables.items():
                rhs = rhs.replace(f"$({key})", val)

            # Expand $(foreach ...) recursively
            rhs = expand_foreach(rhs, variables)

            # Capture only -I tokens
            for token in rhs.split():
                if token.startswith("-I"):
                    path = token[2:]
                    if "$(" in path or path.endswith("))"):
                        print(
                            f"⚠️  Skipping unresolved Makefile include: {path} [line: {lineno}]")
                        continue
                    include_paths.append(path)

    board_name = variables.get("BOARD_NAME", "")
    if not board_name:
        print("⚠️  Warning: BOARD_NAME not resolved")

    return board_name, include_paths


def normalize_paths(project_root: Path, paths: list[str]) -> list[str]:
    """Normalize, dedupe, make relative to workspace, POSIX-style, and keep only existing."""
    seen = set()
    out: list[str] = []
    for p in sorted(set(paths)):
        full = (project_root / p).resolve()
        if full.exists():
            rel = full.relative_to(project_root)
            s = str(rel).replace("\\", "/")
            if s not in seen:
                seen.add(s)
                out.append(s)
        else:
            print(f"⚠️  Warning: include path not found: {p}")
    out.append("${workspaceFolder}/**")
    return out


def discover_boards(project_root: Path) -> list[str]:
    """Return all board names as folder names under src/boards/*."""
    boards_dir = project_root / "src" / "boards"
    if not boards_dir.is_dir():
        return []
    names = [p.name for p in boards_dir.iterdir() if p.is_dir()]
    names.sort()
    return names


# ---------- main ----------

script_dir = Path(__file__).resolve().parent
project_root = script_dir.parent
makefile_path = project_root / "Makefile"
output_dir = project_root / ".vscode"
output_file = output_dir / "c_cpp_properties.json"

boards = discover_boards(project_root)
if not boards:
    print("⚠️  No boards found under src/boards/* — generating a single config from current Makefile defaults.")
    boards = [None]  # will use whatever BOARD_NAME resolves to in the Makefile

configs = []

for board in boards:
    vars_for_board = parse_make_vars(makefile_path, board_override=board)
    board_name, incs = extract_include_paths(makefile_path, vars_for_board)
    include_paths = normalize_paths(project_root, incs)

    # Friendly config name
    cfg_name = board_name if board_name else "default"

    configs.append({
        "name": cfg_name,
        "includePath": include_paths,
        "defines": [],
        "compilerPath": "arm-none-eabi-gcc",
        "cStandard": "gnu11",
        "cppStandard": "gnu++17",
        "intelliSenseMode": "gcc-arm",
        "browse": {
            "limitSymbolsToIncludedHeaders": True,
            "path": include_paths
        }
    })

output_dir.mkdir(exist_ok=True)
with open(output_file, "w", encoding="utf-8") as out:
    json.dump({"configurations": configs, "version": 4}, out, indent=2)

names = ", ".join(cfg["name"] for cfg in configs)
print(f"✅ Generated {output_file} with configurations: {names}")
