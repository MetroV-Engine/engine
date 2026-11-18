import argparse
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent


def run(command: list[str]) -> None:
    print("+", " ".join(str(argument) for argument in command), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def require_tool(tool: str) -> None:
    if shutil.which(tool) is None:
        raise RuntimeError(f"Required tool not found in PATH: {tool}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Build MetroV Engine")
    parser.add_argument(
        "--preset",
        default="all",
        help="CMake preset (see CMakePresets.json): all (default: executables and editor), tests (adds the unit tests), runtime",
    )
    args = parser.parse_args()

    require_tool("cmake")
    require_tool("ninja")

    run([
        "git",
        "submodule",
        "update",
        "--init",
        "--depth",
        "1",
        "vcpkg",
    ])

    run(["cmake", "--preset", args.preset])
    run(["cmake", "--build", "--preset", args.preset, "--parallel"])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
    except RuntimeError as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)