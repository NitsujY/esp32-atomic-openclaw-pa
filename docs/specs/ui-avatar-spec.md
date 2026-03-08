# UI And Avatar Specification

## Purpose

Define how the AtomS3 screen should communicate mode, state, and personality through a tiny centered avatar that remains readable on a very small LCD.

## Core Direction

- The screen should feel like a living assistant, not a static status panel.
- The avatar sits centered on screen and is the primary visual focus.
- Text remains secondary and short.
- Motion should be expressive but lightweight enough for an ESP32-class device.

## Mode Identity

### Work Mode

- Avatar: robot face
- Tone: calm, precise, confident
- Shape language: square head, antenna, geometric eyes, restrained mouth
- Default palette: slate, cyan, warm white, muted teal

### Kid Mode

- Avatar: unicorn face
- Tone: playful, friendly, magical
- Shape language: rounded face, horn, blush, soft eyes, pastel accents
- Default palette: cream, pink, aqua, sky blue, sunshine yellow

## Required States

- `idle`: breathing or subtle blink loop
- `listening`: alert eyes, pulse ring, mic-reactive bounce if feasible
- `thinking`: eyes look around, small orbiting dots or sparkle motion
- `playing`: mouth or body pulse synced loosely to speech playback
- `error`: dim or red-accent warning expression

## Expressive Emotes

- `smile`
- `sleep`
- `love`
- `blink`
- `look_left`
- `look_right`
- `wink`
- `surprised`
- `curious`
- `celebrate`

## Prompt-Responsive Reactions

The UI layer may map prompt or reply keywords to transient emotes during assistant interaction.

Suggested mappings:

- Contains `love`, `heart`, `hug`: `love`
- Contains `sleep`, `bed`, `night`: `sleep`
- Contains `great`, `yay`, `happy`, `nice`: `smile`
- Contains `look`, `see`, `watch`: `curious` or `look_left` or `look_right`
- Contains `wow`, `amazing`, `surprise`: `surprised`

This mapping is optional for firmware at first and should start in the browser simulator.

## Idle Rotation Set

To make the avatar feel alive, idle mode may randomly rotate through a small set of safe expressions.

Recommended idle set:

1. neutral blink
2. look left
3. look right
4. soft smile
5. sleepy blink

Recommended rotation rules:

- Hold each idle scene for 2 to 5 seconds.
- Avoid high-frequency motion.
- Randomly repeat a neutral scene between expressive ones so the animation does not feel noisy.

## Simulator Requirements

- Browser simulator must support both avatars.
- Simulator must let the developer force any runtime state.
- Simulator must let the developer force any emote.
- Simulator should support automatic idle rotation.
- Simulator should let the developer enter prompt and reply text to preview reaction rules.

## Firmware Translation Guidance

- Start with 1-bit or low-color sprite frames if necessary.
- Prefer reusable eye and mouth overlays instead of full-frame animation for every state.
- Use a centered composition with large eyes and minimal detail.
- Favor 6 to 12 high-value animation frames over many low-value frames.