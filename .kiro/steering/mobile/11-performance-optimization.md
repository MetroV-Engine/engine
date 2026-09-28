---
inclusion: auto
name: mobile-performance
description: Performance optimization, memoization, useMemo, useCallback, React.memo, preventing re-renders
---
# Performance Optimization Patterns

**⚠️ CRITICAL: Performance is the TOP priority. Always optimize even if it makes code longer or more complex.**

This application MUST be VERY fast. Every component, hook, and function should be optimized for minimal re-renders and maximum performance.

## Core Performance Rules

1. **Memoize computed values** with `useMemo`
2. **Memoize callback functions** with `useCallback`
3. **Optimize expensive operations** - don't recalculate on every render
4. **Minimize re-renders** - components should only update when necessary
5. **Use proper dependency arrays** - be explicit and complete

## useMemo - Memoizing Computed Values

**Use `useMemo` for:**

- Derived data from props/state
- Filtered or transformed arrays
- Expensive calculations
- Object/array literals passed as props
- Style objects

### ✅ CORRECT Patterns

```tsx
export default function MyFeatureList({ items, filter }: MyFeatureListProps) {
  // ✅ Memoize filtered data
  const filteredItems = useMemo(() => {
    return items.filter((item) => item.category === filter);
  }, [items, filter]);

  // ✅ Memoize computed styles
  const containerStyle = useMemo(
    () => ({
      width: items.length * 100,
      height: '100%',
    }),
    [items.length],
  );

  // ✅ Memoize expensive calculations
  const totalPrice = useMemo(() => {
    return items.reduce((sum, item) => sum + item.price, 0);
  }, [items]);

  return (
    <Container style={containerStyle}>
      {filteredItems.map((item) => (
        <MyFeatureListItem key={item.id} item={item} />
      ))}
      <TotalText>{totalPrice}</TotalText>
    </Container>
  );
}
```

### ❌ INCORRECT - Recalculating on Every Render

```tsx
export default function MyFeatureList({ items, filter }: MyFeatureListProps) {
  // ❌ BAD - Filtered on every render even if items/filter unchanged
  const filteredItems = items.filter((item) => item.category === filter);

  // ❌ BAD - New object created on every render
  const containerStyle = {
    width: items.length * 100,
    height: '100%',
  };

  // ❌ BAD - Expensive calculation on every render
  const totalPrice = items.reduce((sum, item) => sum + item.price, 0);

  return (
    <Container style={containerStyle}>
      {filteredItems.map((item) => (
        <MyFeatureListItem key={item.id} item={item} />
      ))}
      <TotalText>{totalPrice}</TotalText>
    </Container>
  );
}
```

## useCallback - Memoizing Functions

**Use `useCallback` for:**

- Event handlers passed as props
- Functions passed to child components
- Functions used in dependency arrays
- Callback functions passed to hooks

### ✅ CORRECT Patterns

```tsx
export default function MyFeatureCard({ item, onUpdate }: MyFeatureCardProps) {
  const [quantity, setQuantity] = useState(1);

  // ✅ Memoize event handlers
  const handleIncrement = useCallback(() => {
    setQuantity((prev) => prev + 1);
  }, []); // Empty deps - doesn't depend on external values

  const handleDecrement = useCallback(() => {
    setQuantity((prev) => Math.max(1, prev - 1));
  }, []); // Empty deps - uses functional update

  const handleUpdate = useCallback(() => {
    onUpdate(item.id, quantity);
  }, [item.id, quantity, onUpdate]); // Deps: external values used

  const handlePress = useCallback(
    (newValue: number) => {
      setQuantity(newValue);
      onUpdate(item.id, newValue);
    },
    [item.id, onUpdate],
  ); // Deps: external dependencies

  return (
    <Container>
      <MyFeatureCardQuantity
        value={quantity}
        onIncrement={handleIncrement}
        onDecrement={handleDecrement}
      />
      <MyFeatureCardButton onPress={handleUpdate} />
    </Container>
  );
}
```

