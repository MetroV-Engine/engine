---
inclusion: auto
name: mobile-i18n
description: Adding translations, internationalization, or working with text content in mobile components.
---

# Internationalization (i18n) and Translations

## Overview

This application uses `react-i18next` for internationalization. **ALL user-facing text MUST be translated** - there should be **NO hardcoded text** anywhere in the codebase.

## Translation Setup

Translation files are located in `src/i18n/locales/`:

```
src/i18n/
  ├── index.ts           # i18n configuration
  ├── exports.ts         # Export utilities
  ├── types.ts           # TypeScript types
  └── locales/
      ├── en.json        # English translations
      ├── de.json        # German translations
      └── fr.json        # French translations
```

## Using Translations in Components

**ALWAYS** use the `useTranslation` hook to access translations:

```tsx
import { useTranslation } from 'react-i18next';

export default function MyComponent() {
  const { t } = useTranslation();

  return (
    <Container>
      <Title>{t('welcome')}</Title>
      <Button label={t('add_to_bag')} onPress={handlePress} />
    </Container>
  );
}
```

## Adding New Translations

When adding new text, you MUST:

1. Add the translation key to ALL locale files (`en.json`, `de.json`, `fr.json`)
2. Use the translation key in your component via `t('key_name')`

**en.json:**

```json
{
  "my_new_text": "My new text",
  "button_label": "Click me"
}
```

**de.json:**

```json
{
  "my_new_text": "Mein neuer Text",
  "button_label": "Klick mich"
}
```

**fr.json:**

```json
{
  "my_new_text": "Mon nouveau texte",
  "button_label": "Cliquez-moi"
}
```

## Translation Key Naming Conventions

- Use `snake_case` for all translation keys
- Be descriptive but concise
- Group related translations with common prefixes
- Create separate keys for accessibility labels

```json
{
  "cart": "Cart",
  "cart_empty": "Your cart is empty",
  "cart_add_item": "Add item to cart",
  "cart_remove_item": "Remove item from cart",
  "cart_button_add_label": "Add item to cart button",
  "checkout": "Checkout",
  "checkout_confirm": "Confirm checkout"
}
```

## Accessibility Labels

**⚠️ CRITICAL: Accessibility labels (aria-label, accessibilityLabel) MUST use `t()` for translations.**

Screen reader users deserve the same multi-language experience as visual users.

### ✅ CORRECT - Translated Accessibility Labels

```tsx
export default function ProductCardButton() {
  const { t } = useTranslation();

  return (
    <Button
      text={t('add_to_cart')}
      onPress={handlePress}
      accessibilityLabel={t('add_to_cart_accessibility')} // ✅ Translated
      accessibilityHint={t('add_to_cart_hint')} // ✅ Translated
    />
  );
}
```

**Corresponding locale files:**

```json
{
  "add_to_cart": "Add to Cart",
  "add_to_cart_accessibility": "Add product to shopping cart",
  "add_to_cart_hint": "Adds this product to your cart for purchase"
}
```

### ❌ INCORRECT - Hardcoded Accessibility Labels

```tsx
export default function ProductCardButton() {
  const { t } = useTranslation();

  return (
    <Button
      text={t('add_to_cart')}
      onPress={handlePress}
      accessibilityLabel='Add product to cart' // ❌ Hardcoded!
    />
  );
}
```

## Development/Debugging Strings

**Console.log and development debugging strings CAN be hardcoded** since they are not user-facing.

However, **remove all console.log statements before committing** to keep production code clean.

### ✅ CORRECT - Development Logging

```tsx
export default function ProductCard({ product }: ProductCardProps) {
  // ✅ ALLOWED - Development debugging (but remove before commit)
  console.log('Product loaded:', product.id);
  console.error('Failed to load image:', product.imageUrl);

  // Development-only assertions
  if (__DEV__) {
    console.warn('Product price is zero:', product.price === 0);
  }

  return <ProductCardContent product={product} />;
}
```

