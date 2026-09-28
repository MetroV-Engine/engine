---
inclusion: auto
name: mobile-skeleton
description: Adding loading states, skeleton screens, or handling data fetching in mobile components.
---

# Skeleton Loading States

## Overview

**Every page and feature component MUST implement skeleton loading states.** This application uses `react-native-auto-skeleton` via the common `Skeleton` and `SkeletonIgnore` components to provide automatic, zero-configuration skeleton loaders.

## Available Components

```tsx
import { Skeleton, SkeletonIgnore } from 'src/components/common';
```

- **`Skeleton`** - Wraps content that should show skeleton loading when `isLoading` is true
- **`SkeletonIgnore`** - Wraps content that should NOT show skeleton (static UI elements)

## Core Principles

1. **Skeletons wrap views, they don't replace them** - The actual component renders with placeholder/empty data, and the skeleton overlays it
2. **Place Skeleton INSIDE sub-components to preserve gaps** - Don't wrap entire components; wrap individual visual sections so spacing/gaps remain visible
3. **Use SkeletonIgnore for static content** - Buttons, filter bars, navigation, icons that don't depend on loaded data
4. **Pass `isLoading` prop down to sub-components** - Let each sub-component decide what to skeleton internally
5. **Every page needs loading state** - No page should show empty content while loading

## ⚠️ CRITICAL: Preserving Gaps and Spacing

**Skeleton overlays EVERYTHING inside its wrapper, including gaps and padding.** If you wrap too much content, the skeleton will appear as one solid block without visual separation.

### ❌ WRONG - Skeleton covers gaps between elements

```tsx
// DON'T DO THIS - Skeleton wraps entire card, gaps disappear
const renderItem = ({ item }) => (
  <Skeleton isLoading={loading}>
    <ProductCard product={item} /> {/* Gap between image and text is lost */}
  </Skeleton>
);
```

### ✅ CORRECT - Skeleton inside sub-component, preserving gaps

```tsx
// DO THIS - Pass isLoading to component, skeleton internally
const renderItem = ({ item }) => (
  <ProductCard product={item} isLoading={loading} />
);

// Inside ProductCard - wrap image and content SEPARATELY
export default function ProductCard({ product, isLoading = false }) {
  return (
    <Container>
      <Skeleton isLoading={isLoading}>
        <ProductCardImage imageUrl={product.images[0]?.url} />
      </Skeleton>
      {/* Gap between image and content is preserved! */}
      <Skeleton isLoading={isLoading}>
        <ProductCardContent>
          <ProductCardTitle title={product.title} />
          <ProductCardPrice price={product.price} />
        </ProductCardContent>
      </Skeleton>
    </Container>
  );
}
```

## The `isLoading` Prop Pattern

**⚠️ CRITICAL: ALL components that can display loading states MUST accept an `isLoading` prop, even if they don't currently need it.**

This ensures consistency and makes it easier to add loading states later without changing component interfaces.

Instead of wrapping components with Skeleton from the parent, **pass `isLoading` as a prop** and let each component handle its own skeleton internally:

```tsx
// ✅ CORRECT - Parent passes isLoading, child handles skeleton
// Parent component
<ProductCard product={product} isLoading={loading} />
<CartItem item={item} isLoading={loading} />

// Child component handles skeleton internally
export default function ProductCard({ product, isLoading = false }) {
  return (
    <Container>
      <Skeleton isLoading={isLoading}>
        <Image />
      </Skeleton>
      <Skeleton isLoading={isLoading}>
        <TextContent />
      </Skeleton>
      <SkeletonIgnore>
        <AddToCartButton />  {/* Interactive - never skeleton */}
      </SkeletonIgnore>
    </Container>
  );
}
```

## When to Use SkeletonIgnore

Use `SkeletonIgnore` for UI elements that:

- Don't depend on loaded data (buttons, icons, navigation)
- Are hardcoded/static (filter bars, tab bars, headers with static text)
- Should remain interactive during loading
- Are action controls (quantity buttons, delete buttons)
- Are meta-UI like error states, loading indicators, empty states

### Error, Loading, and Empty State Components

**Error, Loading, and Empty state components should be wrapped in `<SkeletonIgnore>` since they are meta-UI elements.**

````tsx
export default function MyFeaturePage({ id }: MyFeaturePageProps) {
  const { data, loading, error } = useMyFeatureData(id);

  // ✅ CORRECT - Error component in SkeletonIgnore
  if (error) {
    return (
      <Background>
        <SkeletonIgnore>
          <Error message={error.message} />
        </SkeletonIgnore>
      </Background>
    );
  }

  // ✅ CORRECT - Empty state in SkeletonIgnore
  if (!loading && !data) {
    return (
      <Background>
        <SkeletonIgnore>
          <MyFeaturePageEmpty />
        </SkeletonIgnore>
      </Background>
    );
  }

  return (
    <Background>
      <Skeleton isLoading={loading}>
        <MyFeaturePageContent data={data} />
      </Skeleton>
    </Background>
  );
}

