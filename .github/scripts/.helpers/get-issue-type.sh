#!/bin/bash

# Not for direct user execution
# Gets the issue type from GitHub for a given issue number
# Called by: commit-msg hook, create-branch-interactive.sh
#
# Usage: get-issue-type.sh <org> <repo> <issue_number>
# Returns: feature, bug, task, or empty string

if [ -z "$3" ]; then
  echo "Usage: $0 <org> <repo> <issue-number>"
  exit 1
fi

if ! command -v gh &> /dev/null; then
  echo "Error: GitHub CLI (gh) is not installed" >&2
  echo "Install it from: https://cli.github.com" >&2
  exit 1
fi

if ! command -v jq &> /dev/null; then
  echo "Error: jq is not installed" >&2
  echo "Install it from: https://stedolan.github.io/jq/download/" >&2
  exit 1
fi

org="$1"
repo="$2"
issue_number="$3"

# Fetch the issue type using GraphQL
issue=$(gh api graphql -H GraphQL-Features:issue_types -f owner="$org" -f repository="$repo" -F number="$issue_number" -f query='
query ($owner: String!, $repository: String!, $number: Int!) {
  repository(owner: $owner, name: $repository) {
    issue(number: $number) {
      issueType {
        name
      }
    }
  }
}' 2>/dev/null)

# Extract the issue type
issue_type=$(echo "$issue" | jq -r '.data.repository.issue.issueType.name // empty' 2>/dev/null)

# Return the issue type (lowercase)
if [ -n "$issue_type" ] && [ "$issue_type" != "null" ]; then
  echo "$issue_type" | tr '[:upper:]' '[:lower:]'
else
  echo ""
fi