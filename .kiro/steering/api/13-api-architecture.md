---
inclusion: manual
---
# API Architecture - End-to-End Request Flow

This document specifies the serverless clean architecture for all backend APIs.

## Design Invariants

- **One-way flow down, one-way flow up**
- **Validation at the edge** (both input AND output)
- **Business logic is transport-agnostic**
- **Infrastructure is replaceable**
- **OpenAPI is the single external contract**

---

## Request Flow

### 1. UI → App Client

1. User triggers an action in the mobile UI
2. UI calls a method from the App Client
3. App Client is **generated from OpenAPI**
4. App Client:
   - Accepts only OpenAPI-defined input types
   - Serializes input into HTTP request
5. **UI does NOT know:**
   - Routes
   - URLs
   - Schemas
   - Validation rules

**Output:** A request that strictly matches OpenAPI

---

### 2. App Client → API Gateway → Lambda (SST)

6. Request reaches API Gateway
7. API Gateway invokes the Lambda defined by SST
8. Lambda cold start (if applicable):
   - Dependency Injection container is created
   - Infrastructure implementations are wired
   - UseCases are instantiated
   - Fastify instance is bootstrapped

**No business logic executed yet**

---

### 3. Lambda → Fastify Router → Handler/Adapter

9. Fastify receives the request
10. Router matches method + path
11. Control enters the Handler → Adapter

**File structure:**
```
handlers/{handlerName}/
├── handler.ts              # Entry point (minimal)
├── {HandlerName}Adapter.ts # Validation + UseCase orchestration
├── {HandlerName}Adapter.test.ts
└── index.ts
```

**Handler is the ONLY layer that knows:**
- HTTP status codes
- Fastify request/reply
- Zod schemas

---

### 4. Adapter — Input Validation

12. Adapter extracts:
    - Path parameters
    - Query parameters
    - Headers
    - Body
13. Adapter validates **ALL** inputs using Zod
14. Zod schemas mirror OpenAPI definitions
15. If validation fails:
    - 400 response sent immediately
    - No UseCase is executed

**Below this line, all data is trusted**

---

### 5. Adapter → UseCase

16. Adapter maps validated DTO → UseCase input
    - Removes transport concerns
    - Renames fields if necessary
17. Adapter calls: `useCase.execute(input)`

**From here on:**
- No HTTP
- No OpenAPI types (use domain types)
- No Zod
- No Fastify

---

### 6. UseCase — Business Logic Execution

18. UseCase enforces business rules:
    - Required fields
    - Invariants
    - Cross-entity constraints
19. Domain entities / value objects are used
20. UseCase calls ports (interfaces):
    - Repository interfaces
    - External service interfaces
21. UseCase throws domain errors (NotFoundError, ValidationError, etc.)
22. **UseCase returns domain data** (NOT response objects)

**UseCase does NOT know:**
- Database type
- ORM
- Lambda
- SST
- HTTP status codes

---

### 7. UseCase → Infrastructure (Through Ports)

23. Infrastructure implementations are invoked
24. Infrastructure:
    - Maps domain objects → ORM entities / API payloads
    - Executes SQL via TypeORM
    - Calls external APIs (Shopify, etc.)
    - Handles transactions
25. Infrastructure returns results or void

**No business logic here** — only data transformation and I/O

---

### 8. UseCase — Completion

26. UseCase receives infrastructure results
27. UseCase finalizes execution
28. **UseCase returns domain data** (e.g., `GiftCard`, not `{ data: GiftCard }`)

---

### 9. Adapter — Response Building & Validation

29. Adapter receives UseCase output (domain data)
30. Adapter wraps in response format: `{ data: result }`
31. **Adapter validates response using Zod** (`ResponseSchema.safeParse(response)`)
32. If validation fails:
    - Log the error (contract drift detected!)
    - Return 500 error
33. Send validated response with appropriate status code

**Why validate output?**
- Catches bugs early (wrong field names, missing data)
- Ensures API always matches OpenAPI contract
- Prevents breaking mobile app unexpectedly

---

### 10. Adapter — Error Handling

34. Adapter catches domain errors from UseCase
35. Maps to HTTP responses:
    - `NotFoundError` → 404
    - `ValidationError` → 400
    - `ConflictError` → 409
    - Unknown errors → re-throw (500 from global handler)

---

### 11. Fastify → Lambda → App Client

36. Fastify serializes validated response
37. Lambda returns response via API Gateway
38. App Client deserializes response
39. Typed data is returned to the UI

---

### 12. UI Update

40. UI renders based on response
41. **UI contains:**
    - No business logic
    - No validation logic
    - No backend coupling

---

## Project Structure

```
api/
├── _template/            # Template for new APIs
├── gift-cards/           # Gift Cards API
│   ├── openapi.yaml      # API contract (define FIRST)
│   ├── package.json
│   ├── sst.config.ts     # SST deployment
│   ├── src/
│   │   ├── adapters/
│   │   │   └── http/
│   │   │       ├── app.ts
│   │   │       ├── handlers/
│   │   │       │   ├── getGiftCardsByEmail/
│   │   │       │   │   ├── handler.ts
│   │   │       │   │   ├── GetGiftCardsByEmailAdapter.ts
│   │   │       │   │   └── index.ts
│   │   │       │   └── index.ts
│   │   │       └── routes/
│   │   │           └── index.ts
│   │   ├── application/
│   │   │   └── useCases/
│   │   │       ├── GetGiftCardsByEmail/
│   │   │       │   ├── GetGiftCardsByEmailUseCase.ts
│   │   │       │   └── index.ts
│   │   │       └── index.ts
│   │   ├── domain/
│   │   │   ├── models/
│   │   │   └── ports/
│   │   │       └── index.ts
│   │   ├── generated/
│   │   │   ├── api-client.ts
│   │   │   └── schemas.ts
│   │   ├── infrastructure/
│   │   │   ├── config/
│   │   │   │   └── env.ts
│   │   │   ├── di/
│   │   │   │   ├── index.ts
│   │   │   │   └── registerTestInfrastructure.ts
│   │   │   └── shopify/
│   │   │       ├── ShopifyGiftCardServiceAdmin.ts
│   │   │       └── FakeShopifyGiftCardService.ts
│   │   ├── lambda.ts
│   │   └── server.ts
│   └── migrations/       # Database migrations (if needed)
└── [future-api]/         # Follow same structure
```

