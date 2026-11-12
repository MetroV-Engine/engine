#!/bin/sh

# Not for direct user execution
# Defines repository norms and patterns used by other scripts
# Used by: commit-msg hook, pre-commit hook, pre-push hook, and all interactive scripts

# ── Workspace identity ──
# Derived from package.json name — used for temp files and display
WORKSPACE_NAME=$(node -p "require('./package.json').name" 2>/dev/null || echo "workspace")

# ── Temp file for bypassing commit-msg hook from interactive scripts ──
INTERACTIVE_COMMIT_FLAG="/tmp/.${WORKSPACE_NAME}-interactive-commit"

# ── Branch and commit types ──
BRANCH_TYPES="build|chore|ci|docs|feat|fix|perf|refactor|test"
COMMIT_TYPES="build|chore|ci|docs|feat|fix|perf|refactor|test|wip"

# ── Validation patterns ──
BRANCH_PATTERN="^(${BRANCH_TYPES})/[0-9]+/.+"
COMMIT_PATTERN="^(${COMMIT_TYPES})\([0-9]+\): .+"

# ── Type arrays for interactive selection ──
# Split into two: branch types (no wip) and commit types (includes wip)
BRANCH_TYPES_ARRAY="feat fix refactor chore test docs perf ci build"
COMMIT_TYPES_ARRAY="feat fix refactor chore test docs perf ci build wip"

# ── Available commands ──
AVAILABLE_COMMANDS="Available commands:
  pnpm helpme    - Show commands and format
  pnpm checkout  - Create branch from GitHub issue
  pnpm commit    - Commit with enforced format
  pnpm pr        - Create pull request
  pnpm merge     - Merge a GitHub PR
  pnpm release   - Create a release"
