#!/usr/bin/env python3
"""Cube Othello — Coverage Report Generator (E2E Tests).

This script generates coverage reports for both:
1. C++ unit tests via Google Test's built-in test runner with lcov
2. Python E2E tests via pytest-cov

Usage:
    pip install pytest pytest-cov
    ctest -C Debug --output-on-failure   # (CMake 経由で gtest を実行)
    pytest --cov=cubo_othello --cov-report=html:build/coverage/htmlcov e2e_tests.py
"""

import subprocess
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).parent.parent / 'cubo_othello'
BUILD_DIR = PROJECT_ROOT / 'build'
HTML_REPORT_DIR = BUILD_DIR / 'coverage' / 'htmlcov'
TERMINAL_REPORT_FILE = BUILD_DIR / 'coverage.txt'


def run_coverage_report() -> None:
    """Generate HTML coverage report for E2E tests."""

    html_report_dir = HTML_REPORT_DIR

    if not html_report_dir.exists():
        html_report_dir.mkdir(parents=True)

    subprocess.run(
        [sys.executable, '-m', 'pytest', '--cov=cubo_othello',
         '--cov-report=html:' + str(html_report_dir),
         PROJECT_ROOT / 'tests' / 'e2e_tests.py'],
        check=False,   # エラーを抑制してレポート生成のみ実行
    )

    print(f"HTML coverage report available at: {html_report_dir.absolute()}")


def run_unit_tests() -> None:
    """Run C++ unit tests via Google Test (if GTest is linked)."""
    subprocess.run(
        ['ctest', '-C', 'Debug', '--output-on-failure'],
        cwd=PROJECT_ROOT / 'build', check=False,
    )


def main() -> int:
    print("=" * 60)
    print("Cube Othello — Coverage Report Generator")
    print("=" * 60)

    # Step 1: Generate HTML coverage for E2E tests
    run_coverage_report()

    # Step 2: Run unit tests (C++ Google Test)
    try:
        run_unit_tests()
    except FileNotFoundError:
        print("⚠️ ctest not found — skipping C++ unit test execution.")

    # Report summary
    print("\n" + "=" * 60)
    print("Coverage Summary")
    print("=" * 60)
    print(f"  • E2E Tests (Python):   pytest --cov=... → {html_report_dir}")
    print(f"  • Unit Tests (C++):     gtest via CMake ctest")
    print("  • Target: ≥90% coverage for both code paths.")

    return 0


if __name__ == '__main__':
    sys.exit(main())
