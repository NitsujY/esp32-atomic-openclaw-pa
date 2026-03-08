# Product Specification

## Title

OpenClaw AtomS3 Voice Client

## Purpose

Build a voice-based personal assistant client on M5Stack AtomS3 that acts only as an edge I/O device. The device captures microphone input, sends it to an external assistant pipeline through a bridge service, receives a text response, plays synthesized speech, and reflects state on the LCD.

The device does not host assistant logic or LLM reasoning locally. It is a network-connected microphone, speaker, button, and display terminal for OpenClaw.

## Goals

1. Provide push-to-talk voice access to OpenClaw over WiFi.
2. Support two user-selectable operating modes: Work and Kid.
3. Keep device-side logic small, deterministic, and recoverable.
4. Make the bridge service the integration boundary for STT, Telegram routing, and TTS.
5. Operate within ESP32-S3 memory limits without large transient allocations or unbounded buffers.

## Non-Goals

1. Running LLM inference on the device.
2. Hosting complex business logic on the ESP32.
3. Supporting arbitrary chat channels beyond the defined mode-based Telegram routing.
4. Building a full local UI framework before the device state machine is stable.

## Hardware Targets

- Device: M5Stack AtomS3 (ESP32-S3)
- Inputs:
  - Center screen button for push-to-talk
  - Side button for mode switching
  - Built-in I2S microphone
- Outputs:
  - Built-in I2S speaker
  - 1.28 inch LCD
- Network:
  - WiFi station mode

## Operating Modes

### Work Mode

- Default boot mode.
- Full assistant access.
- Bridge routes transcribed text to `WORK_CHAT_ID` with prefix `[Voice/Work]`.

### Kid Mode

- Sandboxed mode for a five-year-old child.
- Bridge routes transcribed text to `KID_CHAT_ID` with prefix `[Voice/Kid]`.
- The bridge or upstream assistant may apply safety or policy controls, but those rules do not live on the ESP32.

## User Experience Requirements

### Idle

- LCD shows current mode and WiFi connectivity.
- Device is ready for button input.

### Side Button

- A valid press toggles between Work and Kid mode.
- LCD updates immediately after the toggle.
- Mode toggle is ignored only when hardware or system safety requires it; normal playback is not itself sufficient reason to drop the toggle.

### Center Button Press and Hold

- Press starts audio capture.
- LCD displays listening state.
- Device buffers microphone PCM in bounded chunks.

### Center Button Release

- Release stops capture.
- If captured audio duration is below the minimum threshold, the device discards the utterance and returns to idle.
- Otherwise the device submits the utterance to the bridge.

### Waiting for Response

- LCD displays a thinking state.
- Device waits for bridge response readiness by polling or an equivalent bridge-managed asynchronous workflow.

### Playback

- Device obtains response audio from the bridge.
- Device plays audio through the I2S speaker.
- On playback completion the device returns to idle.

## Functional Requirements

### FR-1 Device Boot

- Device initializes NVS, WiFi, buttons, UI, audio capture, audio playback, audio transport, and bridge client.
- If recoverable initialization failures occur, the device enters an error state with an observable indicator.

### FR-2 State Machine

- Device shall implement these states:
  - `idle`
  - `listening`
  - `uploading`
  - `waiting_response`
  - `playing`
  - `error`
- State transitions shall be event-driven and deterministic.

### FR-3 Input Handling

- Side and center buttons shall be debounced in firmware.
- Center button duration shall be measured precisely enough to enforce minimum utterance length.
- Input ISR shall be lightweight and defer work to a task queue.

### FR-4 Audio Capture

- Microphone input shall be captured as mono 16-bit PCM over I2S.
- Target nominal sample rate is 16 kHz unless explicitly changed in config.
- Audio buffering shall use bounded memory and tolerate dropped samples under saturation rather than corrupting state.

### FR-5 Audio Upload

- Firmware shall submit recorded audio to the bridge, not directly to OpenAI or Telegram.
- Firmware may send raw PCM or WAV-wrapped PCM, but the content type and contract must match `bridge-api-spec.md`.

