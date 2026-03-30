---
inclusion: manual
---

# Architecture Rules - Usage Guide

**Rules load automatically - no manual references needed.**

## How It Works

### Auto Inclusion
Rules load when your request matches their description:
- Ask about "creating a component" → mobile-core loads
- Ask about "translations" → mobile-i18n loads
- Ask about "API endpoint" → api-architecture loads

### FileMatch Inclusion
Rules load when editing matching files:
- Open `Component.tsx` → structure, composition, styling rules load
- Open `handler.test.ts` → testing rules load
- Open `package.json` → npm scripts load

## What Loads When

See `INDEX.md` for the complete mapping of:
- Which rules auto-load for which tasks
- Which file types trigger which rules

### Selective Loading Strategy

Instead of loading all rules at once, load only what's relevant:

1. **Read INDEX.md first** - Understand which rules apply to your task
2. **Load core rules** - Always load the 2-3 core files for your domain (mobile/api)
3. **Load feature-specific rules** - Only when working on that feature
4. **Load reference rules** - Only when debugging or reviewing

### Example Workflows

**Creating a mobile component:**
```
1. Load: mobile/00-core-principles.md
2. Load: mobile/01-structure-naming.md
3. Load: mobile/02-composition-patterns.md
4. Load: mobile/03-styling.md (if styling)
5. Load: mobile/04-typescript.md (if defining types)
```

**Creating an API endpoint:**
```
1. Load: api/13-api-architecture.md
2. Load: api/handler-pattern.mdc
3. Load: api/dependency-injection.mdc
4. Load: api/testing.mdc (when writing tests)
```

**Debugging/reviewing:**
```
1. Load: mobile/09-anti-patterns.md (mobile)
2. Load: mobile/99-quick-reference.md (mobile)
```

## File Organization

```
.kiro/steering/
├── INDEX.md              # Start here - decision tree
├── README.md             # This file - usage guide
├── mobile/               # React Native/Expo rules
│   ├── 00-core-principles.md
│   ├── 01-structure-naming.md
│   ├── 02-composition-patterns.md
│   └── ...
└── api/                  # Serverless/Lambda rules
    ├── 13-api-architecture.md
    ├── handler-pattern.mdc
    └── ...
```

## Benefits of Selective Loading

- **Reduced context size** - Only load what you need
- **Faster comprehension** - Focus on relevant rules
- **Better performance** - Less token usage
- **Clearer guidance** - No information overload

## When to Load All Rules

Only load all rules when:
- Onboarding new team members
- Conducting architecture reviews
- Major refactoring across multiple domains
- Creating new architectural patterns
