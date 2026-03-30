---
inclusion: always
---

# Steering Rules Index

**Rules load automatically based on context:**
- **Auto** - Loads when description matches your request
- **FileMatch** - Loads when working with matching file types

## Mobile (React Native/Expo)

### Auto-Loaded (based on task)
- `mobile-core` - Component architecture, structure, naming, composition, styling, TypeScript
- `mobile-i18n` - Translations, internationalization, text content
- `mobile-skeleton` - Loading states, skeleton screens, data fetching
- `mobile-performance` - Performance optimization, memoization, re-renders
- `mobile-api` - API integration, data fetching, API clients
- `mobile-antipatterns` - Code review, debugging, common mistakes
- `mobile-checklist` - Quick reference checklist

### FileMatch-Loaded (when editing files)
- `**/*.tsx, **/*.ts` - Structure, composition, styling, TypeScript, imports, common components
- `package.json, .env*` - NPM scripts, security/secrets

## API (Serverless/Lambda)

### Auto-Loaded (based on task)
- `api-architecture` - API endpoints, Lambda handlers, structure
- `api-handler` - Handler implementation, request validation
- `api-di` - Dependency injection, service registration
- `api-hexagonal` - Ports/adapters, domain isolation

### FileMatch-Loaded (when editing files)
- `**/*.ts` - Clean code, comments, formatting, SST tagging
- `**/*.test.ts, **/*.spec.ts` - Testing patterns, quality, execution, assertions
