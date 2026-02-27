#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"

echo "[regression] configure"
cmake -S . -B "${BUILD_DIR}"

echo "[regression] build tests"
cmake --build "${BUILD_DIR}" --target libftpp_tests -j

echo "[regression] run ctest"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "[regression] passed"