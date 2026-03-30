---
inclusion: auto
name: mobile-checklist
description: Quick reference checklist for mobile development
---
# Quick Reference Checklist

## Creating New Component

- [ ] Hierarchical naming (include grouping dirs like `Views/`)
- [ ] Styled components inline (no `styles.ts`)
- [ ] All interfaces in `types.ts` (even if empty)
- [ ] No `$` prefix on styled props
- [ ] `useColors()` for all colors (⛔ ZERO hardcoded hex)
- [ ] Common `Text` wrapped in named feature component
- [ ] Icons wrapped in named feature components
- [ ] Flat composition (no children except common primitives)
- [ ] `index.ts` for external exports only
- [ ] Within component: import from direct files, NOT index.ts
- [ ] No inline JSX in `.map()` - extract to sub-components
- [ ] Multiple returns = each is single sub-component
- [ ] Container = ONLY sub-components, NO direct display elements
- [ ] Skeleton loading with `isLoading` prop
- [ ] All text via `t()` (including accessibility labels)
- [ ] Console.log removed before commit

## Performance

- [ ] Computed values use `useMemo`
- [ ] Event handlers use `useCallback`
- [ ] Objects/arrays as props are memoized
- [ ] Dynamic `style` values memoized
- [ ] List items wrapped with `React.memo`
- [ ] `useEffect` only for side effects
- [ ] Complete dependency arrays

## Component Types

**Container:** Only calls sub-components (no icons, text, inputs)
**Leaf:** Renders exactly ONE display element

## Key Rules

- Icons from libraries = display elements → wrap in named components
- `styled(Component)` syntax, never `styled.element`
- ⛔ NEVER hardcoded colors (no #hex, use `colors.*` or `colors.palette.*`)
- Types in `types.ts`, never inline in `.tsx`
- `style` prop ONLY for dynamic runtime values
- All text via `t('key')`, including accessibility
- Pass `isLoading` to components, handle skeleton internally
- SkeletonIgnore for static UI and meta-UI (Error/Loading/Empty states)
