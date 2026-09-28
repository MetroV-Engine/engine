---
inclusion: fileMatch
fileMatchPattern: ["**/*.tsx", "**/*.ts"]
---
# Component Composition Patterns

## The Flat Sub-Component Rule

Components should compose sub-components as **flat, single-line calls with explicit props**. Never use wrapper patterns with `children` props for feature components.

### ✅ CORRECT Pattern

```tsx
export default function CollectionFilterBarModalStockCheckbox({
  checked,
  onToggle,
}: CollectionFilterBarModalStockCheckboxProps) {
  const { t } = useTranslation();

  return (
    <CheckboxContainer onPress={onToggle}>
      <CollectionFilterBarModalStockCheckboxBox checked={checked} />
      <CollectionFilterBarModalStockCheckboxLabel
        text={t('show_out_of_stock_items')}
      />
    </CheckboxContainer>
  );
}
```

### ❌ INCORRECT Pattern

```tsx
// DON'T DO THIS - Using children for composition
export default function CollectionFilterBarModalStockCheckbox({
  checked,
  onToggle,
}: CollectionFilterBarModalStockCheckboxProps) {
  return (
    <CheckboxContainer onPress={onToggle}>
      <CollectionFilterBarModalStockCheckboxBox checked={checked}>
        <CheckboxIcon />
      </CollectionFilterBarModalStockCheckboxBox>
      <CollectionFilterBarModalStockCheckboxLabel>
        <Text>Show out of stock items</Text>
      </CollectionFilterBarModalStockCheckboxLabel>
    </CheckboxContainer>
  );
}
```

## Composition Rules

1. ✅ Sub-components are called as **self-closing tags** with props only
2. ✅ All required data is passed as **explicit props**
3. ✅ Each sub-component handles its own rendering internally
4. ✅ Sub-components are **self-contained** - they render their own children
5. ❌ Don't pass nested JSX content into feature components
6. ❌ Don't nest multiple levels of wrapper components

## The Self-Contained Component Rule

Feature components must be **self-closing one-liners**. They handle their own internal rendering - the parent should never inject content into them.

```tsx
// ✅ CORRECT - Self-closing, self-contained component
// The Box component renders its own CheckIcon internally
<CollectionFilterBarModalStockCheckboxBox checked={checked} />

// ❌ WRONG - Parent is injecting content into the component
// The Box component should handle CheckIcon itself!
<CollectionFilterBarModalStockCheckboxBox checked={checked}>
  <CheckIcon />
</CollectionFilterBarModalStockCheckboxBox>
```

## The Single Display Element Rule (Leaf Components)

**⚠️ CRITICAL RULE: A component is EITHER a "container" that ONLY calls sub-components, OR a "leaf" that renders EXACTLY ONE display element. A component CANNOT be both. There is NO exception to this rule.**

### Two Types of Components:

1. **Container Components** - Compose/orchestrate other components
   - **ONLY** call sub-components with explicit props (self-closing tags)
   - Use ONE styled layout wrapper (View, Container, etc.) to wrap sub-components
   - Do **NOT** render text, icons, inputs, or any display content directly
   - Do **NOT** mix sub-component calls with display elements
   - The body should ONLY contain `<SubComponent />` calls
   - **If you have more than 3 styled components in a file, it's a container, NOT a leaf**
   - **If you have complex useMemo/useCallback logic, it's probably a container**

2. **Leaf Components** - Render exactly ONE display element
   - Render a single `Text`, `Image`, `Icon`, `TextInput`, etc.
   - Do **NOT** have multiple sibling display elements
   - Do **NOT** call other feature sub-components
   - Are the "end of the line" in the component tree
   - **Maximum 1-3 styled components** (usually just a wrapper and the display element)
   - **Minimal logic** - just colors, simple formatting, no complex calculations

### 🚨 LEAF COMPONENT CHECKLIST - ALL must be true:

A component is ONLY a leaf if:

- ✅ It renders EXACTLY ONE display element (`Text`, `TextInput`, `Image`, `Icon`, etc.)
- ✅ It has NO sub-component calls (no `<MySubComponent />`)
- ✅ It has 1-3 styled components maximum
- ✅ It has minimal logic (just colors, simple props)
- ✅ The return statement is simple and obvious

**If ANY of these is false, it's a CONTAINER and must be split into smaller components.**

### Icons from Libraries (lucide-react-native, etc.)

**⚠️ Icons from libraries like `lucide-react-native` are display elements, NOT components from your codebase.**

Icons should be treated like `<Text>` or `<Image>` - they must be wrapped in a properly named feature sub-component:

```tsx
// ❌ WRONG - Direct icon usage in container
export default function SearchContentBar() {
  return (
    <Container>
      <Search size={20} color={colors.textSecondary} />  {/* ❌ Direct icon! */}
      <SearchContentBarInput />
    </Container>
  );
}

// ✅ CORRECT - Icon wrapped in named sub-component
export default function SearchContentBar() {
  return (
    <Container>
      <SearchContentBarIcon />  {/* ✅ Sub-component that renders icon */}
      <SearchContentBarInput />
    </Container>
  );
}

// SearchContentBar/Icon/Icon.tsx
export default function SearchContentBarIcon() {
  const colors = useColors();

  return (
    <IconContainer>
      <Search size={20} color={colors.textSecondary} />
    </IconContainer>
  );
}
```

### ✅ CORRECT - Container with ONLY sub-component calls

```tsx
// SearchContentBar.tsx - CONTAINER component
// Notice: ONLY sub-component calls, NO direct display elements
export default function SearchContentBar({
  value,
  onChangeText,
  onClear,
  onFocus,
  placeholder,
}: SearchContentBarProps) {
  return (
    <Container>
      <SearchContentBarIcon />
      <SearchContentBarInput
        value={value}
        onChangeText={onChangeText}
        onFocus={onFocus}
        placeholder={placeholder}
      />
      {value.length > 0 && <SearchContentBarClear onClear={onClear} />}
    </Container>
  );
}
```

### ❌ INCORRECT - Mixing sub-components with display elements

```tsx
// DON'T DO THIS - Container has direct display elements mixed in
export default function SearchContentBar({ ... }) {
  return (
    <Container>
      <IconContainer>
        <Search size={20} color={colors.textSecondary} />  {/* ❌ Direct icon! */}
      </IconContainer>
      <StyledInput ... />  {/* ❌ Direct input! */}
      {value.length > 0 && (
        <ClearButton onPress={onClear}>
          <X size={20} color={colors.textSecondary} />  {/* ❌ Direct icon! */}
        </ClearButton>
      )}
    </Container>
  );
}
```

### 🚨 COMMON MISTAKE: "Leaf" Components That Are Actually Containers

**These look like leaves but are actually CONTAINERS and must be split:**