### FR-6 Bridge Processing

- Bridge shall manage STT, Telegram routing, reply collection, and TTS.
- Bridge shall expose a session-oriented API for device upload, status, and audio retrieval.
- Bridge shall authenticate requests when a bridge API key is configured.

### FR-7 Audio Playback

- Firmware shall play bridge-supplied response audio via I2S speaker.
- Firmware shall support WAV payloads at minimum.

### FR-8 UI Feedback

- LCD shall show mode and runtime state.
- Minimum required state labels are `Work`, `Kid`, `Listening`, `Thinking`, `Playing`, and an error indicator.
- LCD shall render a mode-specific centered avatar.
- Work mode avatar shall be a robot face.
- Kid mode avatar shall be a unicorn face.
- Avatar art shall use a pixel-friendly visual language suitable for a very small screen.
- Avatar animation shall support at minimum idle, listening, thinking, playing, sleeping, smiling, and love reactions.
- Icons and animation are desirable but secondary to state correctness.

### FR-8A Local UI Simulation

- The project shall provide a local simulator for UI flow validation without flashing firmware.
- The simulator shall allow mode changes, state changes, prompt and reply text preview, and avatar animation preview.
- The simulator shall be lightweight and runnable alongside the bridge during local development.

### FR-9 Network Recovery

- WiFi disconnects shall be detected.
- Device shall attempt reconnection automatically.
- If disconnect happens during active interaction, device shall surface a recoverable error and return to a usable state when connectivity recovers.

### FR-10 Security

- Secrets shall not be hardcoded in source.
- ESP32 stores only the minimum required credentials for WiFi and bridge authentication.
- Telegram and OpenAI credentials shall remain in the bridge environment, not on the device.

## Quality Attributes

### Reliability

- Device should survive intermittent WiFi failure without reboot.
- Device should not require power cycling for normal recovery.

### Performance

- UI should reflect button press within 100 ms.
- Audio capture should begin within 150 ms of push-to-talk press.
- Post-release transition to uploading should occur within 250 ms.

### Resource Constraints

- All buffers must be bounded at compile time or explicit queue/ringbuffer capacity.
- No unbounded heap growth during repeated push-to-talk cycles.
- Large response audio should be streamed or chunked where practical; avoid copying payloads more than necessary.

## External Dependencies

- WiFi access point
- Bridge service reachable over HTTP on local network or reachable URL
- Bridge-side integrations:
  - Telegram Bot API
  - STT provider such as OpenAI Whisper
  - TTS provider such as OpenAI TTS or ElevenLabs

## Open Questions

1. Confirm exact AtomS3 LCD driver and graphics stack.
2. Confirm microphone and speaker pin mapping for the target AtomS3 variant.
3. Decide whether device-to-bridge audio format is WAV by default or raw PCM by default.
4. Decide acceptable response polling interval versus power and latency tradeoff.
5. Decide whether mode toggle is allowed during playback or deferred until idle.
6. Decide final LCD color palette and frame rate budget for avatar animation.

## Acceptance Criteria

### AC-1 Mode Toggle

- Given the device is powered and idle, when the side button is pressed once, the displayed mode changes from Work to Kid or Kid to Work and remains active for the next utterance.

### AC-2 Short Press Rejection

- Given the center button is pressed and released below the minimum PTT duration, no bridge upload occurs and the device returns to idle.

### AC-3 Work Routing

- Given the device is in Work mode and a valid utterance is submitted, the bridge routes the transcribed text to the configured work chat with the work prefix.

### AC-4 Kid Routing

- Given the device is in Kid mode and a valid utterance is submitted, the bridge routes the transcribed text to the configured kid chat with the kid prefix.

### AC-5 End-to-End Playback

- Given a valid utterance and available bridge dependencies, the device captures audio, uploads it, receives a response audio payload, plays it, and returns to idle.

### AC-6 Network Recovery

- Given WiFi disconnect during idle or interaction, the device reports the issue, retries connection, and returns to usable idle state after reconnection.