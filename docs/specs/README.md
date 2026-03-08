# OpenClaw AtomS3 Spec Set

This directory is the source of truth for spec-driven development of the AtomS3 voice client.

## Documents

- `product-spec.md`: user-facing goals, requirements, constraints, and acceptance criteria.
- `architecture-spec.md`: target firmware and bridge architecture, module boundaries, and data flow.
- `bridge-api-spec.md`: HTTP contract between device firmware and the local bridge service.
- `ui-avatar-spec.md`: LCD avatar design, animation vocabulary, and local simulator behavior.
- `implementation-plan.md`: phased delivery plan with exit criteria.

## Working Agreement

1. Start changes by updating the relevant spec when behavior or interfaces change.
2. Implement only against approved requirements in these docs.
3. Validate each phase against the acceptance criteria in `product-spec.md` and `implementation-plan.md`.
4. Treat code comments and ad hoc behavior as subordinate to these specs.

## Current Baseline

- Firmware scaffold exists in `src/` and `components/` using ESP-IDF.
- Bridge service exists in `bridge/` using Node.js and Express.
- Current firmware bridge path is stub-first; real HTTP client behavior is not implemented yet.
- LCD and exact hardware pinout are still partially unresolved and must be confirmed before full hardware bring-up.