"""A lane's test-run progress, for the lane watch (lanes skill, "Test runs").

A repository's test runner (named in its repo.md "Test runs" row) imports this and reports as it
goes; the watch shows it as the lane tab's progress bar and its "<ID> · <run>/<runs> · <percent>%"
label. Outside a lane (no LANE_DIR / LANE_NAME) it does nothing, so a runner works anywhere.

    sys.path.insert(0, str(<repo>/.claude/skills/lanes))
    from test_progress import Progress

    with Progress(total=tests_per_run * runs, runs=runs) as progress:
        for run in range(1, runs + 1):
            progress.start_run(run)
            ... progress.step() as each test finishes ...

The file is <LANE_DIR>/progress/<LANE_NAME>.json: {"done", "total", "run", "runs", "pid"}, written
at most every 0.25 s (and at each run's start and at the end), and removed when the block ends.
"""
import json
import os
import time
from pathlib import Path


class Progress:
    def __init__(self, total, runs=1):
        self.done, self.total, self.run, self.runs, self._last = 0, total, 1, runs, 0.0
        lane_dir, lane = os.environ.get("LANE_DIR"), os.environ.get("LANE_NAME")
        self.path = Path(lane_dir, "progress", f"{lane}.json") if lane_dir and lane else None

    def __enter__(self):
        if self.path:
            self.path.parent.mkdir(parents=True, exist_ok=True)
        self.write(force=True)
        return self

    def __exit__(self, *exc):
        self.write(force=True)
        self.clear()
        return False

    def start_run(self, run):
        self.run = run
        self.write(force=True)

    def step(self, count=1):
        self.done += count
        self.write()

    def write(self, force=False):
        now = time.monotonic()
        if self.path is None or (not force and now - self._last < 0.25):
            return
        self._last = now
        tmp = self.path.with_suffix(".tmp")
        tmp.write_text(json.dumps({"done": self.done, "total": self.total, "run": self.run,
                                   "runs": self.runs, "pid": os.getpid()}))
        os.replace(tmp, self.path)

    def clear(self):
        if self.path:
            try:
                self.path.unlink()
            except FileNotFoundError:
                pass
