#!/bin/bash
set -euo pipefail

RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Release"
echo ""

CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$CURRENT_BRANCH" != "main" ]; then
  echo -e "${RED}Must be on main branch${NC}"
  echo "Current: $CURRENT_BRANCH"
  exit 1
fi

if ! ls .changeset/*.md 1> /dev/null 2>&1; then
  echo "No changesets found"
  echo "Create one with: pnpm release"
  exit 0
fi

CHANGESET_COUNT=$(ls -1 .changeset/*.md 2>/dev/null | grep -v README.md | wc -l)
if [ "$CHANGESET_COUNT" -eq 0 ]; then
  echo "No changesets found"
  exit 0
fi

echo "Found $CHANGESET_COUNT changeset(s)"
echo ""

pnpm changeset status
echo ""

read -p "Continue with version bump? (y/n): " VERSION_CONFIRM
if [[ ! "$VERSION_CONFIRM" =~ ^[Yy]$ ]]; then
  echo "Cancelled"
  exit 0
fi

echo ""
pnpm changeset version

VERSION=$(node -p "require('./package.json').version" 2>/dev/null || echo "")

echo ""
git status --short
echo ""

read -p "Commit version changes? (y/n): " COMMIT_CONFIRM
if [[ ! "$COMMIT_CONFIRM" =~ ^[Yy]$ ]]; then
  echo "Not committed"
  exit 0
fi

DIRTY_FILES=$(git status --porcelain | grep -v "package.json\|CHANGELOG.md\|\.changeset" || true)
if [ -n "$DIRTY_FILES" ]; then
  echo -e "${YELLOW}Warning: Uncommitted changes outside version files${NC}"
  echo "$DIRTY_FILES"
  echo ""
  read -p "Stage all anyway? (y/n): " STAGE_CONFIRM
  if [[ ! "$STAGE_CONFIRM" =~ ^[Yy]$ ]]; then
    echo "Cancelled"
    exit 0
  fi
fi

git add .

if [ -n "$VERSION" ] && [ "$VERSION" != "null" ]; then
  git commit -m "chore(release): $VERSION"
else
  git commit -m "chore(release): bump"
fi

echo ""
read -p "Push to remote? (y/n): " PUSH_CONFIRM
if [[ "$PUSH_CONFIRM" =~ ^[Yy]$ ]]; then
  git push
fi

echo ""
read -p "Publish to npm? (y/n): " NPM_CONFIRM
if [[ "$NPM_CONFIRM" =~ ^[Yy]$ ]]; then
  pnpm changeset publish
  git push --follow-tags
  echo "Published"
else
  echo "Skipped npm publish"
fi

echo ""
echo "Done"