```tsx
// ❌ WRONG - This is NOT a leaf, it's a container!
// It has: multiple styled components, complex useMemo, multiple display elements
export default function CardHeader({ lastFourDigits, status, onResend }) {
  const colors = useColors();

  // ❌ Complex logic = container
  const statusConfig = useMemo(() => {
    switch (status) {
      case 'ACTIVE': return { bg: colors.green, text: 'Active' };
      case 'EXPIRED': return { bg: colors.red, text: 'Expired' };
    }
  }, [status, colors]);

  return (
    <Container>  {/* ❌ Multiple siblings = container */}
      <CardNumber>•••• {lastFourDigits}</CardNumber>
      <StatusBadge bg={statusConfig.bg}>
        <Text>{statusConfig.label}</Text>
      </StatusBadge>
      <ResendButton onPress={onResend}>
        <Text>Resend</Text>
      </ResendButton>
    </Container>
  );
}

// ✅ CORRECT - Split into container + leaf sub-components
export default function CardHeader({ lastFourDigits, status, onResend }) {
  return (
    <Container>
      <CardHeaderNumber digits={lastFourDigits} />
      <CardHeaderStatusBadge status={status} />
      <CardHeaderResendButton onPress={onResend} />
    </Container>
  );
}

// CardHeader/Number/Number.tsx - TRUE LEAF
export default function CardHeaderNumber({ digits }) {
  return (
    <StyledText>•••• {digits}</StyledText>
  );
}

// CardHeader/StatusBadge/StatusBadge.tsx - TRUE LEAF
export default function CardHeaderStatusBadge({ status }) {
  const colors = useColors();
  const { t } = useTranslation();

  const config = useMemo(() => {
    switch (status) {
      case 'ACTIVE': return { bg: colors.green, label: t('active') };
      case 'EXPIRED': return { bg: colors.red, label: t('expired') };
    }
  }, [status, colors, t]);

  return (
    <Badge backgroundColor={config.bg}>
      <StyledText>{config.label}</StyledText>
    </Badge>
  );
}

// CardHeader/ResendButton/ResendButton.tsx - TRUE LEAF
export default function CardHeaderResendButton({ onPress }) {
  const { t } = useTranslation();

  return (
    <StyledButton onPress={onPress}>
      <StyledText>{t('resend')}</StyledText>
    </StyledButton>
  );
}
```

**Signs your "leaf" is actually a container:**

- ❌ Has 4+ styled components
- ❌ Has complex useMemo/useCallback with switch statements
- ❌ Renders multiple sibling elements (CardNumber + StatusBadge + Button)
- ❌ Has conditional rendering of multiple elements
- ❌ File is > 60 lines

**If you see these signs, SPLIT IT into container + leaf sub-components!**

## Exception: Common Primitive Components

**⚠️ ONLY common primitive components in `src/components/common/` are allowed to accept `children`.**

These are foundational layout wrappers meant to contain arbitrary content:

- `Background` / `BackgroundSafeBottom` - Layout wrappers
- `Pressable` - Touchable wrapper
- `Modal` - Modal container
- `Collapsible` - Expandable container

**Common components can also accept JSX elements as props:**

- `Button` accepts `icon` prop as `React.ReactNode`
- You can pass icon JSX directly: `<Button icon={<LogIn size={20} />} />`

```tsx
// ✅ ALLOWED - Common primitives wrapping arbitrary content
<Background>
  <MyFeatureComponent />
</Background>

<Pressable onPress={handlePress}>
  <Icon />
  <Text>Click me</Text>
</Pressable>

// ✅ ALLOWED - Common Button with icon prop
import { LogIn } from 'lucide-react-native';

<Button
  text="Login"
  icon={<LogIn size={20} color={colors.background} />}
  onPress={handleLogin}
/>
```

### Common Text Component - ALWAYS Wrap

**⚠️ CRITICAL: The common `Text` component must ALWAYS be wrapped in a feature-specific leaf component that explains WHY the text exists.**

```tsx
// ❌ WRONG - Direct usage of common Text
export default function ProductCard() {
  const { t } = useTranslation();
  const colors = useColors();

  return (
    <Container>
      <Text color={colors.text}>{t('product_title')}</Text>  {/* ❌ Direct usage! */}
    </Container>
  );
}

// ✅ CORRECT - Wrapped in named feature component
export default function ProductCard() {
  return (
    <Container>
      <ProductCardTitle />  {/* ✅ Explains WHY this text exists */}
    </Container>
  );
}

// ProductCard/Title/Title.tsx - LEAF component
export default function ProductCardTitle() {
  const { t } = useTranslation();
  const colors = useColors();

  return (
    <TitleText color={colors.text}>
      {t('product_title')}
    </TitleText>
  );
}

// Where TitleText is a styled(Text) with feature-specific styling
const TitleText = styled(Text)`
  font-size: 18px;
  font-weight: 600;
`;
```

**Why**: The wrapping component name (e.g., `ProductCardTitle`) provides semantic meaning and makes the component tree self-documenting.

