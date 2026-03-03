#!/usr/bin/env python3
import argparse
import concurrent.futures
import os
import re
import subprocess
import sys
import tempfile
import textwrap
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


@dataclass
class CaseResult:
    module: str
    name: str
    passed: bool
    failure_message: str
    failure_detail: str


@dataclass
class ModuleResult:
    module: str
    return_code: int
    cases: list[CaseResult]
    raw_output: str
    command_error: str


def is_memcheck_success(return_code: int, output: str) -> bool:
    if return_code != 0:
        return False

    if re.search(r"The following tests FAILED:", output, re.IGNORECASE):
        return False

    failed_count = re.search(r"(\d+) tests failed out of (\d+)", output, re.IGNORECASE)
    if failed_count and int(failed_count.group(1)) > 0:
        return False

    if re.search(r"Defects:\s*[1-9]\d*", output):
        return False

    return True


def memcheck_failure_message(output: str) -> str:
    if re.search(r"Defects:\s*[1-9]\d*", output):
        return "memcheck found memory defects"
    if re.search(r"The following tests FAILED:", output, re.IGNORECASE):
        return "ctest reported failed test"
    return "memcheck failed"


def list_tests(build_dir: str, module: str) -> list[str]:
    cmd = ["ctest", "--test-dir", build_dir, "-N", "-R", f"^{module}\\."]
    completed = subprocess.run(cmd, capture_output=True, text=True)
    output = (completed.stdout or "") + (completed.stderr or "")

    names: list[str] = []
    for line in output.splitlines():
        match = re.search(r"Test\s+#\d+:\s+(.+)$", line.strip())
        if match:
            names.append(match.group(1).strip())
    return names


class ProgressPrinter:
    def __init__(self, total: int, width: int = 80):
        self.total = total
        self.width = width
        self.current = 0
        self.use_color = os.getenv("NO_COLOR") is None
        self.green = "\033[32m" if self.use_color else ""
        self.red = "\033[31m" if self.use_color else ""
        self.reset = "\033[0m" if self.use_color else ""
        self.started = False

    def color_symbol(self, symbol: str, passed: bool) -> str:
        if passed:
            return f"{self.green}{symbol}{self.reset}"
        return f"{self.red}{symbol}{self.reset}"

    def start(self) -> None:
        if self.started:
            return
        self.started = True
        dot = self.color_symbol(".", True)
        cross = self.color_symbol("x", False)
        print(f"\nProgress ({dot}=pass, {cross}=fail):")

    def write_case(self, passed: bool) -> None:
        self.start()
        symbol = "." if passed else "x"
        token = self.color_symbol(symbol, passed)

        print(token, end="", flush=True)
        self.current += 1
        if self.current % self.width == 0 or self.current == self.total:
            print(f"  ({self.current}/{self.total})", flush=True)


def run_module(build_dir: str, module: str, xml_path: Path, mode: str) -> ModuleResult:
    if mode == "memcheck":
        test_names = list_tests(build_dir, module)
        if not test_names:
            return ModuleResult(
                module=module,
                return_code=2,
                cases=[],
                raw_output="",
                command_error=f"No tests discovered for {module}",
            )

        cases: list[CaseResult] = []
        outputs: list[str] = []
        module_rc = 0

        for test_name in test_names:
            cmd = [
                "ctest",
                "--test-dir",
                build_dir,
                "-R",
                f"^{re.escape(test_name)}$",
                "-j",
                "1",
                "--output-on-failure",
                "-T",
                "memcheck",
            ]
            completed = subprocess.run(cmd, capture_output=True, text=True)
            output = (completed.stdout or "") + (completed.stderr or "")
            outputs.append(output)

            passed = is_memcheck_success(completed.returncode, output)
            if not passed:
                module_rc = completed.returncode

            cases.append(
                CaseResult(
                    module=module,
                    name=test_name,
                    passed=passed,
                    failure_message=memcheck_failure_message(output) if not passed else "",
                    failure_detail=output.strip() if not passed else "",
                )
            )

        return ModuleResult(
            module=module,
            return_code=module_rc,
            cases=cases,
            raw_output="\n".join(outputs),
            command_error="",
        )

    cmd = [
        "ctest",
        "--test-dir",
        build_dir,
        "-R",
        f"^{module}\\.",
        "-j",
        "1",
        "--output-on-failure",
        "--output-junit",
        str(xml_path),
    ]

    completed = subprocess.run(cmd, capture_output=True, text=True)
    output = (completed.stdout or "") + (completed.stderr or "")

    cases: list[CaseResult] = []
    command_error = ""

    if xml_path.exists():
        try:
            root = ET.parse(xml_path).getroot()
            for testcase in root.iter("testcase"):
                name = testcase.attrib.get("name", "<unknown>")
                failure_node = testcase.find("failure")
                if failure_node is None:
                    cases.append(
                        CaseResult(
                            module=module,
                            name=name,
                            passed=True,
                            failure_message="",
                            failure_detail="",
                        )
                    )
                else:
                    cases.append(
                        CaseResult(
                            module=module,
                            name=name,
                            passed=False,
                            failure_message=failure_node.attrib.get("message", "test failed").strip(),
                            failure_detail=(failure_node.text or "").strip(),
                        )
                    )
        except ET.ParseError as exc:
            command_error = f"Failed to parse junit xml for {module}: {exc}"
    else:
        command_error = f"No junit xml generated for {module}"

    if not cases and not command_error:
        command_error = f"No test cases discovered in junit xml for {module}"

    return ModuleResult(
        module=module,
        return_code=completed.returncode,
        cases=cases,
        raw_output=output,
        command_error=command_error,
    )


