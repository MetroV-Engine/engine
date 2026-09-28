---
inclusion: auto
name: mobile-core
description: Creating or modifying React Native/Expo mobile components. Use for component architecture, structure, naming, composition patterns, styling, and TypeScript.
---

# Mobile Core Principles

React Native/Expo + TypeScript + styled-components. Strict architectural patterns.

## Stack
- React Native/Expo, TypeScript, styled-components, react-i18next, Context API
- Hierarchical component structure

## Non-Negotiables
1. Hierarchical naming (e.g., `CollectionFilterBarModal`)
2. styled-components only (no inline styles, no hardcoded colors)
3. All text via `t()` (no hardcoded strings)
4. Container/leaf pattern
5. Types in `types.ts`
6. `useColors()` for all colors
7. Skeleton loading for data-dependent components
8. **Performance first** - useMemo, useCallback, React.memo

## Performance Priority
**Make code longer/complex if it's faster.** Memoize computed values, callbacks, avoid re-renders.

## Structure & Naming
#[[file:.kiro/steering/mobile/01-structure-naming.md]]

## Composition Patterns
#[[file:.kiro/steering/mobile/02-composition-patterns.md]]

## Styling
#[[file:.kiro/steering/mobile/03-styling.md]]

## TypeScript
#[[file:.kiro/steering/mobile/04-typescript.md]]