### Best Practice - Remove Before Commit

Use eslint rules to catch console.log statements:

```bash
# Run linting before committing
npm run lint:check

# Auto-fix will remove console.logs if configured
npm run lint:fix
```

## ❌ NEVER Hardcode Text

```tsx
// ❌ DON'T DO THIS - Hardcoded strings
<Text>Welcome to our store</Text>
<Button label="Add to Cart" />
<Title>Filter & Sort</Title>

// ❌ DON'T DO THIS - String literals in props
handleOpen(content, ['auto'], 'Filter & Sort');

// ❌ DON'T DO THIS - Hardcoded accessibility label
<Button accessibilityLabel="Click to add to cart" />
```

## ✅ ALWAYS Use Translations

```tsx
// ✅ CORRECT - Using translations
const { t } = useTranslation();

<Text>{t('welcome')}</Text>
<Button label={t('add_to_bag')} />
<Title>{t('filter_and_sort')}</Title>

// ✅ CORRECT - Translated strings in function calls
handleOpen(content, ['auto'], t('filter_and_sort'));

// ✅ CORRECT - Translated accessibility label
<Button accessibilityLabel={t('add_to_cart_label')} />
```

## Dynamic Content with Interpolation

For text with dynamic values, use interpolation:

**en.json:**

```json
{
  "items_count": "{{count}} items",
  "welcome_user": "Welcome, {{name}}!",
  "price_amount": "${{amount}}"
}
```

**Component:**

```tsx
const { t } = useTranslation();

<Text>{t('items_count', { count: 5 })}</Text>
<Text>{t('welcome_user', { name: userName })}</Text>
<Text>{t('price_amount', { amount: product.price.toFixed(2) })}</Text>
```

## Pluralization

For pluralized text:

**en.json:**

```json
{
  "item": "{{count}} item",
  "item_plural": "{{count}} items"
}
```

**Component:**

```tsx
<Text>{t('item', { count: itemCount })}</Text>
```

## Translation Performance Optimization

**⚠️ Memoize translated strings when used in performance-critical contexts:**

```tsx
export default function ProductCard({ product }: ProductCardProps) {
  const { t } = useTranslation();

  // ✅ Memoize static translations
  const addToCartLabel = useMemo(() => t('add_to_cart'), [t]);

  // ✅ Memoize dynamic translations
  const priceText = useMemo(
    () => t('price_amount', { amount: product.price.toFixed(2) }),
    [t, product.price],
  );

  return (
    <Container>
      <ProductCardPrice text={priceText} />
      <ProductCardButton label={addToCartLabel} onPress={handlePress} />
    </Container>
  );
}
```

## Translation Rules Summary

1. ✅ **ALWAYS** use `useTranslation()` hook and `t()` function
2. ✅ Add translations to ALL locale files when adding new text
3. ✅ Use `snake_case` for translation keys
4. ✅ Use interpolation for dynamic content
5. ✅ **Translate accessibility labels** for screen readers
6. ✅ Memoize translations in performance-critical components
7. ✅ Create separate keys for accessibility (e.g., `button_label` vs `button_accessibility_label`)
8. ❌ **NEVER** hardcode user-facing text strings
9. ❌ **NEVER** use string literals directly in JSX or props
10. ❌ **NEVER** hardcode accessibility labels
11. ❌ **NEVER** forget to add translations for other languages
12. ✅ **CAN** hardcode console.log debugging strings (but remove before commit)

## Quick Reference Checklist for Translations

When adding or modifying components with text:

- [ ] All visible text uses `t('key')` function
- [ ] All accessibility labels use `t('key')` function
- [ ] Translation key exists in `en.json`
- [ ] Translation key exists in `de.json`
- [ ] Translation key exists in `fr.json`
- [ ] Dynamic values use interpolation `{{ variable }}`
- [ ] No hardcoded strings anywhere in the component
- [ ] Translations are memoized if used in performance-critical context
- [ ] All console.log statements removed before commit
