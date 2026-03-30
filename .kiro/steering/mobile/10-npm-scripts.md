---
inclusion: fileMatch
fileMatchPattern: ["package.json", ".env*", "**/*.ts", "**/*.tsx"]
---
# NPM Scripts Reference

This section lists all available npm scripts for the Storefront App Template project. Use these commands for development, building, and maintaining the application.

## Development

### `npm start`

**Description**: Start the Expo development server.

**When to use**:

- Starting development on Expo Go app
- Quick prototyping without native code
- Testing on multiple platforms simultaneously

### `npm run start:dev`

**Description**: Start the Expo development server with dev client.

**When to use**:

- When working with custom native modules
- After running `npm run prebuild`
- For production-like development environment

**Equivalent**: `npx expo start --dev-client`

### `npm run start:dev:reset`

**Description**: Start the Expo development server with dev client and clear cache.

**When to use**:

- When experiencing bundling issues
- After changing dependencies
- After modifying native configurations
- To clear Metro bundler cache

**Equivalent**: `npx expo start --dev-client --clear`

## Platform-Specific Commands

### `npm run android`

**Description**: Build and run the Android app on connected device or emulator.

**Prerequisites**:

- Android Studio installed
- Android SDK configured
- Device connected or emulator running
- Run `npm run prebuild` first if you haven't

**Equivalent**: `expo run:android`

### `npm run ios`

**Description**: Build and run the iOS app on simulator or connected device.

**Prerequisites**:

- macOS required
- Xcode installed
- iOS simulator or device connected
- Run `npm run prebuild` first if you haven't

**Equivalent**: `expo run:ios`

### `npm run web`

**Description**: Start the web development server.

**When to use**:

- Testing web compatibility
- Debugging web-specific issues
- Demonstrating the app in a browser

**Equivalent**: `expo start --web`

## Android Build & Install

### `npm run android:build:dev`

**Description**: Build debug APK for Android.

**Output**: `android/app/build/outputs/apk/debug/app-debug.apk`

**When to use**:

- Creating a debug build for testing
- Sharing APK with testers
- Installing on physical devices without Android Studio

**Equivalent**: `cd android && ./gradlew assembleDebug`

### `npm run android:install:dev`

**Description**: Install the debug APK on connected Android device.

**Prerequisites**:

- Debug APK must be built first (`npm run android:build:dev`)
- Android device connected via USB with debugging enabled
- ADB installed and accessible

**Equivalent**: `adb install -r android/app/build/outputs/apk/debug/app-debug.apk`

### `npm run android:build:dev:reset`

**Description**: Clean build directory and rebuild debug APK.

**When to use**:

- After making native code changes
- When build outputs are corrupted
- To ensure a completely fresh build

**Equivalent**: `cd android && ./gradlew clean && ./gradlew assembleDebug`

### `npm run android:install:dev:reset`

**Description**: Uninstall existing app and install fresh debug APK.

**When to use**:

- Need to clear app data completely
- After changing app permissions
- To test first-time app installation flow

**Equivalent**: `adb uninstall com.anonymous.storefrontapptemplate || true && adb install -r android/app/build/outputs/apk/debug/app-debug.apk`

## Code Quality

### `npm run lint:check`

**Description**: Check TypeScript and TSX files for linting errors.

**When to use**:

- Before committing code
- In CI/CD pipelines
- To review code quality issues

**Equivalent**: `eslint . --ext .ts,.tsx`

### `npm run lint:fix`

**Description**: Automatically fix linting errors where possible.

**When to use**:

- To auto-fix fixable linting issues
- Before committing to clean up code
- After refactoring

**Equivalent**: `eslint . --ext .ts,.tsx --fix`

### `npm run format:check`

**Description**: Check if files are formatted according to Prettier rules.

**When to use**:

- In CI/CD to enforce formatting
- Before committing code
- To identify formatting issues

**Equivalent**: `prettier --check .`

### `npm run format:fix`

**Description**: Format all files with Prettier.

**When to use**:

- Before committing code
- After refactoring
- To ensure consistent formatting

**Equivalent**: `prettier --write .`

### `npm run type-check`

**Description**: Run TypeScript compiler to check for type errors.

**When to use**:

- Before committing code
- In CI/CD pipelines
- To verify type safety

**Equivalent**: `tsc --noEmit`

## Project Management

### `npm run prebuild`

**Description**: Generate native Android and iOS project files.

**When to use**:

- First time setting up the project
- After adding native dependencies
- After changing app.json or native configurations
- Before running `npm run android` or `npm run ios`

**Equivalent**: `npx expo prebuild`

### `npm run reset-project`

**Description**: Reset the project to a clean state using custom script.

**When to use**:

- To start fresh with project structure
- Reverting to initial project state
- Use with caution (may delete custom changes)

**Equivalent**: `node ./scripts/reset-project.js`

## Common Workflows

### Development Workflow

```bash
# 1. Start development server
npm run start:dev

# 2. In another terminal, run on Android
npm run android

# Or on iOS (macOS only)
npm run ios
```

### Code Quality Check Workflow

```bash
# Check and fix all quality issues
npm run lint:fix
npm run format:fix
npm run type-check
```

### Android Release Workflow

```bash
# 1. Clean and build
npm run android:build:dev:reset

# 2. Install on device
npm run android:install:dev:reset
```

### Fresh Start Workflow

```bash
# 1. Clean install dependencies
rm -rf node_modules package-lock.json
npm install

# 2. Clear caches and restart
npm run start:dev:reset
```

## Troubleshooting

### Metro Bundler Issues

**Problem**: Bundler not updating code changes  
**Solution**: `npm run start:dev:reset`

### Android Build Failures

**Problem**: Gradle build errors  
**Solution**: `npm run android:build:dev:reset`

### Type Errors in IDE but App Runs

**Problem**: TypeScript errors showing  
**Solution**: `npm run type-check`

### Formatting Conflicts

**Problem**: Code not formatted consistently  
**Solution**: `npm run format:fix`
