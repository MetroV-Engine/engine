# Changesets

This directory contains changeset files that describe changes to be released.

## Creating a Changeset

Run `npm run release` to create a new changeset interactively.

## What is a Changeset?

A changeset is a piece of information about changes made in a branch or commit. It holds three key pieces of information:

1. What we need to release
2. What version we are releasing packages at (using a [semver bump type](https://semver.org/))
3. A changelog entry for the released packages

## How to Use

1. Make your changes
2. Run `npm run release` to create a changeset
3. Commit the changeset file along with your changes
4. When ready to release, merge to main and the changeset will be consumed
