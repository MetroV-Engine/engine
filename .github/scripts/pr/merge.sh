#!/bin/bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

if ! command -v gh &> /dev/null; then
  echo -e "${RED}GitHub CLI (gh) is not installed${NC}"
  echo "Install it from: https://cli.github.com"
  exit 1
fi

if ! command -v jq &> /dev/null; then
  echo -e "${RED}jq is not installed${NC}"
  echo "Install it from: https://stedolan.github.io/jq/download/"
  exit 1
fi

PR="${1:-}"

if [ -z "$PR" ]; then
  echo -e "${BLUE}Interactive PR Merger${NC}"
  echo ""

  REPO=$(gh repo view --json nameWithOwner --template '{{.nameWithOwner}}')
  echo -e "${BLUE}Repository: ${YELLOW}$REPO${NC}"
  echo ""

  echo -e "${BLUE}📥 Fetching open PRs...${NC}"
  PRS=$(gh pr list --state open --limit 20 --json number,title,author,reviewDecision \
    --jq '.[] | "\(.number)|\(.title)|\(.author.login)|\(.reviewDecision)"')

  if [ -z "$PRS" ]; then
    echo -e "${RED}No open PRs found${NC}"
    exit 1
  fi

  echo -e "${BLUE}Available PRs:${NC}"
  echo ""

  declare -A PR_MAP
  while IFS='|' read -r PR_NUM PR_TITLE AUTHOR REVIEW_DECISION; do
    PR_MAP[$PR_NUM]="$PR_TITLE|$AUTHOR|$REVIEW_DECISION"

    if [ "$REVIEW_DECISION" = "APPROVED" ]; then
      STATUS_COLOR="${GREEN}✓ APPROVED${NC}"
    elif [ "$REVIEW_DECISION" = "CHANGES_REQUESTED" ]; then
      STATUS_COLOR="${RED}✗ CHANGES REQUESTED${NC}"
    elif [ "$REVIEW_DECISION" = "PENDING" ]; then
      STATUS_COLOR="${YELLOW}⏳ PENDING REVIEW${NC}"
    else
      STATUS_COLOR="${BLUE}○ NO REVIEW${NC}"
    fi

    echo -e "${YELLOW}#$PR_NUM${NC} - $PR_TITLE"
    echo -e "   Author: ${BLUE}$AUTHOR${NC} | Status: $STATUS_COLOR"
  done <<< "$PRS"

  echo ""
  read -p "Enter PR number to merge: " PR

  if ! [[ "$PR" =~ ^[0-9]+$ ]]; then
    echo -e "${RED}Invalid PR number${NC}"
    exit 1
  fi

  if [ -z "${PR_MAP[$PR]:-}" ]; then
    echo -e "${RED}PR #$PR not found in the list${NC}"
    exit 1
  fi
fi

echo ""
echo -e "${BLUE}Validating PR #${YELLOW}$PR${NC}..."
echo ""

PR_DATA=$(gh pr view "$PR" --json number,title,author,reviewDecision,isDraft \
  --jq '{number: .number, title: .title, author: .author.login, reviewDecision: .reviewDecision, isDraft: .isDraft}')

PR_TITLE=$(echo "$PR_DATA" | jq -r '.title')
PR_AUTHOR=$(echo "$PR_DATA" | jq -r '.author')
REVIEW_DECISION=$(echo "$PR_DATA" | jq -r '.reviewDecision')
IS_DRAFT=$(echo "$PR_DATA" | jq -r '.isDraft')

echo -e "${GREEN}PR #$PR: $PR_TITLE${NC}"
echo -e "Author: ${BLUE}$PR_AUTHOR${NC}"
echo ""

VALIDATION_PASSED=true

if [ "$IS_DRAFT" = "true" ]; then
  echo -e "${RED}✗ PR is still a draft${NC}"
  VALIDATION_PASSED=false
else
  echo -e "${GREEN}✓ PR is ready for review${NC}"
fi

if [ "$REVIEW_DECISION" = "APPROVED" ]; then
  echo -e "${GREEN}✓ PR has been approved${NC}"
elif [ "$REVIEW_DECISION" = "CHANGES_REQUESTED" ]; then
  echo -e "${RED}✗ PR has changes requested${NC}"
  VALIDATION_PASSED=false
elif [ "$REVIEW_DECISION" = "PENDING" ]; then
  echo -e "${YELLOW}⚠ PR is pending review${NC}"
  VALIDATION_PASSED=false
else
  echo -e "${YELLOW}⚠ PR has no review yet${NC}"
  VALIDATION_PASSED=false
fi

echo ""

if [ "$VALIDATION_PASSED" = false ]; then
  echo -e "${RED}⚠ PR does not meet merge requirements${NC}"
  echo ""
  read -p "Continue anyway? (y/n): " CONTINUE

  if [[ ! "$CONTINUE" =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}Merge cancelled${NC}"
    exit 0
  fi
fi

echo ""
read -p "Merge PR #$PR? (y/n): " CONFIRM

if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
  echo -e "${YELLOW}Merge cancelled${NC}"
  exit 0
fi

echo ""
echo -e "${BLUE}Merging PR #${YELLOW}$PR${NC}...${NC}"
echo -e "${BLUE}$ gh pr merge \"$PR\" --squash${NC}"

if gh pr merge "$PR" --squash; then
  echo -e "${GREEN}✓ PR merged successfully${NC}"
else
  echo -e "${RED}✗ Failed to merge PR${NC}"
  exit 1
fi

echo ""
echo -e "${BLUE}Updating main branch...${NC}"
echo -e "${BLUE}$ git checkout main${NC}"
if ! git checkout main 2>&1; then
  echo -e "${YELLOW}⚠ Could not switch to main (you may have uncommitted changes)${NC}"
  echo -e "${YELLOW}  Run 'git checkout main && git pull' manually${NC}"
else
  echo -e "${BLUE}$ git pull${NC}"
  if ! git pull 2>&1; then
    echo -e "${YELLOW}⚠ Could not pull latest main${NC}"
  fi
fi

# Check for changesets
if ls .changeset/*.md 1> /dev/null 2>&1; then
  CHANGESET_COUNT=$(ls -1 .changeset/*.md 2>/dev/null | grep -v README.md | wc -l)
  if [ "$CHANGESET_COUNT" -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}⚠ Found $CHANGESET_COUNT changeset(s)${NC}"
    echo -e "${BLUE}Run 'pnpm release' to version packages${NC}"
  fi
fi

echo ""
echo -e "${GREEN}✓ Merge complete!${NC}"