## Multiple Returns Pattern

**⚠️ CRITICAL RULE: When a component has multiple `return` statements (e.g., conditional views), each return MUST be a single self-closing sub-component call.**

Components with state-based views (like login/signup/options) should NOT have inline JSX in each return. Instead, extract each view into its own sub-component.

### ✅ CORRECT - Each return is a sub-component

```tsx
// ✅ CORRECT - Extract each view to its own sub-component
export default function CartCheckoutAuthModal({
  onClose,
  onGuestCheckout,
  onAuthSuccess,
}: CartCheckoutAuthModalProps) {
  const [currentView, setCurrentView] = useState<AuthView>('options');

  // ... handler functions ...

  if (currentView === 'login') {
    return (
      <CartCheckoutAuthModalViewsLogin
        onBackPress={handleBackPress}
        onSignupPress={handleSignupPress}
        onSuccess={handleAuthSuccess}
      />
    );
  }

  if (currentView === 'signup') {
    return (
      <CartCheckoutAuthModalViewsSignup
        onBackPress={handleBackPress}
        onLoginPress={handleLoginPress}
        onSuccess={handleAuthSuccess}
      />
    );
  }

  return (
    <Container>
      <CartCheckoutAuthModalButtons
        onLoginPress={handleLoginPress}
        onSignupPress={handleSignupPress}
      />
      <CartCheckoutAuthModalDivider />
      <CartCheckoutAuthModalButtonsGuest onPress={handleGuestCheckout} />
    </Container>
  );
}
```

### Key Rules

1. Each conditional return should return a single self-closing sub-component
2. The sub-component handles ALL its internal rendering (header, content, etc.)
3. The parent component only manages state and passes props
4. The default return (no condition) can have multiple sub-component calls in a container

## Conditional Rendering Patterns

### ✅ CORRECT - Conditional Sub-Components

```tsx
export default function ProductCard({ showBadge, isActive }: ProductCardProps) {
  return (
    <Container>
      <ProductCardImage />
      <ProductCardTitle />
      {showBadge && <ProductCardBadge />} {/* ✅ Conditional sub-component */}
      <ProductCardPrice />
    </Container>
  );
}
```

### ✅ CORRECT - Ternary with Sub-Components

```tsx
export default function ProductCard({ isActive }: ProductCardProps) {
  return (
    <Container>
      <ProductCardImage />
      {isActive ? (
        <ProductCardActiveIcon />  {/* ✅ Sub-component */}
      ) : (
        <ProductCardInactiveIcon />  {/* ✅ Sub-component */}
      )}
      <ProductCardTitle />
    </Container>
  );
}
```

## Inline Functions and Event Handlers

### ✅ CORRECT - Simple Parameter Binding

```tsx
export default function ProductList({ items, onItemPress }: ProductListProps) {
  // ✅ Inline arrow functions are ALLOWED for simple parameter binding
  return (
    <Container>
      {items.map(item => (
        <ProductListItem
          key={item.id}
          item={item}
          onPress={() => onItemPress(item.id)}  {/* ✅ Simple binding */}
        />
      ))}
    </Container>
  );
}
```

### ✅ EVEN BETTER - Memoized for Performance

```tsx
export default function ProductList({ items, onItemPress }: ProductListProps) {
  // For better performance, memoize the handler
  const handleItemPress = useCallback(
    (id: string) => {
      onItemPress(id);
    },
    [onItemPress],
  );

  return (
    <Container>
      {items.map((item) => (
        <ProductListItem
          key={item.id}
          item={item}
          onPress={handleItemPress}
          itemId={item.id}
        />
      ))}
    </Container>
  );
}
```

## Data Flow

```
ParentComponent
  ├── gets data and colors from hooks
  ├── passes data as props to SubComponent1
  ├── passes data as props to SubComponent2
  └── passes data as props to SubComponent3

SubComponent1
  ├── receives only what it needs
  └── can call its own sub-components with explicit props
```
