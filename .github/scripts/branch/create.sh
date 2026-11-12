#!/bin/bash
set -euo pipefail

. ./scripts/.helpers/repo-norms.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Build types array from shared norm
IFS=' ' read -ra TYPES <<< "$BRANCH_TYPES_ARRAY"

select_type() {
  local current="$1"

  echo -e "${BLUE}Select type:${NC}"
  for i in "${!TYPES[@]}"; do
    local label=$((i+1))
    if [ "${TYPES[$i]}" = "$current" ]; then
      echo -e "${GREEN}${label}) ${TYPES[$i]} ◀ current${NC}"
    else
      echo -e "${YELLOW}${label})${NC} ${TYPES[$i]}"
    fi
  done
  echo ""
  read -p "Select type (1-${#TYPES[@]}): " TYPE_SELECTION

  if ! [[ "$TYPE_SELECTION" =~ ^[0-9]+$ ]] || [ "$TYPE_SELECTION" -lt 1 ] || [ "$TYPE_SELECTION" -gt "${#TYPES[@]}" ]; then
    echo -e "${RED}Invalid selection${NC}"
    exit 1
  fi

  SELECTED_TYPE="${TYPES[$((TYPE_SELECTION-1))]}"
}

if ! command -v gh &> /dev/null; then
  echo -e "${RED}GitHub CLI (gh) is not installed${NC}"
  echo "Install it from: https://cli.github.com"
  exit 1
fi

echo -e "${BLUE}Interactive Branch Creator${NC}"
echo ""

REPO=$(gh repo view --json nameWithOwner --template '{{.nameWithOwner}}')
echo -e "${BLUE}Repository: ${YELLOW}$REPO${NC}"
echo ""

ORG=$(echo "$REPO" | cut -d'/' -f1)
REPO_NAME=$(echo "$REPO" | cut -d'/' -f2)

echo -e "${BLUE}📥 Fetching open issues...${NC}"
ISSUES=$(gh issue list --state open --limit 50 --json number,title,labels \
  --jq '.[] | "\(.number)|\(.title)|\(.labels | map(.name) | join(","))"')

if [ -z "$ISSUES" ]; then
  echo -e "${RED}No open issues found${NC}"
  exit 1
fi

echo -e "${BLUE}Available issues:${NC}"
echo ""

declare -A ISSUE_MAP
while IFS='|' read -r ISSUE_NUM ISSUE_TITLE LABELS; do
  ISSUE_MAP[$ISSUE_NUM]="$ISSUE_TITLE|$LABELS"
  echo -e "${YELLOW}#$ISSUE_NUM${NC} - $ISSUE_TITLE"
  if [ -n "$LABELS" ]; then
    echo -e "   Labels: ${BLUE}${LABELS%,}${NC}"
  fi
done <<< "$ISSUES"

echo ""
read -p "Enter issue number (e.g., 42 for #42): " ISSUE_NUM

if ! [[ "$ISSUE_NUM" =~ ^[0-9]+$ ]]; then
  echo -e "${RED}Invalid issue number${NC}"
  exit 1
fi

if [ -z "${ISSUE_MAP[$ISSUE_NUM]}" ]; then
  echo -e "${RED}Issue #$ISSUE_NUM not found in the list${NC}"
  exit 1
fi

IFS='|' read -r ISSUE_TITLE LABELS <<< "${ISSUE_MAP[$ISSUE_NUM]}"

echo ""
echo -e "${GREEN}Selected: #$ISSUE_NUM - $ISSUE_TITLE${NC}"
echo ""

GITHUB_TYPE=$(bash "$(dirname "$0")/../.helpers/get-issue-type.sh" "$ORG" "$REPO_NAME" "$ISSUE_NUM" 2>/dev/null)

if [ "$GITHUB_TYPE" = "feature" ]; then
  echo -e "${GREEN}Issue type: feature → feat${NC}"
  echo ""
  read -p "Keep type 'feat'? (y/n): " KEEP_TYPE
  if [[ "$KEEP_TYPE" =~ ^[Yy]$ ]]; then
    TAG="feat"
  else
    echo ""
    select_type "feat"
    TAG="$SELECTED_TYPE"
  fi

elif [ "$GITHUB_TYPE" = "bug" ]; then
  echo -e "${GREEN}Issue type: bug → fix${NC}"
  echo ""
  read -p "Keep type 'fix'? (y/n): " KEEP_TYPE
  if [[ "$KEEP_TYPE" =~ ^[Yy]$ ]]; then
    TAG="fix"
  else
    echo ""
    select_type "fix"
    TAG="$SELECTED_TYPE"
  fi

elif [ "$GITHUB_TYPE" = "task" ]; then
  echo -e "${YELLOW}Issue type 'task' has no direct mapping — please select a type:${NC}"
  echo ""
  select_type ""
  TAG="$SELECTED_TYPE"

else
  echo -e "${YELLOW}No issue type found — please select one:${NC}"
  echo ""
  select_type ""
  TAG="$SELECTED_TYPE"
fi

echo ""
echo -e "${GREEN}Type: $TAG${NC}"
echo ""

TITLE_SLUG=$(echo "$ISSUE_TITLE" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/-/g' | sed 's/-+/-/g' | sed 's/^-\|-$//' | cut -c1-50)

BRANCH_NAME="${TAG}/${ISSUE_NUM}/${TITLE_SLUG}"

echo -e "${BLUE}Branch name: ${YELLOW}$BRANCH_NAME${NC}"
echo ""
read -p "Create branch? (y/n/c - customize): " CONFIRM

if [[ "$CONFIRM" =~ ^[Cc]$ ]]; then
  echo ""
  echo -e "${BLUE}Current branch name: ${YELLOW}$BRANCH_NAME${NC}"
  echo -e "${BLUE}Format: ${TAG}/{ISSUE_NUMBER}/{CUSTOM_MESSAGE}${NC}"
  echo ""
  read -p "Enter custom message (or press Enter to keep current): " CUSTOM_MESSAGE

  if [ -n "$CUSTOM_MESSAGE" ]; then
    CUSTOM_MESSAGE=$(echo "$CUSTOM_MESSAGE" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/-/g' | sed 's/-+/-/g' | sed 's/^-\|-$//')
    BRANCH_NAME="${TAG}/${ISSUE_NUM}/${CUSTOM_MESSAGE}"
    echo ""
    echo -e "${BLUE}Updated branch name: ${YELLOW}$BRANCH_NAME${NC}"
    echo ""
  fi

  read -p "Create branch? (y/n): " CONFIRM
fi

if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
  echo -e "${YELLOW}Cancelled${NC}"
  exit 0
fi

echo ""
echo -e "${BLUE}Creating branch...${NC}"
echo -e "${BLUE}$ git checkout -b \"$BRANCH_NAME\"${NC}"
git checkout -b "$BRANCH_NAME"

echo ""
echo -e "${GREEN}Branch created!${NC}"
echo ""
echo -e "Branch: ${YELLOW}$BRANCH_NAME${NC}"
echo -e "Issue:  ${YELLOW}#$ISSUE_NUM - $ISSUE_TITLE${NC}"
echo ""
echo -e "${BLUE}Next commit format:${NC} ${YELLOW}${TAG}(${ISSUE_NUM}): <your message>${NC}"
