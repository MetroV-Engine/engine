#!/bin/bash
set -euo pipefail

. ./scripts/.helpers/repo-norms.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}Interactive Release Preparation${NC}"
echo ""

# Validate we're not on main
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$CURRENT_BRANCH" = "main" ]; then
  echo -e "${RED}Cannot create changeset from main branch${NC}"
  exit 1
fi

echo -e "${GREEN}Current branch: $CURRENT_BRANCH${NC}"
echo ""

# Extract ticket from branch
TICKET=$(echo "$CURRENT_BRANCH" | sed -E 's/^[a-z]+\/([0-9]+)\/.*/\1/' || echo "")
if [ -n "$TICKET" ] && [ "$TICKET" != "$CURRENT_BRANCH" ]; then
  echo -e "${GREEN}Ticket: $TICKET${NC}"
  echo ""
fi

# Detect changed files
echo -e "${BLUE}📦 Detecting changed packages...${NC}"
CHANGED_FILES=$(git diff --name-only main...HEAD 2>/dev/null || git diff --name-only HEAD~1 2>/dev/null || echo "")

if [ -z "$CHANGED_FILES" ]; then
  echo -e "${YELLOW}No changes detected${NC}"
  exit 0
fi

# Build package-to-prefix mapping dynamically from subtrees.json (if exists)
declare -A CHANGED_PACKAGES
if [ -f ./subtrees.json ]; then
  SUBTREE_PREFIXES=$(node -e "require('./subtrees.json').forEach(e => console.log(e.prefix))")

  while IFS= read -r file; do
    while IFS= read -r prefix; do
      if [[ "$file" =~ ^${prefix}/ ]]; then
        PKG_NAME=$(node -p "require('./${prefix}/package.json').name" 2>/dev/null || echo "$prefix")
        CHANGED_PACKAGES["$PKG_NAME"]="$prefix"
        break
      fi
    done <<< "$SUBTREE_PREFIXES"
  done <<< "$CHANGED_FILES"
fi

if [ ${#CHANGED_PACKAGES[@]} -eq 0 ]; then
  echo -e "${YELLOW}No package changes detected (only root or config files)${NC}"
  echo ""
  read -p "Create changeset anyway? (y/n): " CREATE_ANYWAY
  if [[ ! "$CREATE_ANYWAY" =~ ^[Yy]$ ]]; then
    exit 0
  fi
fi

# Show detected packages
if [ ${#CHANGED_PACKAGES[@]} -gt 0 ]; then
  echo ""
  for pkg in "${!CHANGED_PACKAGES[@]}"; do
    echo -e "  ${GREEN}•${NC} $pkg (${CHANGED_PACKAGES[$pkg]})"
  done
  echo ""
fi

# Show instructions before running changeset
echo -e "${BLUE}Creating changeset...${NC}"
echo ""
echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${RED}IMPORTANT: Use SPACE to select packages, ENTER to confirm${NC}"
echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${YELLOW}Instructions:${NC}"
echo -e "  1. ${GREEN}Press SPACE${NC} to select/deselect packages"
echo -e "  2. ${GREEN}Press ENTER${NC} when done selecting"
echo -e "  3. Choose version bump type (major/minor/patch)"
echo -e "  4. Write a summary of changes"
echo ""
read -p "Press ENTER to continue..."
echo ""

pnpm changeset

# Check if changeset was created
if ! ls .changeset/*.md 1> /dev/null 2>&1 || [ "$(ls -1 .changeset/*.md 2>/dev/null | wc -l)" -eq 0 ]; then
  echo ""
  echo -e "${YELLOW}No changeset created${NC}"
  exit 0
fi

# Get the latest changeset file
LATEST_CHANGESET=$(ls -t .changeset/*.md | grep -v README.md | head -1)

echo ""
echo -e "${BLUE}Changeset created:${NC}"
echo -e "${YELLOW}$(basename "$LATEST_CHANGESET")${NC}"
echo ""

# Show preview
echo -e "${BLUE}Preview:${NC}"
echo -e "${YELLOW}─────────────────────────────────────${NC}"
cat "$LATEST_CHANGESET"
echo -e "${YELLOW}─────────────────────────────────────${NC}"
echo ""

# Commit changeset
read -p "Commit changeset? (y/n): " COMMIT_CONFIRM

if [[ "$COMMIT_CONFIRM" =~ ^[Yy]$ ]]; then
  git add .changeset/
  
  if [ -n "$TICKET" ] && [ "$TICKET" != "$CURRENT_BRANCH" ]; then
    COMMIT_MSG="chore($TICKET): add changeset"
  else
    COMMIT_MSG="chore: add changeset"
  fi
  
  touch "$INTERACTIVE_COMMIT_FLAG"
  git commit -m "$COMMIT_MSG"
  
  echo ""
  echo -e "${GREEN}✓ Changeset committed${NC}"
else
  echo ""
  echo -e "${YELLOW}Changeset created but not committed${NC}"
  echo -e "${BLUE}Stage with 'git add .changeset/' then run 'pnpm commit'${NC}"
fi

echo ""
echo -e "${GREEN}✓ Release preparation complete${NC}"
echo ""
echo -e "${BLUE}Next steps:${NC}"
echo -e "  • Push changes: ${YELLOW}git push${NC}"
echo -e "  • Create PR: ${YELLOW}pnpm pr${NC}"
