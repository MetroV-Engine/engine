---
inclusion: fileMatch
fileMatchPattern: ["**/*.tsx", "**/*.ts"]
---
# Directory Structure and Naming

## Component Structure

```
ComponentName/
  ├── SubComponent/
  │   ├── SubComponent.tsx
  │   ├── types.ts
  │   └── index.ts
  ├── ComponentName.tsx
  ├── types.ts
  └── index.ts
```

### Rules
- Directory name = component file name
- Each sub-component gets own directory
- `index.ts` for EXTERNAL exports only
- `types.ts` required (even if empty)
- Styled components inline (no `styles.ts`)

### Import Rules Within Same Component
```tsx
// ❌ WRONG - via index.ts
import SubComponent from './SubComponent';

// ✅ CORRECT - direct file
import SubComponent from './SubComponent/SubComponent';
```

## Hierarchical Naming

Concatenate parent directories up to feature root, INCLUDING grouping dirs.

| Path | Component Name |
|------|----------------|
| `Collection-UI/Content/FilterBar/Modal/Modal.tsx` | `CollectionFilterBarModal` |
| `Cart-UI/CheckoutAuthModal/Views/Login/Login.tsx` | `CartCheckoutAuthModalViewsLogin` |

### Rules
- PascalCase
- Include grouping dirs (`Views/`, `Forms/`, `Buttons/`)
- Never generic names (`Text`, `Icon`, `Button`)

## Feature Directories

```
src/components/
  ├── Collection-UI/    # Collection page
  ├── Product-UI/       # Product page
  ├── Cart-UI/          # Cart page
  └── common/           # Shared components
```

## Grouping Related Components

Multiple sub-components of same type → group into plural directory.

**Before:**
```
AuthModal/
  ├── LoginView/
  └── SignupView/
```

**After:**
```
AuthModal/
  └── Views/
      ├── Login/Login.tsx  → AuthModalViewsLogin
      └── Signup/Signup.tsx → AuthModalViewsSignup
```

## Providers & Hooks

**Providers:** `src/providers/{FeatureName}/{FeatureName}Provider.tsx`
**Hooks:** `src/hooks/use{FeatureName}.ts` or in provider file
