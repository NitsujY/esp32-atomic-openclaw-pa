# Bridge API Specification

## Purpose

This document defines the HTTP contract between the AtomS3 firmware and the bridge service. The device talks only to the bridge. The bridge owns all third-party provider integrations.

## Runtime Defaults

- Default bridge port: `3001`
- Default local development URL: `http://127.0.0.1:3001`

## Versioning

- Initial contract version: `v1`
- Base path: `/device`

This repository currently exposes unversioned routes. The spec keeps the resource shapes stable even if explicit versioning is added later.

## Authentication

- If `BRIDGE_API_KEY` is unset, authentication is optional for development.
- If `BRIDGE_API_KEY` is set, the device must send either:
  - `Authorization: Bearer <key>`
  - `x-api-key: <key>`

## Content Types

- JSON requests and responses use `application/json`.
- Audio upload supports:
  - `audio/wav`
  - `application/octet-stream`
- Audio download returns bridge-selected audio MIME type, initially expected to be WAV.

## Data Types

### DeviceMode

Possible values:

- `work`
- `kid`

### SessionStatus

Possible values:

- `idle`
- `uploading`
- `waiting_response`
- `ready`
- `completed`
- `error`

### SessionView

```json
{
  "sessionId": "uuid",
  "deviceId": "atoms3-dev",
  "mode": "work",
  "status": "waiting_response",
  "createdAt": "2026-03-08T12:00:00.000Z",
  "updatedAt": "2026-03-08T12:00:02.500Z",
  "transcript": "optional text",
  "replyText": "optional text",
  "responseBytes": 12345,
  "error": "optional error message"
}
```

## Endpoints

### `GET /health`

Purpose:

- Service liveness and mock-mode visibility.

Response `200` example:

```json
{
  "status": "ok",
  "bridgeStubModes": {
    "telegram": true,
    "openAi": true
  }
}
```

### `POST /device/session/start`

Purpose:

- Create a new bridge session for one utterance/response cycle.

Request body:

```json
{
  "deviceId": "atoms3-dev",
  "mode": "work"
}
```

Validation rules:

- `deviceId` is required.
- `mode` must be `work` or `kid`.

Response `201` example:

```json
{
  "sessionId": "uuid",
  "deviceId": "atoms3-dev",
  "mode": "work",
  "status": "idle",
  "createdAt": "2026-03-08T12:00:00.000Z",
  "updatedAt": "2026-03-08T12:00:00.000Z"
}
```

Response `400` example:

```json
{
  "error": "deviceId and mode are required"
}
```

### `POST /device/session/:sessionId/utterance`

Purpose:

- Upload the captured utterance for the session.

Headers:

- `Content-Type: audio/wav` for WAV-wrapped PCM
- `Content-Type: application/octet-stream` for raw PCM

Request body:

- Binary audio payload

Behavior:

- Bridge marks session as `uploading`, then asynchronously processes STT, Telegram routing, and TTS.
- Current implementation returns before asynchronous processing finishes.

Response `202` example:

```json
{
  "sessionId": "uuid",
  "deviceId": "atoms3-dev",
  "mode": "work",
  "status": "uploading",
  "createdAt": "2026-03-08T12:00:00.000Z",
  "updatedAt": "2026-03-08T12:00:01.000Z"
}
```

Response `404` example:

```json
{
  "error": "Unknown session uuid"
}
```

### `GET /device/session/:sessionId`

Purpose:

- Poll session state until response audio is ready or an error occurs.

Response `200` example:

```json
{
  "sessionId": "uuid",
  "deviceId": "atoms3-dev",
  "mode": "kid",
  "status": "ready",
  "createdAt": "2026-03-08T12:00:00.000Z",
  "updatedAt": "2026-03-08T12:00:05.000Z",
  "transcript": "tell me a story",
  "replyText": "Once upon a time...",
  "responseBytes": 18234
}
```

Response `404` example:

```json
{
  "error": "Session not found"
}
```

Firmware polling rules:

- Poll every `CONFIG_OPENCLAW_RESPONSE_POLL_MS`.
- Stop polling on `ready`, `error`, or timeout.

### `GET /device/session/:sessionId/audio`

Purpose:

- Fetch synthesized response audio once the session is ready.

Response `200`:

- Binary audio body.
- `Content-Type` set by bridge, initially expected `audio/wav`.

Response `409` example:

```json
{
  "error": "Audio is not ready"
}
```

Response `404` example:

```json
{
  "error": "Unknown session uuid"
}
```

### `POST /device/session/:sessionId/complete`

Purpose:

- Mark session complete after playback or terminal failure.

Response `200` example:

```json
{
  "sessionId": "uuid",
  "deviceId": "atoms3-dev",
  "mode": "work",
  "status": "completed",
  "createdAt": "2026-03-08T12:00:00.000Z",
  "updatedAt": "2026-03-08T12:00:08.000Z"
}
```

## Required Firmware Workflow

1. Call `POST /device/session/start`.
2. Call `POST /device/session/:sessionId/utterance` with audio body.
3. Poll `GET /device/session/:sessionId` until status is `ready` or `error`.
4. If `ready`, call `GET /device/session/:sessionId/audio`.
5. Play returned audio.
6. Call `POST /device/session/:sessionId/complete`.

## Error Handling Rules

- `401`: authentication failure; surface recoverable error to UI.
- `404`: session or route not found; fail current interaction.
- `409`: audio not ready; continue polling unless overall timeout expired.
- `5xx`: bridge or provider failure; fail current interaction and return to idle after error presentation.

## Implementation Gap Versus Current Firmware

Current firmware `bridge_client` is still stub-backed and does not yet implement:

1. Session creation
2. Real audio upload over HTTP
3. Session polling
4. Response audio download
5. Session completion call

These are the primary requirements for the next implementation phase.