#!/bin/bash
set -euo pipefail

. ./scripts/.helpers/repo-norms.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}Interactive Commit${NC}"
echo ""

# Check for staged files
STAGED=$(git diff --cached --name-only 2>/dev/null)
if [ -z "$STAGED" ]; then
  echo -e "${YELLOW}No staged files found.${NC}"
  echo ""
  echo -e "${BLUE}Stage your changes first:${NC}"
  echo -e "  ${YELLOW}git add <files>${NC}     # Stage specific files"
  echo -e "  ${YELLOW}git add .${NC}           # Stage all changes"
  echo ""
  read -p "Stage all changes now? (y/n): " STAGE_ALL
  if [[ "$STAGE_ALL" =~ ^[Yy]$ ]]; then
    git add .
    STAGED=$(git diff --cached --name-only 2>/dev/null)
    if [ -z "$STAGED" ]; then
      echo -e "${RED}No changes to commit${NC}"
      exit 1
    fi
    echo -e "${GREEN}All changes staged${NC}"
    echo ""
  else
    echo -e "${YELLOW}Cancelled${NC}"
    exit 0
  fi
fi

BRANCH=$(git rev-parse --abbrev-ref HEAD)

if [ "$BRANCH" = "main" ]; then
  TICKET="0"
  echo -e "${YELLOW}On main branch - using ticket #0${NC}"
else
  TICKET=$(echo "$BRANCH" | sed -E 's/^[a-z]+\/([0-9]+)\/.*/\1/')
  
  if [ -z "$TICKET" ] || [ "$TICKET" = "$BRANCH" ]; then
    echo -e "${RED}Could not extract ticket from branch: $BRANCH${NC}"
    echo "Expected format: type/NUMBER/description"
    exit 1
  fi
  
  echo -e "${GREEN}Issue: #$TICKET${NC}"
fi
echo ""

# Build types array from shared norm
IFS=' ' read -ra TYPES <<< "$COMMIT_TYPES_ARRAY"

echo -e "${BLUE}Select commit type:${NC}"
for i in "${!TYPES[@]}"; do
  label=$((i+1))
  [ "$label" -eq 10 ] && label=0
  echo -e "${YELLOW}${label})${NC} ${TYPES[$i]}"
done

echo ""
read -p "Select type (1-9, 0): " TYPE_SELECTION

if ! [[ "$TYPE_SELECTION" =~ ^[0-9]$ ]]; then
  echo -e "${RED}Invalid type selection${NC}"
  exit 1
fi

[ "$TYPE_SELECTION" -eq 0 ] && TYPE_SELECTION=10

COMMIT_TYPE="${TYPES[$((TYPE_SELECTION-1))]}"

echo ""
echo -e "${GREEN}Type: $COMMIT_TYPE${NC}"
echo ""

read -p "Commit message: " COMMIT_MSG

if [ -z "$COMMIT_MSG" ]; then
  echo -e "${RED}Commit message cannot be empty${NC}"
  exit 1
fi

FINAL_MSG="${COMMIT_TYPE}(${TICKET}): $COMMIT_MSG"

echo ""
echo -e "${BLUE}Commit message:${NC}"
echo -e "${YELLOW}$FINAL_MSG${NC}"
echo ""

read -p "Commit? (y/n): " CONFIRM

if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
  echo -e "${YELLOW}⊘ Cancelled${NC}"
  exit 0
fi

touch "$INTERACTIVE_COMMIT_FLAG"

echo -e "${BLUE}$ git commit -m \"$FINAL_MSG\"${NC}"
git commit -m "$FINAL_MSG"

echo ""
echo -e "${GREEN}Committed!${NC}"