---

## Adapter Pattern Example

```typescript
// GetGiftCardsByEmailAdapter.ts
import type { FastifyReply, FastifyRequest } from 'fastify';
import { inject } from '@trackit.io/di-container';

import {
  GetGiftCardsByEmailParamsSchema,  // Input validation
  GiftCardListResponseSchema,        // Output validation
  type ErrorResponse,
} from '@/generated/schemas';
import { tokenGetGiftCardsByEmailUseCase } from '@/application/useCases';
import { tokenEnv } from '@/infrastructure/config/env';

const createErrorResponse = (code: string, message: string): ErrorResponse => ({
  error: { code, message },
});

export class GetGiftCardsByEmailAdapter {
  private readonly useCase = inject(tokenGetGiftCardsByEmailUseCase);
  private readonly env = inject(tokenEnv);

  public async handle(request: FastifyRequest, reply: FastifyReply): Promise<void> {
    // 1. Auth
    if (!this.validateApiKey(request, reply)) return;

    // 2. Input validation
    const parseResult = GetGiftCardsByEmailParamsSchema.safeParse(request.query);
    if (!parseResult.success) {
      reply.status(400).send(createErrorResponse('VALIDATION_ERROR', 'Invalid parameters'));
      return;
    }

    // 3. Call UseCase (returns domain data, not response object)
    const giftCards = await this.useCase.execute(parseResult.data);

    // 4. Build response
    const response = { data: giftCards };

    // 5. Output validation
    const responseValidation = GiftCardListResponseSchema.safeParse(response);
    if (!responseValidation.success) {
      request.log.error('Response validation failed', responseValidation.error);
      reply.status(500).send(createErrorResponse('INTERNAL_ERROR', 'Response validation failed'));
      return;
    }

    // 6. Send validated response
    reply.status(200).send(responseValidation.data);
  }

  private validateApiKey(request: FastifyRequest, reply: FastifyReply): boolean {
    const apiKey = request.headers['x-api-key'];
    if (apiKey !== this.env.API_KEY) {
      reply.status(401).send(createErrorResponse('UNAUTHORIZED', 'Invalid or missing API key'));
      return false;
    }
    return true;
  }
}
```

---

## UseCase Pattern Example

```typescript
// GetGiftCardsByEmailUseCase.ts
import { createInjectionToken, inject } from '@trackit.io/di-container';

import type { GiftCard } from '@/generated/schemas';
import { tokenShopifyGiftCardService } from '@/infrastructure/shopify';

export type GetGiftCardsByEmailUseCaseArgs = {
  email: string;
};

export interface IGetGiftCardsByEmailUseCase {
  execute(args: GetGiftCardsByEmailUseCaseArgs): Promise<GiftCard[]>;  // Returns domain data!
}

export class GetGiftCardsByEmailUseCase implements IGetGiftCardsByEmailUseCase {
  private readonly shopifyService = inject(tokenShopifyGiftCardService);

  public async execute(args: GetGiftCardsByEmailUseCaseArgs): Promise<GiftCard[]> {
    const { email } = args;
    const result = await this.shopifyService.getGiftCardsByEmail({ email });
    return result.giftCards;  // Domain data, not { data: giftCards }
  }
}

export const tokenGetGiftCardsByEmailUseCase = createInjectionToken<IGetGiftCardsByEmailUseCase>(
  'GetGiftCardsByEmailUseCase',
  { useClass: GetGiftCardsByEmailUseCase },
);
```

---

## Failure Containment

| Failure Type | Responsible Layer | HTTP Status |
|--------------|-------------------|-------------|
| Input validation | Adapter | 400 |
| Auth failure | Adapter | 401 |
| Not found | UseCase → Adapter | 404 |
| Business rule | UseCase → Adapter | 400/409 |
| Response validation | Adapter | 500 |
| Persistence error | Infrastructure | 500 |
| Unknown error | Global handler | 500 |

---

## Key Rules

### ✅ DO

1. **OpenAPI first** — Define contract before implementation
2. **Validate input AND output** — Zod at the edge for both
3. **UseCases return domain data** — Not `{ data: ... }` response objects
4. **Adapters wrap responses** — Build `{ data: result }` in adapter
5. **Ports for dependencies** — UseCases depend on interfaces
6. **Fakes for testing** — Every infrastructure has a Fake implementation

### ❌ DON'T

1. **No validation in UseCases** — Data is trusted after adapter validation
2. **No HTTP in UseCases** — No status codes, no request/response objects
3. **No business logic in infrastructure** — Only data mapping and I/O
4. **No response objects from UseCases** — Adapter builds response format
5. **No skipping output validation** — Always validate before sending
