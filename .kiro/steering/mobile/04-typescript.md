---
inclusion: fileMatch
fileMatchPattern: ["**/*.tsx", "**/*.ts"]
---
# TypeScript and Props

## Props Interface Rules

1. **ALL interfaces MUST be defined in `types.ts` files**
2. **ALL interfaces MUST be exported from `types.ts`** (component props AND styled component props)
3. **NEVER define interfaces inline in component files (`.tsx` files)**
4. **Component directories MAY have a `types.ts` file** (optional if empty)

## Types File Extension

**Always use `.ts` extension for types files, NOT `.tsx`.**

Types files contain only TypeScript interfaces and types - no JSX. Using `.ts` makes this clear and follows TypeScript conventions.

```
ComponentName/
  ├── ComponentName.tsx   ← Component with JSX uses .tsx
  ├── types.ts            ← Types without JSX uses .ts  ✅
  └── index.ts            ← Barrel export uses .ts
```

❌ **NEVER** use `types.tsx` - types files don't contain JSX

## Types Organization Strategy

### Optional: types.ts Files (Empty Allowed)

**Component directories MAY have a `types.ts` file, but it's OPTIONAL if the component has no custom types.**

- ✅ **Required** if component has custom interfaces or types
- ✅ **Optional** if component only uses standard types (string, number, etc.)
- ✅ **Empty files allowed** - don't delete empty types.ts files (serves as documentation)

If a component has no custom types, it may omit the types.ts file entirely.

### Shared/Common Types

**For types shared across multiple features or components, create them in common locations:**

1. **Domain types** (Product, User, Cart, etc.) → `src/types/`
2. **Common component prop types** → `src/components/common/types.ts`
3. **Provider/context types** → In the provider's `types.ts`

### Component-Specific Types

**For types used only within a component hierarchy:**

1. **Parent component types** → Parent's `types.ts`
2. **Child component types that extend parent** → Child's `types.ts` extends parent types
3. **Types passed down unchanged** → Defined in parent, imported by child

### Type Extension Rules

**⚠️ CRITICAL: When extending parent types, child types MUST NOT include unused props.**

```tsx
// Child/types.ts
import { ParentItemProps } from '../types';

// ✅ CORRECT - Child only adds what it needs
// Parent/types.ts
export interface ParentItemProps {
  id: string;
  title: string;
  price: number;
}

// Child extends and adds its own specific prop
export interface ChildItemProps extends ParentItemProps {
  showDetails: boolean; // ✅ Child-specific prop
}

// Usage in Child:
// Child receives all parent props + showDetails
// Child uses ALL props it receives
```

```tsx
// ❌ WRONG - Child extends but doesn't use inherited props
// Parent/types.ts
export interface ParentItemProps {
  id: string;
  title: string;
  price: number;
  category: string; // Parent uses this
}

// Child/types.ts
export interface ChildItemProps extends ParentItemProps {
  showDetails: boolean;
}

// ❌ BAD - Child doesn't use 'category' from parent
// Don't extend if you won't use all inherited props!
```

### When to Use Extension vs. Separate Types

**✅ Extend when:**

- Child component needs ALL parent props
- Child adds additional props on top
- There's a clear parent-child data flow

**✅ Create separate types when:**

- Child only needs SOME of the parent's props
- Types are semantically different
- No clear inheritance relationship

## ✅ CORRECT Pattern

**Box.tsx:**

```tsx
import React from 'react';
import { View } from 'react-native';
import styled from 'styled-components/native';

import { useColors } from 'src/providers/Theme/ThemeProvider';

import { BoxProps, StyledBoxProps } from './types';

const StyledBox = styled(View)<StyledBoxProps>`
  background-color: ${({ backgroundColor }) => backgroundColor};
  width: ${({ size }) => size}px;
  height: ${({ size }) => size}px;
`;

export default function Box({ checked, size = 24 }: BoxProps) {
  const colors = useColors();
  const backgroundColor = checked ? colors.primary : colors.border;

  return <StyledBox backgroundColor={backgroundColor} size={size} />;
}
```

**types.ts:**

```tsx
// Component props - exported
export interface BoxProps {
  checked: boolean;
  size?: number;
}

// Styled component props - also exported
export interface StyledBoxProps {
  backgroundColor: string;
  size: number;
}
```

## Shared Domain Types

For types shared across multiple features (e.g., Product, Cart, Collection), use `src/types/`:

**src/types/product.ts:**

```tsx
export interface Product {
  id: string;
  title: string;
  price: number;
  imageUrl: string;
  category: string;
}

export interface ProductVariant {
  id: string;
  title: string;
  price: number;
  available: boolean;
}
```

**Usage in components:**

```tsx
// Import shared domain types
import { Product } from 'src/types/product';

// Component types reference domain types
export interface ProductCardProps {
  product: Product; // ✅ Reusing shared type
  onPress: (productId: string) => void;
}
```

## Common Component Types

For common components used across features, define shared prop types:

**src/components/common/types.ts:**

```tsx
// Base props that many common components share
export interface BaseComponentProps {
  testID?: string;
  accessible?: boolean;
}

// Common text props
export interface CommonTextProps extends BaseComponentProps {
  color?: string;
  fontSize?: number;
}

// Common button props
export interface CommonButtonProps extends BaseComponentProps {
  onPress: () => void;
  disabled?: boolean;
}
```

## Type Naming Conventions

- **Component props**: `{ComponentName}Props`
  - Example: `ProductCardProps`, `SearchContentBarProps`
- **Styled component props**: `Styled{ElementName}Props` or `{ComponentName}StyledProps`
  - Example: `StyledButtonProps`, `ProductCardContainerStyledProps`

- **Shared domain types**: Singular noun (PascalCase)
  - Example: `Product`, `User`, `CartItem`

- **Always use PascalCase**
- **Be descriptive and specific**

## Type Import Patterns

### ✅ CORRECT - Importing Types

```tsx
// From same component
// From common components
import { CommonButtonProps } from 'src/components/common/types';
// From shared types
import { Product } from 'src/types/product';

// From sibling component (direct import, not through index.ts)
import { SiblingProps } from '../Sibling/types';
// From parent component
import { ParentItemProps } from '../types';
import { CheckboxProps, CheckboxContainerStyledProps } from './types';
```

### ❌ INCORRECT - Type Exports from Component Files

```tsx
// ❌ DON'T DO THIS - Exporting types from component file
// ProductCard.tsx
export interface ProductCardProps {
  // ❌ Wrong file!
  product: Product;
}

export default function ProductCard({ product }: ProductCardProps) {
  // ...
}
```

**✅ CORRECT - Types ONLY in types.ts:**

```tsx
// ProductCard.tsx
import { ProductCardProps } from './types';

// types.ts
export interface ProductCardProps {
  // ✅ In types.ts
  product: Product;
}

// ✅ Import from types.ts

export default function ProductCard({ product }: ProductCardProps) {
  // ...
}
```

## Type Safety Rules

1. ✅ **ALWAYS** define all interfaces in `types.ts` files (when types.ts exists)
2. ✅ **ALWAYS** export all interfaces (component AND styled props)
3. ✅ Use type extension (`extends`) only when child uses ALL parent props
4. ✅ `types.ts` files are optional when empty (no custom types needed)
5. ✅ Import types from `./types`, never from component files
6. ❌ **NEVER** define interfaces inline in `.tsx` files
7. ❌ **NEVER** export types from component files
8. ❌ **NEVER** extend parent types and leave props unused

## Performance Note

**TypeScript types have ZERO runtime cost.** Don't worry about creating many small, specific interfaces. Proper typing improves development experience and catches bugs without affecting performance.
