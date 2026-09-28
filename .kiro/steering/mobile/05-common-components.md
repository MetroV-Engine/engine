---
inclusion: fileMatch
fileMatchPattern: ["**/*.tsx", "**/*.ts"]
---
# Common Components

## Reusable Common Components

The `src/components/common/` directory contains reusable components that should be used throughout the app.

## Available Common Components

```tsx
// Text rendering
Text; // Base text component with variants
Heading; // Heading component with levels

// Interactions
Button; // Primary button component
Pressable; // Touchable/pressable element
Link; // Link component

// Layouts
Background; // Background with theme support
BackgroundSafeBottom; // Background with safe area bottom

// UI Elements
Loading; // Loading indicator
Error; // Error display
Modal; // Modal component
Collapsible; // Collapsible/accordion component
Selector; // Dropdown/select component

// Loading States
Skeleton; // Auto-skeleton wrapper (shows skeleton when isLoading=true)
SkeletonIgnore; // Excludes content from skeleton rendering

// Forms
Inputs; // Form input components

// Navigation
Navigator; // Navigation component
```

## Usage Pattern

```tsx
import { Text, Button, Background, Loading } from 'src/components/common';

export default function MyComponent() {
  return (
    <Background>
      <Text type='title'>Welcome</Text>
      <Button label='Click Me' onPress={handlePress} />
      <Loading />
    </Background>
  );
}
```

## Common Component Rules

1. ✅ **ALWAYS** check `src/components/common/` before creating new components
2. ✅ Use common components for standard UI elements
3. ✅ Import from barrel export: `import { X } from 'src/components/common'`
4. ✅ **ALWAYS** use `Button` from `src/components/common/` for ALL interactive buttons
5. ✅ Use Button's `variant` prop (primary, secondary, tertiary, transparent, danger) for automatic color styling
6. ✅ Button supports `icon`, `width`, `height`, `disabled`, and `loading` props out of the box
7. ❌ **NEVER** recreate Text, Button, or other common components
8. ❌ **NEVER** create custom Pressable components styled as buttons - use common/Button with variants instead
9. ❌ **NEVER** create custom primitives without checking common components first

## Button Component Variants

The common `Button` component provides built-in variants with automatic theming:

- `primary` - Primary action button (default) - uses `colors.buttonPrimary` background
- `secondary` - Secondary action button - uses `colors.text` background with `colors.background` text
- `tertiary` - Tertiary/outline button - transparent background with border and `colors.text`
- `transparent` - Transparent button - no background or border
- `danger` - Destructive action button - uses `colors.error` background

**Example Usage:**

```tsx
import { LogIn } from 'lucide-react-native';
import { Button } from 'src/components/common';
import { useColors } from 'src/providers/Theme/ThemeProvider';

export default function MyComponent() {
  const colors = useColors();

  return (
    <>
      <Button
        text='Login'
        onPress={handleLogin}
        variant='primary'
        icon={<LogIn size={20} color={colors.background} />}
        width='100%'
      />

      <Button
        text='Sign Up'
        onPress={handleSignup}
        variant='secondary'
        width='100%'
      />

      <Button text='Cancel' onPress={handleCancel} variant='tertiary' />
    </>
  );
}
```

## When to Create a New Common Component

Create a component in `src/components/common/` when:

- It will be reused across multiple features
- It's a fundamental UI primitive
- It requires consistent theming and behavior
- It's feature-agnostic

**Don't** put feature-specific components in `common/`. Use feature directories (`{Feature}-UI/`) instead.
