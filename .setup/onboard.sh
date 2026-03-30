#!/bin/bash
set -euo pipefail

# ── Pull scripts submodule ──
if ! git submodule update --init --remote scripts 2>/dev/null; then
  echo ""
  echo "⚠  Could not fetch scripts submodule (you may not have access to the MetroV-Engine org)."
  echo "   CI scripts won't be available locally — this is fine for non-CI work."
  echo ""
  exit 0
fi

# ── Show onboarding info ──
WORKSPACE_NAME=$(node -p "require('./package.json').name" 2>/dev/null || echo "workspace")

echo ""
echo "🎉 $WORKSPACE_NAME setup complete!"
echo ""

if [ -f ./scripts/.helpers/repo-norms.sh ]; then
  . ./scripts/.helpers/repo-norms.sh
  echo "$AVAILABLE_COMMANDS"
else
  echo "Available commands:"
  echo "  pnpm helpme    - Show help"
  echo "  pnpm checkout  - Create branch from issue"
  echo "  pnpm commit    - Commit changes"
  echo "  pnpm pr        - Create pull request"
  echo "  pnpm merge     - Merge PR"
fi

echo ""
