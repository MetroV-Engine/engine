---
inclusion: fileMatch
fileMatchPattern: ["**/*.tsx", "**/*.ts"]
---
# Import and Export Patterns

## Import Organization

Organize imports in the following order:

```tsx
// 1. React and React Native
import React from 'react';
import { View, Text, TouchableOpacity } from 'react-native';

// 2. External libraries
import styled from 'styled-components/native';
import { useTranslation } from 'react-i18next';

// 3. Internal components (from components/)
import { Text, Button, Background } from 'src/components/common';
import CollectionFilterBarModalStockCheckboxBox from './Box/Box';

// 4. Hooks (from providers/ or hooks/)
import { useColors } from 'src/providers/Theme/ThemeProvider';
import { useCurrency } from 'src/hooks/useCurrency';

// 5. Local imports (from ./ or ../)
import { CheckboxProps } from './types';
import { formatPrice } from '../utils';
```

## Import Rules

1. ✅ Group imports by category with blank lines between
2. ✅ Use absolute imports for `src/` paths
3. ✅ Use relative imports for local files
4. ✅ Sort alphabetically within each group (optional but recommended)
5. ✅ Use barrel exports when available

## Export Pattern

### Barrel Exports with `index.ts`

Every component directory must have an `index.ts` file for clean exports.

### Component Directory Export

```tsx
// ComponentName/index.ts
export { default } from './ComponentName';
export type { ComponentNameProps } from './ComponentName';

// If component has sub-components
export { default as ComponentNameSubComponent } from './SubComponent';
export type { ComponentNameSubComponentProps } from './SubComponent';
```

### Example: Checkbox Component

```tsx
// Checkbox/index.ts
export { default } from './Checkbox';
export type { CollectionFilterBarModalStockCheckboxProps } from './types';
export { default as CollectionFilterBarModalStockCheckboxBox } from './Box';
export { default as CollectionFilterBarModalStockCheckboxLabel } from './Label';
```

### Common Components Barrel Export

```tsx
// src/components/common/index.ts
export { default as Text } from './Text';
export { default as Button } from './Button';
export { default as Background } from './Background';
export { default as Pressable } from './Pressable';
export { default as Loading } from './Loading';
// ... etc
```

## Export Rules

1. ✅ Always export default component
2. ✅ Always export component prop types
3. ✅ Use named exports for sub-components
4. ✅ Create barrel exports for feature directories
5. ❌ Don't export styled component prop interfaces
