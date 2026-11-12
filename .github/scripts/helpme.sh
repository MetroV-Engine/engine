#!/bin/bash

. ./scripts/.helpers/repo-norms.sh

echo "$AVAILABLE_COMMANDS"
echo ""
echo "Format:"
echo "  Branch: type/NUMBER/description"
echo "  Commit: type(NUMBER): message"
echo "  PR:     type(<-NUMBER): Description"
echo ""
echo "Types: ${BRANCH_TYPES}"
