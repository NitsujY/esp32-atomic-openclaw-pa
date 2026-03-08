# Implementation Plan

## Objective

Move the repository from scaffold status to a spec-driven, testable AtomS3 voice client with a bridge-backed end-to-end flow.

## Phase 0: Spec Baseline

Status:

- Completed by adding this spec set.

Exit criteria:

- Product requirements, architecture, API contract, and delivery phases are documented.
- All future behavior changes reference these specs first.

## Phase 1: Hardware Bring-Up

Scope:

- Confirm actual AtomS3 pinout for LCD, microphone, and speaker.
- Replace log-only UI with LCD-backed renderer.
- Verify button behavior on physical hardware.

Tasks:

1. Confirm exact GPIO assignments and bus parameters.
2. Implement LCD init and primitive state rendering in `components/ui`.
3. Validate center button and side button semantics on-device.
4. Validate microphone capture on real input.
5. Validate speaker playback with known WAV sample.
6. Validate avatar readability on the AtomS3 display.
7. Keep the browser simulator visually aligned with the device UI states.

Exit criteria:

- Device shows mode and runtime state on LCD.
- Real button interactions generate expected state transitions.
- Mic and speaker work without stub-only fallback.

## Phase 2: Firmware Bridge Client

Scope:

- Replace stub transport in `components/bridge_client` with the session-oriented HTTP workflow in `bridge-api-spec.md`.

Tasks:

1. Add HTTP client wrapper for bridge base URL and API key.
2. Implement `start session` request.
3. Implement audio upload request with correct content type.
4. Implement session polling using configured interval and timeout.
5. Implement response audio download.
6. Implement session completion call.
7. Map failures to app events and error state.

Exit criteria:

- Firmware completes one utterance-response cycle against the real bridge without using local synthesized stub audio.

## Phase 3: Bridge Integrations

Scope:

- Make the bridge production-capable for STT, Telegram, and TTS.

Tasks:

1. Finalize STT service adapter contract and provider implementation.
2. Finalize Telegram routing to work and kid chats with prefixes.
3. Finalize reply collection strategy.
4. Finalize TTS output format for firmware playback.
5. Add bridge-level timeouts, validation, and structured logging.

Exit criteria:

- Bridge handles non-mock end-to-end processing for both modes.
- Work and Kid mode route to different Telegram destinations correctly.

## Phase 4: Hardening

Scope:

- Improve reliability, diagnostics, and memory behavior.

Tasks:

1. Add retry and timeout strategy for bridge calls.
2. Verify no heap growth across repeated push-to-talk cycles.
3. Validate ringbuffer and playback queue sizing under long utterances.
4. Improve UI error messages and reconnection feedback.
5. Add structured logs for state transitions and bridge timings.

Exit criteria:

- Repeated interactions do not degrade device stability.
- WiFi loss and provider failures recover cleanly.

## Engineering Rules

1. Keep ESP32 logic limited to input, output, transport, and state management.
2. Do not add provider-specific credentials or APIs to firmware.
3. Prefer bounded buffers and fixed-capacity queues.
4. If a change alters an interface or workflow, update the matching spec first.
5. Do not merge bridge-client behavior that diverges from `bridge-api-spec.md` without revising the spec.

## Immediate Next Build Items

1. Confirm hardware pinout and LCD driver for AtomS3.
2. Implement LCD-backed `ui` module.
3. Implement real HTTP `bridge_client` session workflow.
4. Validate bridge mock and non-mock modes against the device contract.
5. Convert the browser simulator avatar states into firmware sprite assets.