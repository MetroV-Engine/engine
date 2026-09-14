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


def require_tool(tool: str) -> None:
    if shutil.which(tool) is None:
        raise RuntimeError(f"Required tool not found in PATH: {tool}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Build MetroV Engine")
    parser.add_argument("--tests", action="store_true", help="Build unit tests")
    parser.add_argument(
        "--no-test-run",
        action="store_true",
        help="Build tests without running them",
    )
    parser.add_argument(
        "--coverage",
        action="store_true",
        help="Run tests and generate coverage on Linux",
    )
    args = parser.parse_args()

    if args.no_test_run and not args.tests:
        parser.error("--no-test-run requires --tests")
    if args.coverage and not args.tests:
        parser.error("--coverage requires --tests")
    if args.coverage and platform.system() != "Linux":
        parser.error("--coverage is only supported on Linux")

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
        f"-DENGINE_BUILD_TESTS={'ON' if args.tests else 'OFF'}",
        f"-DENGINE_ENABLE_COVERAGE={'ON' if args.coverage else 'OFF'}",
    ])
    run(["cmake", "--build", str(build_dir), "--parallel"])

    if args.tests and not args.no_test_run:
        run(["ctest", "--test-dir", str(build_dir), "--output-on-failure"])

    if args.coverage:
        require_tool("gcovr")
        coverage_dir = build_dir / "tests" / "coverage"
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
        ])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
    except RuntimeError as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)