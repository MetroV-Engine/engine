---
inclusion: fileMatch
fileMatchPattern: ["**/*.tsx", "**/*.ts"]
---
# Styling with styled-components

## Inline Styled Components Only

All styled components **MUST** be defined in the same file as the component that uses them. **Never create separate `styles.ts` files.**

## ✅ CORRECT Pattern

```tsx
import React from 'react';
import { View } from 'react-native';
import styled from 'styled-components/native';

import { useColors } from 'src/providers/Theme/ThemeProvider';

import { CheckboxProps, CheckboxContainerStyledProps } from './types';

// 1. Create styled components with types from types.ts
const CheckboxContainer = styled(View)<CheckboxContainerStyledProps>`
  flex-direction: row;
  align-items: center;
  gap: 8px;
  padding: 8px 0;
  border-color: ${({ borderColor }) => borderColor};
`;

// 2. Component implementation
export default function Checkbox({ checked, onToggle }: CheckboxProps) {
  const colors = useColors();

  return (
    <CheckboxContainer borderColor={colors.border} onPress={onToggle}>
      {/* Component content */}
    </CheckboxContainer>
  );
}
```

## Styled Component Syntax Rule

**ALWAYS use `styled(Component)` syntax, NEVER use `styled.element` syntax.**

```tsx
// ✅ CORRECT - Use styled(View), styled(Text), etc.
import { View, Text, TouchableOpacity } from 'react-native';
import styled from 'styled-components/native';

const Container = styled(View)`
  flex: 1;
`;

const Title = styled(Text)`
  font-size: 16px;
`;

const Button = styled(TouchableOpacity)`
  padding: 8px;
`;

// ❌ WRONG - NEVER use styled.View, styled.Text, etc.
const Container = styled.View`
  flex: 1;
`;

const Title = styled.Text`
  font-size: 16px;
`;
```

**Why**: The `styled(Component)` syntax is explicit, consistent, and works correctly with React Native components. The shorthand `styled.element` can cause issues and is not the convention in this codebase.

## Styled Component Prop Rules

### NO `$` Prefix Rule

**All props passed to styled components use plain names WITHOUT the `$` prefix.**

```tsx
// types.ts
export interface StyledTextProps {
  color: string;
  fontSize?: number;
  fontWeight?: string;
}

// Component.tsx
const StyledText = styled(Text)<StyledTextProps>`
  color: ${({ color }) => color};
  font-size: ${({ fontSize }) => fontSize ?? 16}px;
  font-weight: ${({ fontWeight }) => fontWeight ?? 'normal'};
`;

// Usage
<StyledText color={colors.text} fontSize={18} fontWeight='600'>
  Hello
</StyledText>;
```

### Why NO `$` Prefix?

- This codebase uses plain prop names for styled components
- All typed props are defined in `types.ts`
- TypeScript provides type safety without needing transient props

## The `style` Prop - When to Use

**⚠️ The React Native `style` prop should be used ONLY for truly dynamic runtime values.**

### ❌ NEVER Use `style` Prop For Static Values

```tsx
// ❌ WRONG - Static values should be in styled-component
<View style={{ padding: 16, backgroundColor: '#fff' }}>
```

### ✅ Use Styled Components for Static Styles

```tsx
// ✅ CORRECT - Static styles in styled-component
const Container = styled(View)`
  padding: 16px;
  background-color: ${({ backgroundColor }) => backgroundColor};
`;

<Container backgroundColor={colors.background}>
```

### ✅ Use `style` Prop ONLY for Dynamic Runtime Values

**The `style` prop is ALLOWED when values are calculated at runtime and cannot be known in advance:**

```tsx
// ✅ CORRECT - Dynamic width calculated from data
export default function ProductList({ items }: ProductListProps) {
  // Width depends on number of items (runtime data)
  const dynamicStyle = useMemo(
    () => ({
      width: items.length * ITEM_WIDTH,
    }),
    [items.length],
  );

  return <ScrollContainer style={dynamicStyle}>{/* ... */}</ScrollContainer>;
}

// ✅ CORRECT - Position from user interaction
export default function DraggableItem({ position }: DraggableItemProps) {
  const animatedStyle = useMemo(
    () => ({
      transform: [{ translateX: position.x }, { translateY: position.y }],
    }),
    [position.x, position.y],
  );

  return <DragContainer style={animatedStyle}>{/* ... */}</DragContainer>;
}

// ✅ CORRECT - Responsive sizing
export default function ResponsiveGrid() {
  const { width } = useWindowDimensions();

  const gridStyle = useMemo(
    () => ({
      width: width - 32, // Full width minus padding
      columnCount: Math.floor(width / 150),
    }),
    [width],
  );

  return <GridContainer style={gridStyle}>{/* ... */}</GridContainer>;
}
```

