#!/usr/bin/env bash
set -euo pipefail

git config core.hooksPath .githooks
chmod +x .githooks/pre-push

echo "Git hooks installed: core.hooksPath=.githooks"
echo "pre-push hook now runs regression tests before every push."