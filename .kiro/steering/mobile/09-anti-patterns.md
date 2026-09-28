---
inclusion: auto
name: mobile-antipatterns
description: Code review, debugging, common mistakes, anti-patterns to avoid
---
# Anti-Patterns - Things to NEVER Do

## 1. Separate Style Files

```tsx
// ❌ DON'T DO THIS
// styles.ts
export const Container = styled(View)``;

// Component.tsx
import { Container } from './styles';
```

**Why**: Styling should be colocated with components for maintainability.

## 2. Inline Styles

```tsx
// ❌ DON'T DO THIS
<View style={{ backgroundColor: '#fff', padding: 16 }}>
```

**Why**: Use styled-components for all styling to maintain consistency.

## 3. Hardcoded Colors - STRICTLY FORBIDDEN

**⛔ CRITICAL: Hardcoded colors are NEVER allowed under ANY circumstances.**

```tsx
// ❌ NEVER DO THIS - Hardcoded hex colors in styled-components
const Title = styled(Text)`
  color: #000000;
  background-color: #ffffff;
`;

// ❌ NEVER DO THIS - Hardcoded colors in JSX props
<ErrorText color='#DC2626'>Error</ErrorText>

// ❌ NEVER DO THIS - Hardcoded colors as fallbacks
<Container backgroundColor={colors.success || '#22C55E'}>

// ❌ NEVER DO THIS - Hardcoded colors in conditional logic
hasError ? '#DC2626' : borderColor

// ❌ NEVER DO THIS - Hardcoded colors for status indicators
backgroundColor: '#DCFCE7'  // Even "success green" must use theme
```

**✅ ALWAYS use theme colors:**

```tsx
const colors = useColors();

// Use standard theme colors
color={colors.text}
color={colors.error}
color={colors.success}

// Use palette colors for specific shades
backgroundColor={colors.palette.green[2]}
textColor={colors.palette.red[7]}
borderColor={colors.palette.gray[3]}
```

**Why**: 
- Hardcoded colors BREAK dark mode completely
- They create "white on white" or "black on black" bugs
- They make theming and maintenance impossible
- ALWAYS use `colors.*` or `colors.palette.*`

## 4. Children Prop Composition

```tsx
// ❌ DON'T DO THIS
<Container>
  <Title>
    <Text>Hello</Text>
  </Title>
</Container>
```

**Why**: Use flat composition with explicit props instead.

## 5. Generic Component Names

```tsx
// ❌ DON'T DO THIS
// File: BottomBar/Tile/Text/Text.tsx
export default function Text() { ... }
```

**Why**: Use hierarchical naming (e.g., `CollectionFilterBarModalStock`) for clarity.

## 6. Using `$` Prefix on Styled Props

```tsx
// ❌ DON'T DO THIS
interface StyledTextProps {
  $color: string; // Don't use $ prefix
}

const StyledText = styled(Text)<StyledTextProps>`
  color: ${({ $color }) => $color};
`;
```

**Why**: This codebase does NOT use `$` prefix. Use plain prop names.

## 7. Defining Interfaces Inline

```tsx
// ❌ DON'T DO THIS - Interface defined in component file
export interface BoxProps {
  checked: boolean;
}

export default function Box({ checked }: BoxProps) {
  // ...
}
```

**Why**: ALL interfaces must be defined in `types.ts` files, not inline in component files.

## 8. Missing Export for Sub-Components

```tsx
// ❌ DON'T DO THIS - Checkbox component without index.ts
Checkbox/
  ├── Checkbox.tsx
  ├── types.ts
  └── Box/
      ├── Box.tsx
      └── types.ts
```

**Why**: Every component and sub-component must have an `index.ts` for clean exports.

## 9. Not Exporting Styled Prop Interfaces

```tsx
// ❌ DON'T DO THIS - in types.ts
export interface BoxProps {
  checked: boolean;
}

// Missing styled props interface!
```

**Why**: ALL interfaces (component AND styled) must be exported from `types.ts`.

## 10. Creating Custom Primitives

```tsx
// ❌ DON'T DO THIS - Creating custom Text when common/Text exists
const MyText = styled(RNText)``;
```

**Why**: Use existing common components to maintain consistency.

## 11. Creating Custom Button Components

```tsx
// ❌ DON'T DO THIS - Custom styled button instead of common/Button
const CustomButton = styled(Pressable)`
  padding: 16px;
  border-radius: 12px;
  border-width: 1px;
  border-color: ${({ borderColor }) => borderColor};
  background-color: ${({ backgroundColor }) => backgroundColor};
`;

export default function MyComponent() {
  return (
    <CustomButton onPress={handlePress} borderColor={colors.border}>
      <ButtonText>Click Me</ButtonText>
    </CustomButton>
  );
}
```

**Why**: Always use `Button` from `src/components/common/` with the appropriate `variant` prop.

## 12. Direct Theme Access

```tsx
// ❌ DON'T DO THIS
import { theme } from 'src/theme';

const color = theme.colors.text;
```

**Why**: Always use `useColors()` hook for proper theme context.

## 13. Unnecessary Comments

```tsx
// ❌ DON'T DO THIS - GPT-like explanatory comments
// This function handles the user login
// It takes the email and password and validates them
export default function Login() {
  // Initialize the state for loading
  const [loading, setLoading] = useState(false);
  // ...
}
```

