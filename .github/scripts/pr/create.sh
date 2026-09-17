#!/bin/bash
set -euo pipefail

. ./scripts/.helpers/repo-norms.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}Interactive PR Creator${NC}"
echo ""

if ! command -v gh &> /dev/null; then
  echo -e "${RED}GitHub CLI (gh) is not installed${NC}"
  echo "Install it from: https://cli.github.com"
  exit 1
fi

CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

if [ "$CURRENT_BRANCH" = "main" ]; then
  echo -e "${RED}Cannot create PR from main branch${NC}"
  exit 1
fi

echo -e "${GREEN}Source branch: $CURRENT_BRANCH${NC}"
echo ""

TICKET=""
if echo "$CURRENT_BRANCH" | grep -qE "$BRANCH_PATTERN" && [[ "$CURRENT_BRANCH" =~ ^([a-z]+)/([0-9]+)/(.+)$ ]]; then
  TAG="${BASH_REMATCH[1]}"
  TICKET="${BASH_REMATCH[2]}"
  DESCRIPTION="${BASH_REMATCH[3]}"

  TITLE=$(echo "$DESCRIPTION" | sed 's/-/ /g' | awk '{for(i=1;i<=NF;i++)sub(/./,toupper(substr($i,1,1)),$i)}1')

  PR_TITLE="${TAG}(<-${TICKET}): $TITLE"
else
  echo -e "${RED}Branch name doesn't match expected format${NC}"
  echo "Expected: type/number/description"
  echo "Example: feat/42/add-user-login"
  echo ""
  PR_TITLE="$CURRENT_BRANCH"
fi

echo -e "${BLUE}PR Title:${NC} ${YELLOW}$PR_TITLE${NC}"
echo ""

echo -e "${BLUE}Select destination branch:${NC}"
echo -e "${YELLOW}1)${NC} main"
echo -e "${YELLOW}2)${NC} parent branch (from reflog)"
echo -e "${YELLOW}3)${NC} other (show all branches)"
echo ""

read -p "Select destination (1-3): " DEST_SELECTION

case "$DEST_SELECTION" in
  1)
    DEST_BRANCH="main"
    ;;
  2)
    ORIGIN_BRANCH=$(git reflog show --format="%gs" "$CURRENT_BRANCH" | grep -m1 "branch: Created from" | sed 's/.*Created from //' || echo "")

    if [ -z "$ORIGIN_BRANCH" ]; then
      echo -e "${YELLOW}Could not determine origin branch, defaulting to main${NC}"
      DEST_BRANCH="main"
    else
      DEST_BRANCH="$ORIGIN_BRANCH"
    fi
    ;;
  3)
    echo ""
    echo -e "${BLUE}Available branches:${NC}"
    git branch -a | grep -v "HEAD" | sed 's/remotes\/origin\///' | sort -u | nl
    echo ""
    read -p "Enter branch name: " DEST_BRANCH
    ;;
  *)
    echo -e "${RED}Invalid selection${NC}"
    exit 1
    ;;
esac

echo ""
echo -e "${GREEN}Destination branch: $DEST_BRANCH${NC}"
echo ""

echo -e "${BLUE}Create PR:${NC}"
echo -e "  ${YELLOW}From:${NC} $CURRENT_BRANCH"
echo -e "  ${YELLOW}To:${NC} $DEST_BRANCH"
echo -e "  ${YELLOW}Title:${NC} $PR_TITLE"
echo ""

read -p "Create PR? (y/n): " CONFIRM

if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
  echo -e "${YELLOW}Cancelled${NC}"
  exit 0
fi

if ! git ls-remote --exit-code --heads origin "$CURRENT_BRANCH" > /dev/null 2>&1; then
  echo ""
  echo -e "${BLUE}Pushing branch to origin...${NC}"
  echo -e "${BLUE}$ git push -u origin $CURRENT_BRANCH${NC}"
  git push -u origin "$CURRENT_BRANCH"
fi

echo ""
echo -e "${BLUE}Creating PR...${NC}"

if [ -n "$TICKET" ]; then
  echo -e "${BLUE}$ gh pr create --base \"$DEST_BRANCH\" --head \"$CURRENT_BRANCH\" --title \"$PR_TITLE\" --body \"Closes #$TICKET\"${NC}"
  gh pr create --base "$DEST_BRANCH" --head "$CURRENT_BRANCH" --title "$PR_TITLE" --body "Closes #$TICKET"
  echo ""
  echo -e "${GREEN}✓ PR created and linked to issue #$TICKET!${NC}"
else
  echo -e "${BLUE}$ gh pr create --base \"$DEST_BRANCH\" --head \"$CURRENT_BRANCH\" --title \"$PR_TITLE\"${NC}"
  gh pr create --base "$DEST_BRANCH" --head "$CURRENT_BRANCH" --title "$PR_TITLE"
  echo ""
  echo -e "${GREEN}✓ PR created!${NC}"
fi
