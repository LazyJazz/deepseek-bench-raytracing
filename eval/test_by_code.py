#!/usr/bin/env python3
"""Run from eval/ and write resolved, score, reason to code_result.json."""
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


class EvaluationError(RuntimeError):
    pass


def write_result(path, result):
    if set(result) != {"resolved", "score", "reason"}:
        raise EvaluationError("invalid result fields")
    temporary = None
    try:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent,
                                         prefix=".code-result-", delete=False) as stream:
            temporary = stream.name
            json.dump(result, stream, ensure_ascii=False, allow_nan=False, indent=2)
            stream.write("\n")
        os.replace(temporary, path)
    finally:
        if temporary and os.path.exists(temporary):
            os.unlink(temporary)


def stop(process):
    if os.name == "posix":
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    else:
        process.kill()
    process.wait()


def run(command, cwd, log_path, timeout):
    with log_path.open("w", encoding="utf-8") as log:
        log.write("Command: " + json.dumps(command) + "\n")
        log.flush()
        process = subprocess.Popen(command, cwd=str(cwd), stdin=subprocess.DEVNULL,
                                   stdout=log, stderr=subprocess.STDOUT,
                                   start_new_session=os.name == "posix")
        try:
            return process.wait(timeout=timeout)
        except subprocess.TimeoutExpired as error:
            stop(process)
            raise EvaluationError("timed out after {} seconds".format(timeout)) from error


def ctest_statuses(build_dir, expected):
    tag = (build_dir / "Testing" / "TAG").read_text(encoding="utf-8").splitlines()[0]
    if not re.fullmatch(r"[0-9]+-[0-9]+", tag):
        raise EvaluationError("invalid CTest tag")
    root = ET.parse(build_dir / "Testing" / tag / "Test.xml").getroot()
    statuses = {}
    for test in root.findall("./Testing/Test"):
        name = test.findtext("Name")
        if name not in expected or name in statuses:
            raise EvaluationError("unexpected CTest result: {}".format(name))
        statuses[name] = test.get("Status") == "passed"
    if set(statuses) != set(expected):
        raise EvaluationError("incomplete CTest results")
    return statuses


def evaluate(eval_dir, workspace_dir, data_dir, config):
    logs = eval_dir / "task-1-raytracing"
    logs.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="build-raytracing-", dir=eval_dir) as temporary:
        root_build = Path(temporary)
        workspace_build = root_build / "workspace"
        test_build = root_build / "tests"
        code = run(["cmake", "-S", str(workspace_dir), "-B", str(workspace_build),
                    "-DCMAKE_BUILD_TYPE=Release"],
                   workspace_dir, logs / "configure-workspace.log", config["configure_timeout_seconds"])
        if code:
            raise EvaluationError("workspace configure failed (see eval/task-1-raytracing/configure-workspace.log)")
        code = run(["cmake", "--build", str(workspace_build), "--config", "Release", "--parallel", "2"],
                   workspace_dir, logs / "build-workspace.log", config["build_timeout_seconds"])
        if code:
            raise EvaluationError("workspace build failed (see eval/task-1-raytracing/build-workspace.log)")
        candidate = workspace_build / "raytracer_submission"
        code = run(["cmake", "-S", str(data_dir), "-B", str(test_build),
                    "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=ON",
                    "-DCANDIDATE_BINARY=" + str(candidate)],
                   data_dir, logs / "configure-tests.log", config["configure_timeout_seconds"])
        if code:
            raise EvaluationError("test configure failed (see eval/task-1-raytracing/configure-tests.log)")
        code = run(["cmake", "--build", str(test_build), "--config", "Release", "--parallel", "2"],
                   data_dir, logs / "build-tests.log", config["build_timeout_seconds"])
        if code:
            raise EvaluationError("test build failed (see eval/task-1-raytracing/build-tests.log)")
        code = run(["ctest", "--test-dir", str(test_build), "-C", "Release", "-T", "Test",
                    "--no-compress-output", "--output-on-failure", "--parallel", "1"],
                   data_dir, logs / "test.log", len(config["groups"]) * 130)
        if code not in (0, 8):
            raise EvaluationError("CTest failed with code {}".format(code))
        statuses = ctest_statuses(test_build, config["groups"])
        if (code == 0) != all(statuses.values()):
            raise EvaluationError("CTest status disagreement")
        return statuses


def main():
    eval_dir = Path.cwd().resolve()
    data_dir = eval_dir.parent / "test_files" / "data"
    workspace_dir = eval_dir.parent / "workspace"
    result_path = eval_dir / "code_result.json"
    result = {"resolved": False, "score": 0.0, "reason": "evaluation did not complete"}
    try:
        write_result(result_path, result)
        with (data_dir / "grading.json").open(encoding="utf-8") as stream:
            config = json.load(stream)
        weights = config["groups"]
        if not weights or any(type(value) is not int or value <= 0 for value in weights.values()):
            raise EvaluationError("invalid group weights")
        threshold = config["resolved_threshold"]
        if isinstance(threshold, bool) or not isinstance(threshold, (int, float)) or not 0 < threshold <= 1:
            raise EvaluationError("invalid resolved threshold")
        statuses = evaluate(eval_dir, workspace_dir, data_dir, config)
        score = round(sum(weights[name] for name, passed in statuses.items() if passed) /
                      sum(weights.values()), 6)
        failed = [name for name, passed in statuses.items() if not passed]
        result = {"resolved": score >= threshold, "score": score,
                  "reason": "all reference images matched" if not failed else
                            "failed image comparisons: " + ", ".join(failed)}
    except KeyboardInterrupt:
        result = {"resolved": False, "score": 0.0, "reason": "evaluation interrupted"}
    except Exception as error:
        result = {"resolved": False, "score": 0.0,
                  "reason": "evaluation error: {}: {}".format(type(error).__name__, error)}
    write_result(result_path, result)
    print(json.dumps(result, ensure_ascii=False, allow_nan=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