### ❌ INCORRECT - Creating New Functions on Every Render

```tsx
export default function MyFeatureCard({ item, onUpdate }: MyFeatureCardProps) {
  const [quantity, setQuantity] = useState(1);

  // ❌ BAD - New function created on every render
  const handleIncrement = () => {
    setQuantity(prev => prev + 1);
  };

  // ❌ BAD - Child components re-render unnecessarily
  const handleUpdate = () => {
    onUpdate(item.id, quantity);
  };

  return (
    <Container>
      <MyFeatureCardQuantity
        value={quantity}
        onIncrement={handleIncrement}  {/* New reference every render */}
      />
      <MyFeatureCardButton onPress={handleUpdate} />  {/* New reference every render */}
    </Container>
  );
}
```

## useEffect - Side Effects Only

**Use `useEffect` ONLY for:**

- API calls and data fetching
- Subscriptions (WebSocket, event listeners)
- DOM manipulation (rare in React Native)
- Setting up/cleaning up side effects
- Synchronizing with external systems

**DO NOT use `useEffect` for:**

- Computing derived state (use `useMemo` instead)
- Updating state based on prop changes (use `useMemo` or update in event handler)
- Transforming data (use `useMemo` instead)

### ✅ CORRECT Patterns

```tsx
export default function MyFeaturePage({ productId }: MyFeaturePageProps) {
  const [data, setData] = useState(null);
  const [loading, setLoading] = useState(true);

  // ✅ CORRECT - Data fetching side effect
  useEffect(() => {
    let cancelled = false;

    const fetchData = async () => {
      setLoading(true);
      try {
        const result = await api.getProduct(productId);
        if (!cancelled) {
          setData(result);
        }
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    };

    fetchData();

    // Cleanup function
    return () => {
      cancelled = true;
    };
  }, [productId]);

  // ✅ CORRECT - Event listener with cleanup
  useEffect(() => {
    const handleResize = () => {
      console.log('Window resized');
    };

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
    };
  }, []); // Empty deps - only run once

  return <MyFeatureContent data={data} loading={loading} />;
}
```

### ❌ INCORRECT - Misusing useEffect

```tsx
export default function MyFeaturePage({ items, filter }: MyFeaturePageProps) {
  const [filteredItems, setFilteredItems] = useState([]);

  // ❌ BAD - Don't use useEffect for derived state
  useEffect(() => {
    setFilteredItems(items.filter((item) => item.category === filter));
  }, [items, filter]);

  // ✅ SHOULD BE - Use useMemo instead
  // const filteredItems = useMemo(() =>
  //   items.filter(item => item.category === filter),
  //   [items, filter]
  // );

  return <MyFeatureContent items={filteredItems} />;
}
```

## React.memo - Preventing Unnecessary Re-renders

**Use `React.memo` for:**

- Components that receive stable props
- Leaf components rendered in lists
- Components that are expensive to render
- Components that don't need to update often

### ✅ CORRECT Pattern

```tsx
// Wrap leaf components with React.memo
export default React.memo(function MyFeatureListItem({
  item,
  onPress,
}: MyFeatureListItemProps) {
  const colors = useColors();
  const { t } = useTranslation();

  // Memoize handler
  const handlePress = useCallback(() => {
    onPress(item.id);
  }, [item.id, onPress]);

  return (
    <Container>
      <MyFeatureListItemImage imageUrl={item.imageUrl} />
      <MyFeatureListItemText text={item.title} />
      <MyFeatureListItemButton onPress={handlePress} />
    </Container>
  );
});

// Custom comparison function for complex props
export default React.memo(
  function MyFeatureCard({ item, metadata }: MyFeatureCardProps) {
    // Component implementation
  },
  (prevProps, nextProps) => {
    // Return true if props are equal (skip re-render)
    return (
      prevProps.item.id === nextProps.item.id &&
      prevProps.item.title === nextProps.item.title &&
      prevProps.metadata.timestamp === nextProps.metadata.timestamp
    );
  },
);
```

