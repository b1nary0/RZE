#!/usr/bin/env python3
"""Run RZE's UnitTests (doctest) N times, reporting progress for the lane's tab.

From the worktree root, after the build (verify.md, stage 1):
    python3 /home/JPOD/dev/RZE/.claude/workflow/run_tests.py --runs 1 [-v] [--config development] [TEST ...]

Each run starts RZE/_build/<config>/UnitTests (UnitTests.exe on Windows) with --duration=true, which
prints one "<seconds> s: <name>" line as each test case finishes; those lines are counted. The
total comes from UnitTests --count. TESTs select test cases by name, as doctest's -tc does
(wildcards allowed, e.g. "*Undo*"). Progress (test cases finished out of the total, across all
runs, and the run it's on) goes through the lanes skill's test_progress.py, which the lane watch
shows on the lane's tab; outside a lane it just runs the tests. --duration=true makes doctest print a
header block (separator, file:line, "TEST CASE:") before every test case; without -v, a block is
printed only when it holds more than that (a failure, or the engine's own log lines), and the
duration lines are left out. Exit code 0 only if every run passed.

Per repository (RZE), never synced. RZGit's runner has the same interface.
"""
import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "skills" / "lanes"))
from test_progress import Progress  # noqa: E402

FINISHED = re.compile(r"^\d+(\.\d+)? s: ")  # --duration=true: one line per finished test case
HEADER = re.compile(r"^(=+|\S+:\d+:|TEST CASE:.*|\s*)$")  # doctest's per-case header lines
COUNT = re.compile(r"passing the current filters: (\d+)")


def binary(config):
    name = "UnitTests.exe" if os.name == "nt" else "UnitTests"
    path = Path("RZE", "_build", config, name)
    if not path.is_file():
        sys.exit(f"{path} isn't there: run from the worktree root, after the build (verify.md, stage 1)")
    return str(path.resolve())


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--runs", type=int, default=1, help="how many times to run the tests")
    parser.add_argument("-v", "--verbose", action="store_true", help="also print each test case's duration line")
    parser.add_argument("--config", default="development", help="the build to run (default: development)")
    parser.add_argument("tests", nargs="*", metavar="TEST",
                        help="only test cases matching these names (doctest -tc); all without any")
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("--runs must be at least 1")

    exe = binary(args.config)
    filters = [f"-tc={','.join(args.tests)}"] if args.tests else []
    counted = subprocess.run([exe, "--count", *filters], capture_output=True, text=True)
    match = COUNT.search(counted.stdout + counted.stderr)
    if not match:
        sys.exit(f"couldn't read the test count from {exe} --count")
    per_run = int(match.group(1))

    failed = []
    with Progress(total=per_run * args.runs, runs=args.runs) as progress:
        for run in range(1, args.runs + 1):
            progress.start_run(run)
            print(f"=== run {run} of {args.runs} ({per_run} test cases)", flush=True)
            proc = subprocess.Popen([exe, "--duration=true", *filters], stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, text=True, errors="replace")
            block = []  # the current test case's lines, held until we know it has more than its header
            for line in proc.stdout:
                if FINISHED.match(line):
                    progress.step()
                    if args.verbose or any(not HEADER.match(l.rstrip("\n")) for l in block):
                        sys.stdout.writelines(block)
                    if args.verbose:
                        sys.stdout.write(line)
                    block = []
                elif args.verbose:
                    sys.stdout.write(line)
                else:
                    block.append(line)
            sys.stdout.writelines(block)  # the summary after the last test case
            if proc.wait() != 0:
                failed.append(run)
    print(f"=== {args.runs - len(failed)} of {args.runs} runs passed"
          + (f"; failed: {', '.join(map(str, failed))}" if failed else ""), flush=True)
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
