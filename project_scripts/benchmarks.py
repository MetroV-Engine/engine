"""Runs the benchmarks, compares them to a baseline and reports the result.

Run from the repository root, after a Release build with
ENGINE_BUILD_BENCHMARKS=ON (see benchmarks/README.md):

    python project_scripts/benchmarks.py run --build-dir build/bench --out bench-results
    python project_scripts/benchmarks.py compare --current bench-results --baseline baseline

`run` runs every bench_* executable of the build directory and writes one JSON
file per benchmark. `compare` prints a Markdown table of the changes against
the baseline; with --discord it also posts a short digest to the webhook in the
DISCORD_WEBHOOK environment variable. Regressions are reported, never fatal:
shared CI runners are too noisy to fail a build on timings.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import json
import os
import subprocess
import sys
import urllib.error
import urllib.request

# A scenario is flagged only when it is both this much slower in percent and
# slower by at least MIN_DELTA_US: sub-microsecond scenarios jump by tens of
# percent from noise alone.
DEFAULT_THRESHOLD = 20.0
MIN_DELTA_US = 1.0


@dataclass
class Change:
    benchmark: str
    scenario: str
    baseline_us: float | None
    current_us: float | None

    @property
    def percent(self) -> float | None:
        if self.baseline_us is None or self.current_us is None or self.baseline_us <= 0:
            return None
        return (self.current_us - self.baseline_us) / self.baseline_us * 100.0

    def is_regression(self, threshold: float) -> bool:
        percent = self.percent
        return (percent is not None and percent >= threshold
                and self.current_us - self.baseline_us >= MIN_DELTA_US)

    def is_improvement(self, threshold: float) -> bool:
        percent = self.percent
        return (percent is not None and percent <= -threshold
                and self.baseline_us - self.current_us >= MIN_DELTA_US)


# --- run ---------------------------------------------------------------------

def find_benchmarks(build_dir: Path) -> list[Path]:
    found = []
    for path in sorted(build_dir.glob("bench_*")):
        if path.is_file() and (path.suffix == ".exe" or os.access(path, os.X_OK)):
            if path.suffix in ("", ".exe"):
                found.append(path)
    return found


def run(args: argparse.Namespace) -> int:
    build_dir = Path(args.build_dir)
    benchmarks = find_benchmarks(build_dir)
    if not benchmarks:
        print(f"no bench_* executable in {build_dir}; build with "
              "-DENGINE_BUILD_BENCHMARKS=ON first", file=sys.stderr)
        return 1

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    for executable in benchmarks:
        name = executable.stem
        print(f"=== {name}", flush=True)
        result = subprocess.run([str(executable), str(args.repetitions),
                                 "--json", str(out / f"{name}.json")])
        if result.returncode != 0:
            print(f"{name} failed with exit code {result.returncode}", file=sys.stderr)
            return result.returncode
    print(f"results written to {out}/")
    return 0


# --- compare -----------------------------------------------------------------

def load_results(directory: Path) -> dict[tuple[str, str], float]:
    """Maps (benchmark, scenario) to its median in microseconds."""
    medians = {}
    for path in sorted(directory.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        for entry in data["results"]:
            medians[(data["benchmark"], entry["name"])] = float(entry["median_us"])
    return medians


def compute_changes(current: dict, baseline: dict) -> list[Change]:
    keys = list(current) + [key for key in baseline if key not in current]
    return [Change(benchmark, scenario, baseline.get((benchmark, scenario)),
                   current.get((benchmark, scenario)))
            for benchmark, scenario in keys]


def format_us(value: float | None) -> str:
    return "—" if value is None else f"{value:,.2f}"


def format_percent(change: Change) -> str:
    if change.current_us is None:
        return "removed"
    if change.baseline_us is None:
        return "new"
    return f"{change.percent:+.1f}%"


def markdown_report(changes: list[Change], threshold: float, has_baseline: bool) -> str:
    regressions = [c for c in changes if c.is_regression(threshold)]
    lines = ["## Benchmarks", ""]
    if not has_baseline:
        lines += ["No baseline yet (no successful run on `dev`): showing raw results.", ""]
    elif regressions:
        lines += [f"⚠️ {len(regressions)} possible regression(s) above {threshold:g}% "
                  "(alert only, CI runners are noisy).", ""]
    else:
        lines += [f"✅ No regression above {threshold:g}%.", ""]

    lines += ["| Benchmark | Scenario | Baseline median (µs) | Median (µs) | Change |",
              "| --- | --- | ---: | ---: | ---: |"]
    for change in changes:
        flag = ""
        if change.is_regression(threshold):
            flag = " ⚠️"
        elif change.is_improvement(threshold):
            flag = " 🚀"
        lines.append(f"| {change.benchmark} | {change.scenario} | "
                     f"{format_us(change.baseline_us)} | {format_us(change.current_us)} | "
                     f"{format_percent(change)}{flag} |")
    return "\n".join(lines) + "\n"


def discord_payload(changes: list[Change], threshold: float, has_baseline: bool,
                    branch: str, commit: str, run_url: str) -> dict:
    regressions = [c for c in changes if c.is_regression(threshold)]
    if not has_baseline:
        status, color = "No baseline yet: first results recorded.", 0x95A5A6
    elif regressions:
        status, color = f"⚠️ {len(regressions)} possible regression(s) above {threshold:g}%", 0xE67E22
    else:
        status, color = f"✅ No regression above {threshold:g}%", 0x2ECC71

    # The three largest changes, in either direction; not every scenario.
    compared = sorted((c for c in changes if c.percent is not None),
                      key=lambda c: abs(c.percent), reverse=True)[:3]
    lines = [f"`{format_percent(c)}` {c.benchmark} · {c.scenario} "
             f"({format_us(c.baseline_us)} → {format_us(c.current_us)} µs)"
             for c in compared]

    description = status
    if lines:
        description += "\n\n**Largest changes**\n" + "\n".join(lines)
    embed = {
        "title": f"Benchmarks · {branch}",
        "description": description[:4000],
        "color": color,
        "fields": [{"name": "Commit", "value": f"`{commit[:7]}`", "inline": True}],
    }
    if run_url:
        embed["url"] = run_url
        embed["fields"].append({"name": "Run", "value": f"[details]({run_url})", "inline": True})
    return {"username": "Benchmarks", "embeds": [embed]}


def post_discord(webhook: str, payload: dict) -> bool:
    request = urllib.request.Request(
        webhook, data=json.dumps(payload).encode("utf-8"), method="POST",
        # Discord rejects urllib's default user agent.
        headers={"Content-Type": "application/json", "User-Agent": "bench-report/1.0"})
    try:
        with urllib.request.urlopen(request, timeout=15):
            return True
    except urllib.error.URLError as error:
        # The webhook URL is a secret: never print it.
        print(f"Discord post failed: {error}", file=sys.stderr)
        return False


def compare(args: argparse.Namespace) -> int:
    current = load_results(Path(args.current))
    if not current:
        print(f"no results in {args.current}", file=sys.stderr)
        return 1
    baseline_dir = Path(args.baseline) if args.baseline else None
    baseline = load_results(baseline_dir) if baseline_dir and baseline_dir.is_dir() else {}
    has_baseline = bool(baseline)

    changes = compute_changes(current, baseline)
    report = markdown_report(changes, args.threshold, has_baseline)
    print(report)
    if args.markdown:
        with open(args.markdown, "a", encoding="utf-8") as file:
            file.write(report)

    if args.discord:
        webhook = os.environ.get("DISCORD_WEBHOOK", "")
        if not webhook:
            print("DISCORD_WEBHOOK is not set: skipping the Discord report")
        else:
            payload = discord_payload(changes, args.threshold, has_baseline,
                                      args.branch, args.commit, args.run_url)
            if post_discord(webhook, payload):
                print("Discord report posted")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)

    run_parser = commands.add_parser("run", help="run every benchmark and write JSON results")
    run_parser.add_argument("--build-dir", default="build/bench",
                            help="directory holding the bench_* executables (default: build/bench)")
    run_parser.add_argument("--out", default="bench-results",
                            help="directory for the JSON results (default: bench-results)")
    run_parser.add_argument("--repetitions", type=int, default=200,
                            help="repetitions per scenario (default: 200)")
    run_parser.set_defaults(func=run)

    compare_parser = commands.add_parser("compare", help="compare results to a baseline")
    compare_parser.add_argument("--current", default="bench-results",
                                help="directory with the new JSON results")
    compare_parser.add_argument("--baseline", help="directory with the baseline JSON results")
    compare_parser.add_argument("--threshold", type=float, default=DEFAULT_THRESHOLD,
                                help=f"regression threshold in percent (default: {DEFAULT_THRESHOLD:g})")
    compare_parser.add_argument("--markdown", help="also append the report to this file "
                                "(CI passes $GITHUB_STEP_SUMMARY)")
    compare_parser.add_argument("--discord", action="store_true",
                                help="post a digest to the DISCORD_WEBHOOK environment variable")
    compare_parser.add_argument("--branch", default=os.environ.get("GITHUB_REF_NAME", "local"))
    compare_parser.add_argument("--commit", default=os.environ.get("GITHUB_SHA", "unknown"))
    compare_parser.add_argument("--run-url", default="", help="link to the CI run")
    compare_parser.set_defaults(func=compare)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
