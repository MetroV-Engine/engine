from pathlib import Path
import argparse
import platform
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent


def run(command: list[str]) -> None:
    print("+", " ".join(str(argument) for argument in command), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build, test, and check MetroV Engine coverage"
    )
    parser.add_argument(
        "--minimum",
        type=float,
        default=90.0,
        help="Minimum line coverage percentage (default: 90)",
    )
    args = parser.parse_args()

    if platform.system() != "Linux":
        parser.error("coverage is only supported on Linux")
    if not 0 <= args.minimum <= 100:
        parser.error("--minimum must be between 0 and 100")
    if shutil.which("gcovr") is None:
        raise RuntimeError("gcovr is required to generate the coverage report")

    if shutil.which("cmake") is None:
        raise RuntimeError("cmake is required to generate coverage")
    if shutil.which("ninja") is None:
        raise RuntimeError("ninja is required to generate coverage")

    run(["git", "submodule", "update", "--init", "--depth", "1", "vcpkg"])

    build_dir = ROOT / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)

    run([
        "cmake",
        "-S",
        ".",
        "-B",
        str(build_dir),
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DENGINE_BUILD_APP=OFF",
        "-DENGINE_BUILD_TESTS=ON",
        "-DENGINE_ENABLE_COVERAGE=ON",
    ])
    run(["cmake", "--build", str(build_dir), "--parallel"])
    run(["ctest", "--test-dir", str(build_dir), "--output-on-failure"])

    coverage_dir = ROOT / "build" / "tests" / "coverage"
    coverage_dir.mkdir(parents=True, exist_ok=True)
    run([
        "gcovr",
        "--root",
        str(ROOT),
        "--filter",
        str(ROOT / "src"),
        "--html-details",
        str(coverage_dir / "index.html"),
        "--print-summary",
        "--fail-under-line",
        str(args.minimum),
    ])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
    except RuntimeError as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)