## Complete Optimization Example

```tsx
import React, { useState, useMemo, useCallback } from 'react';
import { FlatList } from 'react-native';

export default function ProductList({
  products,
  category,
  onProductPress,
}: ProductListProps) {
  const [sortBy, setSortBy] = useState<'price' | 'name'>('name');

  // ✅ Memoize filtered products
  const filteredProducts = useMemo(() => {
    return products.filter((product) => product.category === category);
  }, [products, category]);

  // ✅ Memoize sorted products
  const sortedProducts = useMemo(() => {
    return [...filteredProducts].sort((a, b) => {
      if (sortBy === 'price') {
        return a.price - b.price;
      }
      return a.name.localeCompare(b.name);
    });
  }, [filteredProducts, sortBy]);

  // ✅ Memoize handlers
  const handleSortByPrice = useCallback(() => {
    setSortBy('price');
  }, []);

  const handleSortByName = useCallback(() => {
    setSortBy('name');
  }, []);

  // ✅ Memoize render function
  const renderItem = useCallback(
    ({ item }) => <ProductListItem item={item} onPress={onProductPress} />,
    [onProductPress],
  );

  // ✅ Memoize key extractor
  const keyExtractor = useCallback((item: Product) => item.id, []);

  return (
    <Container>
      <ProductListSortButtons
        onPricePress={handleSortByPrice}
        onNamePress={handleSortByName}
        activeSort={sortBy}
      />
      <FlatList
        data={sortedProducts}
        renderItem={renderItem}
        keyExtractor={keyExtractor}
      />
    </Container>
  );
}

// ✅ Memoize list item to prevent unnecessary re-renders
const ProductListItem = React.memo(function ProductListItem({
  item,
  onPress,
}: ProductListItemProps) {
  const handlePress = useCallback(() => {
    onPress(item.id);
  }, [item.id, onPress]);

  return (
    <ItemContainer>
      <ProductListItemImage imageUrl={item.imageUrl} />
      <ProductListItemDetails name={item.name} price={item.price} />
      <ProductListItemButton onPress={handlePress} />
    </ItemContainer>
  );
});
```

## Performance Checklist

When writing any component, verify:

- [ ] All computed values use `useMemo`
- [ ] All event handlers use `useCallback`
- [ ] All expensive operations are memoized
- [ ] Functions passed as props are memoized with `useCallback`
- [ ] Objects/arrays passed as props are memoized with `useMemo`
- [ ] List item components are wrapped with `React.memo`
- [ ] `useEffect` is only used for side effects, not derived state
- [ ] Dependency arrays are complete and correct
- [ ] No unnecessary re-renders (use React DevTools Profiler to verify)

## Common Performance Mistakes

### ❌ Creating Objects/Arrays in Render

```tsx
// ❌ BAD - New object every render
<MyComponent style={{ width: 100 }} />;

// ✅ GOOD - Memoized style
const style = useMemo(() => ({ width: 100 }), []);
<MyComponent style={style} />;
```

### ❌ Inline Functions as Props

```tsx
// ❌ BAD - New function every render
<Button onPress={() => handleClick(id)} />;

// ✅ GOOD - Memoized function
const handlePress = useCallback(() => handleClick(id), [id, handleClick]);
<Button onPress={handlePress} />;
```

### ❌ Expensive Calculations Without useMemo

```tsx
// ❌ BAD - Expensive calculation every render
const total = items.reduce((sum, item) => sum + (item.price * item.quantity), 0);

// ✅ GOOD - Memoized calculation
const total = useMemo(
  () => items.reduce((sum, item) => sum + (item.price * item.quantity), 0),
  [items]
);
```

**Remember: It's better to have verbose, optimized code than simple, slow code. ALWAYS optimize for performance.**