def print_progress(cases: list[CaseResult], width: int = 50) -> None:
    progress = ProgressPrinter(total=max(len(cases), 1), width=width)
    dot = progress.color_symbol(".", True)
    cross = progress.color_symbol("x", False)
    print(f"\nProgress ({dot}=pass, {cross}=fail):")
    symbols = [progress.color_symbol(".", True) if case.passed else progress.color_symbol("x", False) for case in cases]
    if not symbols:
        print("(no tests)")
        return

    for index in range(0, len(symbols), width):
        chunk = symbols[index : index + width]
        right = min(index + width, len(symbols))
        print(f"{' '.join(chunk)}  ({right}/{len(symbols)})")


def count_tests(build_dir: str, module: str) -> int:
    cmd = ["ctest", "--test-dir", build_dir, "-N", "-R", f"^{module}\\."]
    completed = subprocess.run(cmd, capture_output=True, text=True)
    output = (completed.stdout or "") + (completed.stderr or "")
    match = re.search(r"Total Tests:\s*(\d+)", output)
    if match:
        return int(match.group(1))
    return 0


def shorten(text: str, limit: int = 600) -> str:
    if len(text) <= limit:
        return text
    return text[:limit].rstrip() + "\n... (truncated)"


def maybe_shorten(text: str, full_detail: bool, limit: int = 600) -> str:
    if full_detail:
        return text
    return shorten(text, limit=limit)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run ctest modules in parallel with concise progress output")
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--mode", choices=["normal", "memcheck"], default="normal")
    parser.add_argument("--full-detail", action="store_true", help="Print full failure and module error output without truncation")
    parser.add_argument("modules", nargs="+")
    args = parser.parse_args()

    jobs = max(1, args.jobs)
    modules = args.modules

    totals = {module: count_tests(args.build_dir, module) for module in modules}
    total_expected = sum(totals.values())

    print(
        f"Running {len(modules)} module(s) with {jobs} parallel worker(s), expected tests={total_expected}..."
    )

    progress = ProgressPrinter(total=max(total_expected, 1))

    with tempfile.TemporaryDirectory(prefix="ctest-pretty-") as tmp_dir:
        tmp_path = Path(tmp_dir)

        futures: dict[concurrent.futures.Future[ModuleResult], str] = {}
        results_by_module: dict[str, ModuleResult] = {}

        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as executor:
            for module in modules:
                xml_file = tmp_path / f"{module}.xml"
                future = executor.submit(run_module, args.build_dir, module, xml_file, args.mode)
                futures[future] = module

            for future in concurrent.futures.as_completed(futures):
                result = future.result()
                results_by_module[result.module] = result
                for case in result.cases:
                    progress.write_case(case.passed)

    ordered_results = [results_by_module[m] for m in modules]
    all_cases = [case for result in ordered_results for case in result.cases]
    failed_cases = [case for case in all_cases if not case.passed]
    module_errors = [result for result in ordered_results if result.command_error]

    if total_expected == 0:
        print("\nProgress (.=pass, x=fail):")
        print("(no tests)")
    elif progress.current != total_expected:
        print(f"\nProgress count mismatch: printed={progress.current}, expected={total_expected}")
    print(
        f"\nSummary: total={len(all_cases)} passed={len(all_cases) - len(failed_cases)} failed={len(failed_cases)}"
    )

    if module_errors:
        print("\nModule execution errors:")
        for result in module_errors:
            print(f"- {result.module}: {result.command_error}")
            if result.raw_output.strip():
                print(textwrap.indent(maybe_shorten(result.raw_output.strip(), args.full_detail), "  "))

    if failed_cases:
        print("\nFailed tests:")
        grouped: dict[str, list[CaseResult]] = {}
        for case in failed_cases:
            grouped.setdefault(case.module, []).append(case)

        for module in modules:
            cases = grouped.get(module, [])
            if not cases:
                continue
            print(f"- {module}")
            for case in cases:
                print(f"  • {case.name}")
                if case.failure_message:
                    print(f"    message: {case.failure_message}")
                if case.failure_detail:
                    detail = re.sub(r"\n{3,}", "\n\n", case.failure_detail.strip())
                    print(textwrap.indent(maybe_shorten(detail, args.full_detail), "    "))

        return 1

    if module_errors:
        return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())