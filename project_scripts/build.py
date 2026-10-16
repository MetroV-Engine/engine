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
        "--compile-tests",
        action="store_true",
        help=argparse.SUPPRESS,
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

    build_dir = ROOT / "build"

    run([
        "cmake",
        "-S",
        ".",
        "-B",
        str(build_dir),
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=Debug",
        f"-DENGINE_BUILD_TESTS={'ON' if args.compile_tests else 'OFF'}",
        "-DENGINE_ENABLE_COVERAGE=OFF",
    ])
    run(["cmake", "--build", str(build_dir), "--parallel"])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
    except RuntimeError as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)