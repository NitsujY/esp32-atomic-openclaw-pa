# Architecture Specification

## Scope

This document defines the target architecture for the AtomS3 client and the companion bridge service. It is aligned to the current repository layout and is intended to guide implementation rather than replace it.

## Repository Structure

### Firmware

- `src/main.c`: firmware entrypoint and top-level orchestration.
- `components/app_state`: state machine and event vocabulary.
- `components/input`: button ISR, debounce, press duration handling.
- `components/ui`: LCD-facing rendering layer. Currently log-only fallback.
- `components/audio_capture`: microphone I2S capture and bounded buffering.
- `components/audio_transport`: utterance assembly and upload payload preparation.
- `components/audio_playback`: I2S speaker playback.
- `components/bridge_client`: device-to-bridge networking and session workflow.
- `components/config`: Kconfig-backed device configuration and compile-time sizing.

### Bridge

- `bridge/src/server.ts`: HTTP server bootstrap.
- `bridge/src/routes/device.ts`: device API endpoints.
- `bridge/src/services/sessionService.ts`: session lifecycle orchestration.
- `bridge/src/services/sttService.ts`: STT provider adapter.
- `bridge/src/services/telegramService.ts`: Telegram adapter.
- `bridge/src/services/ttsService.ts`: TTS provider adapter.
- `bridge/public/simulator.html`: browser-based local UI and avatar simulator.

## Architecture Principles

1. ESP32 remains a thin edge device.
2. The bridge is the only component allowed to know third-party provider details.
3. Firmware modules communicate through explicit events and typed interfaces.
4. Stub and hardware-fallback behavior is allowed during bring-up, but must remain isolated behind module boundaries.
5. Feature completeness is subordinate to predictable state transitions and bounded memory use.

## Target Runtime Data Flow

1. User presses center button.
2. `input` publishes `APP_EVENT_PTT_PRESSED`.
3. `app_state` transitions to `listening`.
4. `audio_capture` begins filling a bounded buffer.
5. User releases center button.
6. `input` publishes `APP_EVENT_PTT_RELEASED` with duration.
7. `audio_transport` finalizes utterance buffer.
8. `bridge_client` starts or resumes a bridge session and uploads audio.
9. Bridge performs STT, Telegram delivery, reply collection, and TTS.
10. Firmware polls bridge session until response audio is ready.
11. `audio_playback` plays response audio.
12. `app_state` returns to `idle`.

## Firmware State Model

The canonical event set is defined in `components/app_state`.

### States

- `idle`: ready for input.
- `listening`: active capture in progress.
- `uploading`: utterance is being sent to bridge.
- `waiting_response`: upload complete, awaiting response readiness.
- `playing`: playback active.
- `error`: recoverable fault state.

### Events

- `boot`
- `wifi_connected`
- `wifi_disconnected`
- `mode_toggle`
- `ptt_pressed`
- `ptt_released`
- `upload_started`
- `upload_done`
- `response_ready`
- `playback_started`
- `playback_done`
- `error`
- `clear_error`

### State Ownership

- `app_state` owns state transitions and mode.
- `input`, `bridge_client`, and `audio_playback` may publish events.
- `ui` is a pure renderer from state snapshot plus connectivity/error metadata.

## Module Contracts

### `input`

- Responsibilities:
  - Configure GPIO buttons.
  - Debounce transitions.
  - Convert raw presses into semantic app events.
- Must not:
  - Perform business logic.
  - Call bridge or audio code directly.

### `ui`

- Responsibilities:
  - Initialize LCD subsystem.
  - Render current mode and state.
  - Provide simple status and error indication.
- Must not:
  - Maintain independent app state.

### `audio_capture`

- Responsibilities:
  - Initialize microphone I2S.
  - Capture PCM in a bounded ringbuffer.
  - Expose drain function for utterance assembly.
- Must not:
  - Decide upload timing.

### `audio_transport`

- Responsibilities:
  - Reset utterance state before capture.
  - Finalize captured data into bridge upload format.
  - Optionally wrap PCM in WAV.
- Must not:
  - Handle network operations.

### `bridge_client`

- Responsibilities:
  - Manage HTTP communication with the bridge.
  - Start session, upload utterance, poll session, fetch response audio, mark completion.
  - Publish app-level events reflecting upload and response progress.
- Must not:
  - Implement STT, TTS, or Telegram provider logic locally.

### `audio_playback`

- Responsibilities:
  - Initialize speaker I2S.
  - Play response audio payloads.
  - Publish playback completion event.

## Network Topology

- Device connects to WiFi as a station.
- Device talks only to bridge over HTTP.
- Bridge talks to external services.
- Bridge may run on local workstation, homelab node, or reachable service endpoint.

## Local Simulation

- Bridge may expose a browser-based simulator for UI iteration and acceptance testing.
- The simulator is a development aid only; it is not part of the firmware runtime path.
- The simulator should render the same state names, mode semantics, and avatar vocabulary used by the device UI.

## Configuration Model

### Device-Side Config

- WiFi SSID and password
- Bridge base URL
- Bridge API key
- Device ID
- Default mode
- Audio sample rate
- Timing thresholds
- GPIO assignments

These are stored through ESP-IDF Kconfig and `sdkconfig.defaults`.

### Bridge-Side Config

- Port
- Bridge API key
- Telegram bot token
- Work and Kid chat IDs
- Work and Kid prefixes
- OpenAI API key and model names
- Mock-mode toggles for Telegram and OpenAI integrations

These are stored as environment variables in the bridge runtime.

## Failure Model

### Recoverable Failures

- WiFi disconnect
- Bridge timeout
- Session not found
- Response not ready yet
- Upstream STT/TTS/Telegram provider failure

Expected behavior:

- Publish error or recovery events.
- Keep device in a defined state.
- Avoid reboot unless the underlying ESP-IDF subsystem becomes unrecoverable.

### Bring-Up Fallbacks

- Unknown LCD implementation may temporarily render to logs.
- Missing mic or speaker pins may keep modules in stub mode with clear warnings.
- Bridge stub mode may synthesize local response audio until real HTTP workflow is complete.

## Target Milestones

### M1 Hardware Bring-Up

- Buttons, LCD, mic, and speaker initialize on real hardware.

### M2 Real Bridge Session Workflow

- Replace firmware bridge stub with session-based HTTP client.

### M3 End-to-End External Integration

- Bridge connects to STT, Telegram, and TTS in non-mock mode.

### M4 Hardening

- Timeouts, retries, and memory validation are in place.