**Why**: Code should be self-explanatory. Only use comments for:

- `TODO:` - Tasks to be completed
- `WARNING:` or `WARN:` - Important warnings about the code
- `@eslint-disable` or similar linter directives
- Complex business logic that truly requires explanation

## 14. Inline JSX in Map Iterations

```tsx
// ❌ DON'T DO THIS - Inline JSX elements inside .map()
<Container>
  {items.map((item, index) => (
    <React.Fragment key={index}>
      {index > 0 && (
        <SeparatorText textColor={colors.textSecondary}>{'>'}</SeparatorText>
      )}
      <TouchableItem onPress={() => handlePress(index)}>
        <ItemText textColor={colors.text}>{item.name}</ItemText>
      </TouchableItem>
    </React.Fragment>
  ))}
</Container>
```

**Why**: Each repeated element in a list MUST be its own sub-component.

**Correct approach:**

```tsx
// ✅ DO THIS - Extract to a dedicated sub-component
<Container>
  {items.map((item, index) => (
    <MyFeatureListItem
      key={item.id}
      item={item}
      index={index}
      showSeparator={index > 0}
      onPress={handleItemPress}
    />
  ))}
</Container>
```

## 15. Inline JSX in Multiple Return Statements

```tsx
// ❌ DON'T DO THIS - Inline JSX in conditional returns
export default function MyModal({ onClose }: MyModalProps) {
  const [view, setView] = useState<'main' | 'detail'>('main');

  if (view === 'detail') {
    return (
      <Container>
        <Header>
          <BackButton onPress={() => setView('main')} />
          <Title>Detail View</Title>
        </Header>
        <Content>
          <SomeText>Detail content here</SomeText>
        </Content>
      </Container>
    );
  }

  return (
    <Container>
      <Title>Main View</Title>
      <Button onPress={() => setView('detail')} />
    </Container>
  );
}
```

**Why**: Each conditional return should return a single self-closing sub-component.

## 16. Mixing Display Elements with Sub-Component Calls

```tsx
// ❌ DON'T DO THIS - Container renders direct elements AND calls sub-components
export default function MyFeatureSection() {
  return (
    <Container>
      <Button text='Login' onPress={handleLogin} /> {/* ❌ Direct element */}
      <Button text='Signup' onPress={handleSignup} /> {/* ❌ Direct element */}
      <DividerLine /> {/* ❌ Direct styled element */}
      <Text>or continue as guest</Text> {/* ❌ Direct text */}
    </Container>
  );
}
```

**Why**: A container component should ONLY call sub-components. Each button, divider, and text should be its own sub-component.

## 17. Inline Display Elements Inside Skeleton or SkeletonIgnore

```tsx
// ❌ DON'T DO THIS - Direct display elements inside Skeleton/SkeletonIgnore
export default function OrderDetailsPage({ orderId }: Props) {
  return (
    <Background>
      <Skeleton isLoading={isLoading}>
        <Content>
          <OrderDetailsHeader ... />
          <OrderDetailsSummary ... />
        </Content>
      </Skeleton>
      <SkeletonIgnore>
        <ReorderButtonContainer>
          {message && (
            <Text color={colors.success}>  {/* ❌ Direct Text element! */}
              {message}
            </Text>
          )}
          <Button  {/* ❌ Direct Button element! */}
            text={t('reorder')}
            onPress={handleReorder}
            icon={<RefreshCw size={18} />}  {/* ❌ Direct icon! */}
          />
        </ReorderButtonContainer>
      </SkeletonIgnore>
    </Background>
  );
}
```

**Why**: `Skeleton` and `SkeletonIgnore` are ONLY wrappers. The content inside them must STILL follow all component architecture rules.

## 18. Using styled.element Syntax

```tsx
// ❌ DON'T DO THIS
const Container = styled.View`
  flex: 1;
`;

const Title = styled.Text`
  font-size: 16px;
`;
```

**Why**: Always use `styled(Component)` syntax for consistency.

**Correct approach:**

```tsx
// ✅ DO THIS
import { View, Text } from 'react-native';
import styled from 'styled-components/native';

const Container = styled(View)`
  flex: 1;
`;

const Title = styled(Text)`
  font-size: 16px;
`;
```

## 19. Hardcoded Text Strings

```tsx
// ❌ DON'T DO THIS
<Text>Welcome to our store</Text>
<Button label="Add to Cart" />
handleOpen(content, ['auto'], 'Filter & Sort');
```

**Why**: ALL text must be internationalized using `t()` function.

**Correct approach:**

```tsx
// ✅ DO THIS
const { t } = useTranslation();

<Text>{t('welcome')}</Text>
<Button label={t('add_to_bag')} />
handleOpen(content, ['auto'], t('filter_and_sort'));
```

## 20. Creating Documentation Files

```tsx
// ❌ DON'T DO THIS - Creating README.md, DOCS.md, or other documentation files
// Unless explicitly requested by the user
```

**Why**: NO DOCUMENTATION EXCEPT EXPLICITLY ASKED. Do not create documentation files (README.md, DOCS.md, etc.) unless the user explicitly requests them.