```tsx
// Component with mixed content
export default function CartItem({ item, isLoading = false }) {
  return (
    <Container>
      {/* Data-dependent content - skeleton */}
      <Skeleton isLoading={isLoading}>
        <CartItemImage imageUrl={item.image?.url} />
      </Skeleton>
      <Skeleton isLoading={isLoading}>
        <CartItemContent title={item.title} price={item.price} />
      </Skeleton>

      {/* Interactive controls - never skeleton */}
      <SkeletonIgnore>
        <CartItemControls
          quantity={item.quantity}
          onIncrement={handleIncrement}
          onDecrement={handleDecrement}
          onRemove={handleRemove}
        />
      </SkeletonIgnore>
    </Container>
  );
}
````

## Skeleton with Lists (FlatList/FlashList)

**⚠️ CRITICAL: Never wrap an entire FlatList/FlashList with Skeleton. Pass `isLoading` to each list item instead.**

### ❌ WRONG - Skeleton wrapping list or list items from outside

```tsx
// DON'T DO THIS - Wrapping from outside loses gaps
<FlatList
  data={items}
  renderItem={({ item }) => (
    <Skeleton isLoading={loading}>
      <ListItem item={item} />
    </Skeleton>
  )}
/>
```

### ✅ CORRECT - Pass isLoading to list item, handle skeleton internally

```tsx
// DO THIS - Let list item handle its own skeleton
<FlatList
  data={items}
  renderItem={({ item }) => <ListItem item={item} isLoading={loading} />}
/>;

// ListItem handles skeleton internally with proper separation
export default function ListItem({ item, isLoading = false }) {
  return (
    <Container>
      <Skeleton isLoading={isLoading}>
        <ListItemImage />
      </Skeleton>
      <Skeleton isLoading={isLoading}>
        <ListItemContent />
      </Skeleton>
      <SkeletonIgnore>
        <ListItemActions />
      </SkeletonIgnore>
    </Container>
  );
}
```

## ❌ NEVER Use Placeholder Data Arrays

**Do NOT create placeholder data arrays for skeleton loading.** This pattern is unnecessary and adds complexity:

```tsx
// ❌ DON'T DO THIS - Placeholder data pattern
const PLACEHOLDER_ITEMS = Array.from({ length: 5 }, (_, i) => ({
  id: `placeholder-${i}`,
  title: 'Placeholder',
}));

const data = loading ? PLACEHOLDER_ITEMS : items;
```

Instead, simply pass `isLoading` to your list items and let them handle the skeleton state internally. The actual data structure doesn't need to change.

## Page-Level Implementation Pattern

Every page component should follow this pattern:

```tsx
export default function MyFeaturePage({ id }: MyFeaturePageProps) {
  const { data, loading, error } = useMyFeatureData(id);

  if (error) {
    return <Error message={error.message} />;
  }

  return (
    <Background>
      {/* Static header/navigation - always SkeletonIgnore */}
      <SkeletonIgnore>
        <MyFeaturePageHeader />
        <FilterBar />
      </SkeletonIgnore>

      {/* For non-list content, wrap sections separately */}
      <Skeleton isLoading={loading}>
        <MyFeaturePageImage />
      </Skeleton>
      <Skeleton isLoading={loading}>
        <MyFeaturePageDetails data={data} />
      </Skeleton>

      {/* For lists, pass isLoading to items */}
      <FlatList
        data={items}
        renderItem={({ item }) => (
          <MyListItem item={item} isLoading={loading} />
        )}
      />

      {/* Static footer/actions - always SkeletonIgnore */}
      <SkeletonIgnore>
        <MyFeaturePageActions />
      </SkeletonIgnore>
    </Background>
  );
}
```

## Quick Reference Checklist for Skeletons

When implementing skeleton loading:

- [ ] Pass `isLoading` prop to sub-components instead of wrapping with Skeleton from parent
- [ ] Skeleton placed INSIDE sub-components to preserve gaps/spacing
- [ ] Each visual section (image, text content) wrapped in separate Skeleton
- [ ] Static UI elements wrapped in `<SkeletonIgnore>`
- [ ] Interactive controls (buttons, quantity selectors) use SkeletonIgnore
- [ ] Filter bars, navigation, headers with static text use SkeletonIgnore
- [ ] Component renders with actual data structure (no placeholder data arrays)
- [ ] Loading state passed from data hook via `isLoading` prop
- [ ] **NO placeholder data arrays (PLACEHOLDER_ITEMS pattern is forbidden)**
- [ ] **For lists: pass `isLoading` to each list item component, handle skeleton internally**
