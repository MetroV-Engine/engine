import argparse
from pathlib import Path
import platform
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


def compiler_options() -> list[str]:
    # On Windows, vcpkg builds the dependencies with MSVC (x64-windows
    # triplet), so the project must use MSVC too: with Ninja, CMake would take
    # the first compiler in PATH, often a MinGW g++ that cannot link them.
    if platform.system() != "Windows":
        return []
    if shutil.which("cl") is None:
        raise RuntimeError(
            "MSVC (cl) not found in PATH: run this script from a "
            "\"Developer PowerShell for VS\" or an \"x64 Native Tools\" prompt"
        )
    return ["-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl"]


def main() -> None:
    parser = argparse.ArgumentParser(description="Build MetroV Engine")
    parser.add_argument(
        "--compile-tests",
        action="store_true",
        help=argparse.SUPPRESS,
    )
    parser.add_argument(
        "--gui",
        action="store_true",
        help="Also build the editor GUI (src/Editor)",
    )
    args = parser.parse_args()

    require_tool("cmake")
    require_tool("ninja")
    compilers = compiler_options()

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
        f"-DENGINE_BUILD_GUI={'ON' if args.gui else 'OFF'}",
        "-DENGINE_ENABLE_COVERAGE=OFF",
        *compilers,
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