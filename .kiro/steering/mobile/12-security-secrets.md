---
inclusion: fileMatch
fileMatchPattern: ["package.json", ".env*", "**/*.ts", "**/*.tsx"]
---
# Security and Secrets Management

## Critical Rules

### 1. NO Hardcoded Secrets

**NEVER** commit any of the following to the repository:

- ❌ API keys, tokens, or access credentials
- ❌ Private keys or certificates
- ❌ Passwords or authentication secrets
- ❌ Shop domains or store identifiers
- ❌ Database connection strings
- ❌ OAuth secrets or client secrets
- ❌ Encryption keys
- ❌ Any sensitive configuration values

### 2. Environment Variables Only

**ALL** secrets and sensitive configuration MUST be:

✅ Stored in `.env` files (which MUST be in `.gitignore`)
✅ Accessed via `process.env.VARIABLE_NAME`
✅ Required with explicit error messages if missing
✅ Documented in `.env.example` (with fake/placeholder values only)

### 3. Required Pattern for Secrets

When accessing environment variables for secrets:

```typescript
// ✅ CORRECT - No fallback, clear error if missing
const API_KEY = process.env.EXPO_PUBLIC_API_KEY;

if (!API_KEY) {
  throw new Error(
    'EXPO_PUBLIC_API_KEY environment variable is required. Please set it in your .env file.',
  );
}
```

```typescript
// ❌ WRONG - Hardcoded fallback
const API_KEY = process.env.EXPO_PUBLIC_API_KEY || 'hardcoded-key-value';
```

### 4. Configuration with Fallbacks

Non-sensitive configuration CAN have fallbacks:

```typescript
// ✅ OK - Public configuration with safe default
const API_VERSION = process.env.EXPO_PUBLIC_API_VERSION || '2026-01';
const MAX_RETRIES = process.env.MAX_RETRIES || 3;
```

### 5. .env File Structure

Create a `.env` file in the project root (NOT committed):

```bash
# .env (gitignored)
EXPO_PUBLIC_SHOP_DOMAIN=your-shop.myshopify.com
EXPO_PUBLIC_STOREFRONT_ACCESS_TOKEN=your-actual-token
EXPO_PUBLIC_API_VERSION=2026-01
```

Create a `.env.example` for documentation (committed):

```bash
# .env.example (committed as template)
EXPO_PUBLIC_SHOP_DOMAIN=example-shop.myshopify.com
EXPO_PUBLIC_STOREFRONT_ACCESS_TOKEN=your-storefront-access-token-here
EXPO_PUBLIC_API_VERSION=2026-01
```

### 6. Detection and Prevention

- Use tools like `git-secrets` or `truffleHog` to scan for secrets
- Enable pre-commit hooks to block commits with secrets
- Review all code changes for accidental secret exposure
- Use GitHub secret scanning (enabled by default on public repos)

### 7. What to Do If Secrets Are Exposed

If secrets are accidentally committed:

1. **ROTATE/REVOKE** the exposed credentials immediately
2. Clean git history (see below)
3. Verify the new secrets are properly stored in `.env`
4. Audit access logs for unauthorized use

## Summary

- **NO hardcoded secrets** - EVER
- **Environment variables** - with explicit validation
- **Clear error messages** - if required variables are missing
- **Document in .env.example** - never .env itself

**Consistency is paramount. These rules are the source of truth.**
