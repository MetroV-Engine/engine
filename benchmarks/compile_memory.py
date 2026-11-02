"""Measures the peak memory and the time the compiler needs for one source file.

Run from the repository root:

    python benchmarks/compile_memory.py                      # BenchHasComponents.cpp, -O0
    python benchmarks/compile_memory.py --opt=-O3            # same file, optimised
    python benchmarks/compile_memory.py benchmarks/BenchZipperDriver.cpp

Linux and macOS only (it reads the children's peak resident memory).
"""

from pathlib import Path
import argparse
import resource
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source", nargs="?", default="benchmarks/BenchHasComponents.cpp",
                        help="file to compile (default: benchmarks/BenchHasComponents.cpp)")
    parser.add_argument("--compiler", default="c++", help="compiler to run (default: c++)")
    parser.add_argument("--opt", default="-O0", help="optimisation flag, written --opt=-O3 (default: -O0)")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmp:
        command = [args.compiler, "-std=c++20", args.opt, f"-I{ROOT / 'src'}",
                   "-c", str(ROOT / args.source), "-o", str(Path(tmp) / "out.o")]
        start = time.monotonic()
        process = subprocess.Popen(command, stderr=subprocess.PIPE, text=True)
        # A single file gives no progress, so show the elapsed time while it runs.
        while True:
            try:
                _, errors = process.communicate(timeout=1)
                break
            except subprocess.TimeoutExpired:
                if sys.stdout.isatty():
                    print(f"\rcompiling {args.source}... {time.monotonic() - start:.0f} s",
                          end="", flush=True)
        seconds = time.monotonic() - start
        if sys.stdout.isatty():
            print("\r\033[K", end="")

    if process.returncode != 0:
        print(errors, file=sys.stderr)
        print(f"compilation failed (exit code {process.returncode})", file=sys.stderr)
        return process.returncode

    peak_kib = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
    if sys.platform == "darwin":
        peak_kib //= 1024  # macOS reports bytes, Linux reports KiB
    print(f"{args.source} ({args.compiler} {args.opt}): "
          f"peak memory {peak_kib // 1024} MiB, {seconds:.1f} s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