### Rule Summary: Static vs Dynamic Styles

**Static Styles (known at write-time):**

- Padding, margins (fixed values)
- Font sizes, weights
- Fixed colors (even from theme)
- Border radius, border width
  → **Use styled-components with props**

**Dynamic Styles (calculated at runtime):**

- Values from calculations (width based on data length)
- Animation transforms
- Positions from user interaction (drag & drop)
- Responsive values from window dimensions
  → **Use `style` prop with memoized style objects**

## Theme and Colors

### The `useColors()` Hook

**ALWAYS** use the `useColors()` hook from `src/providers/Theme/ThemeProvider` to access theme colors.

### ✅ CORRECT Pattern

```tsx
import { useColors } from 'src/providers/Theme/ThemeProvider';

export default function MyComponent() {
  const colors = useColors();

  return (
    <Container backgroundColor={colors.background}>
      <Title color={colors.text}>Hello</Title>
      <Subtitle color={colors.textSecondary}>World</Subtitle>
    </Container>
  );
}
```

### ❌ INCORRECT Pattern - NEVER USE HARDCODED COLORS

**CRITICAL: Hardcoded colors are STRICTLY FORBIDDEN in ALL circumstances.**

```tsx
// ❌ NEVER DO THIS - Hardcoded hex colors
<Container backgroundColor="#ffffff">
  <Title color="#000000">Hello</Title>
</Container>

// ❌ NEVER DO THIS - Hardcoded colors in styled-components
const ErrorText = styled(Text)`
  color: #DC2626;  // ❌ WRONG!
`;

// ❌ NEVER DO THIS - Hardcoded colors as fallbacks
backgroundColor={colors.successBackground || '#F0FDF4'}  // ❌ WRONG!

// ❌ NEVER DO THIS - Direct theme access
import { theme } from 'src/theme';
<Container backgroundColor={theme.colors.background}>
```

### ✅ CORRECT - Always Use Theme Colors

```tsx
// ✅ ALWAYS use colors from useColors() or colors.palette
const colors = useColors();

// For standard theme colors:
<Container backgroundColor={colors.background}>
<Title color={colors.text}>Hello</Title>
<ErrorText color={colors.error}>Error message</ErrorText>

// For specific palette colors (when standard theme colors don't exist):
backgroundColor={colors.palette.green[2]}
textColor={colors.palette.red[7]}
```

**Why hardcoded colors are forbidden:**
- They break dark mode / light mode theming
- They create inconsistent visual appearance
- They make the app impossible to maintain or re-theme
- Use `colors.palette.*` when you need specific color shades

## Performance: Memoize Style Objects

**When using the `style` prop with dynamic values, ALWAYS memoize the style object:**

```tsx
export default function MyComponent({ itemCount }: MyComponentProps) {
  // ✅ CORRECT - Memoized style object
  const containerStyle = useMemo(
    () => ({
      width: itemCount * 100,
      height: '100%',
    }),
    [itemCount],
  );

  // ❌ WRONG - New object every render
  // const containerStyle = {
  //   width: itemCount * 100,
  //   height: '100%',
  // };

  return <Container style={containerStyle}>{/* ... */}</Container>;
}
```

## Styling Rules Summary

1. ✅ Define styled components **inline** in the component file
2. ✅ Always use typed interfaces for styled component props from `types.ts`
3. ✅ **ALWAYS** use `styled(Component)` syntax (e.g., `styled(View)`, `styled(Text)`)
4. ✅ **NEVER** use `$` prefix for styled component props
5. ✅ Pass theme colors from `useColors()` as props
6. ✅ Export ALL interfaces (component AND styled) from `types.ts`
7. ✅ Use `style` prop ONLY for truly dynamic runtime values
8. ✅ **ALWAYS memoize** dynamic style objects with `useMemo`
9. ❌ **NEVER** use `styled.View`, `styled.Text`, or any `styled.element` syntax
10. ❌ **NEVER** create separate `styles.ts` files
11. ❌ **NEVER** use inline styles with `style={}` for static values
12. ❌ **NEVER** hardcode colors
13. ❌ **NEVER** define interfaces inline in component files
