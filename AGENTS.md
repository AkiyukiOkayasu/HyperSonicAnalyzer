# AGENTS.md - Development Guidelines for AI Assistants

## Language Settings

- **Respond in Japanese**
- **Write code comments in Japanese**

## Project Overview

HyperSonic Analyzer is a VST3/Standalone spectrum analyzer plugin built with JUCE 8, designed for visualizing ultra-high sample rate audio (up to 768kHz).

## Build Commands

```bash
# Configure and build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Run format check and build with warnings
./scripts/lint.sh
```

## Code Style

- Use `.clang-format` (Allman style, 4-space indent, 100 char limit)
- Run `clang-format -i Source/*.cpp Source/*.h` before committing
- Avoid compiler warnings (treat warnings as errors in practice)

## Architecture

### Audio Thread Safety
- **CRITICAL**: Never allocate memory or do heavy computation on the audio thread
- FFT processing is intentionally done on the UI thread via timer callback
- Audio thread only copies samples to a lock-free FIFO buffer
- This design prevents audio glitches even with 65536-sample FFT

### Key Classes
- `PluginProcessor`: Audio passthrough + FIFO buffer for UI thread
- `PluginEditor`: Main UI with amplitude range controls
- `SpectralAnalyzer`: Spectrum visualization component

### FFT Configuration
- Fixed 65536 sample FFT size (fftOrder = 16)
- Blackman-Harris window function
- Log frequency scale (10Hz to Nyquist)
- Amplitude range: -192dB to +20dB (adjustable)

## Testing

No automated tests currently. Manual testing:
1. Build the VST3 plugin
2. Load in a DAW with high sample rate support
3. Verify spectrum display from DC to Nyquist
4. Check audio passthrough is clean (no artifacts)
