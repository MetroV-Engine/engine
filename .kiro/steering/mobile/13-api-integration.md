---
inclusion: auto
name: mobile-api
description: API integration, data fetching, API clients, backend communication
---
# API Integration

Guidelines for integrating with backend APIs from the mobile app.

## Directory Structure

```
src/api/
├── client.ts           # Base ApiClient class (shared across all APIs)
├── config.ts           # Environment-based API configuration
├── types.ts            # Shared types (ApiResult, ApiError, etc.)
├── index.ts            # Main exports for the API module
│
└── {apiName}/          # Per-API directory (e.g., giftCards/)
    ├── client.ts       # Typed client class for this API
    ├── schemas.ts      # Re-exports types from generated/types.ts
    ├── index.ts        # Public exports for this API
    └── generated/
        └── types.ts    # Auto-generated from OpenAPI spec (DO NOT EDIT)
```

## Core Principles

1. **Types are generated from OpenAPI** - Never manually define API response types
2. **Single source of truth** - The backend OpenAPI spec defines the contract
3. **Type-safe clients** - All API calls return `ApiResult<T>` with proper typing
4. **Environment-based config** - API URLs and keys come from environment variables

## Generated Types

### Generation Command

```bash
npm run generate:api:{api-name}
```

For example:
```bash
npm run generate:api:gift-cards
```

### Generated File

Types are generated into `src/api/{apiName}/generated/types.ts`:

```typescript
// ❌ NEVER edit this file manually
// This file is auto-generated from OpenAPI spec
export interface components {
  schemas: {
    GiftCard: { ... };
    // ...
  };
}
```

### Re-exporting Types

Create a `schemas.ts` file that re-exports generated types with clean names:

```typescript
// src/api/giftCards/schemas.ts
import type { components } from './generated/types';

export type GiftCard = components['schemas']['GiftCard'];
export type GiftCardResponse = components['schemas']['GiftCardResponse'];
export type GiftCardListResponse = components['schemas']['GiftCardListResponse'];
```

## API Client Pattern

### Base Client (`src/api/client.ts`)

The base `ApiClient` handles:
- HTTP methods (GET, POST, PUT, DELETE)
- API key authentication via `X-API-Key` header
- Timeout handling
- Error parsing into `ApiError` type
- Type-safe responses with `ApiResult<T>`

```typescript
// Usage in domain-specific clients
const result = await this.client.get<GiftCardResponse>(
  '/v1/gift-cards/lookup',
  { email, lastFourDigits },
);
```

### Domain Client

Each API has a typed client class:

```typescript
// src/api/giftCards/client.ts
import type { ApiClient } from '../client';
import type { ApiResult } from '../types';
import type { GiftCard, GiftCardResponse } from './schemas';

export interface CheckBalanceParams {
  email: string;
  lastFourDigits: string;
}

export class GiftCardsClient {
  private client: ApiClient;

  constructor(client: ApiClient) {
    this.client = client;
  }

  async checkBalance(params: CheckBalanceParams): Promise<ApiResult<GiftCard>> {
    const result = await this.client.get<GiftCardResponse>(
      '/v1/gift-cards/lookup',
      {
        email: params.email,
        lastFourDigits: params.lastFourDigits,
      },
    );

    if (!result.success) {
      return result;
    }

    return {
      success: true,
      data: result.data.data,
    };
  }
}
```

### Key Patterns

1. **Constructor takes base `ApiClient`** - Allows dependency injection
2. **Methods take typed params** - Define interface for each method's parameters
3. **Methods return `ApiResult<T>`** - Consistent error handling pattern
4. **Unwrap response wrappers** - Convert `{ data: GiftCard }` to just `GiftCard`

## Configuration

### Environment Variables

API configuration uses environment variables with the `EXPO_PUBLIC_` prefix:

```bash
# .env
EXPO_PUBLIC_GIFT_CARDS_API_URL=https://api.example.com
EXPO_PUBLIC_GIFT_CARDS_API_KEY=your-api-key-here
```

### Config Functions

Create a config function for each API in `src/api/config.ts`:

```typescript
import type { ApiConfig } from './types';

const getEnvVar = (name: string, required = true): string => {
  const value = process.env[name];
  if (required && !value) {
    throw new Error(`Missing required environment variable: ${name}`);
  }
  return value ?? '';
};

export const getGiftCardsApiConfig = (): ApiConfig => ({
  baseUrl: getEnvVar('EXPO_PUBLIC_GIFT_CARDS_API_URL'),
  apiKey: getEnvVar('EXPO_PUBLIC_GIFT_CARDS_API_KEY'),
  timeout: 30000,
});
```

## Using API Clients

### In Providers

Create a provider that initializes and provides the API client:

```typescript
// src/providers/GiftCards/GiftCardsProvider.tsx
import { ApiClient, getGiftCardsApiConfig } from '@/api';
import { GiftCardsClient } from '@/api/giftCards';

const apiClient = new ApiClient(getGiftCardsApiConfig());
const giftCardsClient = new GiftCardsClient(apiClient);
```

### In Components (via hooks)

Components should access API through custom hooks, never directly:

```typescript
// ✅ Good - use a hook
const { checkBalance, isLoading, error } = useGiftCards();

// ❌ Bad - don't instantiate clients in components
const client = new GiftCardsClient(...);
```

## Error Handling

### ApiResult Pattern

All API methods return `ApiResult<T>`:

```typescript
type ApiResult<T> =
  | { success: true; data: T }
  | { success: false; error: ApiError; status: number };
```

### Handling in Components

```typescript
const result = await giftCardsClient.checkBalance({ email, lastFourDigits });

if (!result.success) {
  // Handle error - result.error has { code, message }
  showError(result.error.message);
  return;
}

// Use result.data (typed as GiftCard)
setGiftCard(result.data);
```

## Adding a New API

1. **Create OpenAPI spec** in `api/{api-name}/openapi.yaml`
2. **Add generation script** to `package.json`:
   ```json
   "generate:api:{api-name}": "openapi-typescript api/{api-name}/openapi.yaml -o src/api/{apiName}/generated/types.ts"
   ```
3. **Run generation**: `npm run generate:api:{api-name}`
4. **Create `schemas.ts`** - Re-export generated types
5. **Create `client.ts`** - Typed client class
6. **Create `index.ts`** - Public exports
7. **Add config** in `src/api/config.ts`
8. **Add environment variables** to `.env` and `.env.example`

## Forbidden Patterns

- ❌ Manually defining API response types (use generation)
- ❌ Editing files in `generated/` directory
- ❌ Hardcoding API URLs or keys
- ❌ Using `fetch` directly in components (use client classes)
- ❌ Importing from `generated/types.ts` in components (use `schemas.ts`)
- ❌ Instantiating API clients in components (use providers/hooks)

## Quick Reference

| Task | Location |
|------|----------|
| Add new endpoint method | `src/api/{apiName}/client.ts` |
| Re-export a new type | `src/api/{apiName}/schemas.ts` |
| Configure API URL/key | `src/api/config.ts` + `.env` |
| Regenerate types | `npm run generate:api:{api-name}` |
| Extend base client | `src/api/client.ts` |
