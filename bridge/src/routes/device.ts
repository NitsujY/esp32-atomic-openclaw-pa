import express from "express";

import { DeviceMode, bridgeConfig } from "../config.js";
import { SessionService } from "../services/sessionService.js";

const isDeviceMode = (value: unknown): value is DeviceMode => value === "work" || value === "kid";

const extractApiKey = (request: express.Request): string => {
  const bearer = request.header("authorization");
  if (bearer?.startsWith("Bearer ")) {
    return bearer.slice("Bearer ".length);
  }

  return request.header("x-api-key") ?? "";
};

export const createDeviceRouter = (sessionService: SessionService): express.Router => {
  const router = express.Router();

  router.use((request, response, next) => {
    if (!bridgeConfig.apiKey) {
      next();
      return;
    }

    if (extractApiKey(request) !== bridgeConfig.apiKey) {
      response.status(401).json({ error: "Unauthorized" });
      return;
    }

    next();
  });

  router.post("/session/start", express.json(), (request, response) => {
    const { deviceId, mode } = request.body as { deviceId?: string; mode?: DeviceMode };
    if (!deviceId || !isDeviceMode(mode)) {
      response.status(400).json({ error: "deviceId and mode are required" });
      return;
    }

    response.status(201).json(sessionService.startSession(deviceId, mode));
  });

  router.post(
    "/session/:sessionId/utterance",
    express.raw({ type: ["audio/wav", "application/octet-stream"], limit: "2mb" }),
    async (request, response) => {
      try {
        const audio = Buffer.isBuffer(request.body) ? request.body : Buffer.from([]);
        const contentType = request.header("content-type") ?? "application/octet-stream";
        const session = await sessionService.submitUtterance(
          request.params.sessionId,
          audio,
          contentType,
        );
        response.status(202).json(session);
      } catch (error) {
        response.status(404).json({ error: error instanceof Error ? error.message : String(error) });
      }
    },
  );

  router.get("/session/:sessionId", (request, response) => {
    const session = sessionService.getSession(request.params.sessionId);
    if (!session) {
      response.status(404).json({ error: "Session not found" });
      return;
    }

    response.json(session);
  });

  router.get("/session/:sessionId/audio", (request, response) => {
    try {
      const result = sessionService.getAudio(request.params.sessionId);
      if (!result) {
        response.status(409).json({ error: "Audio is not ready" });
        return;
      }

      response.setHeader("content-type", result.contentType);
      response.send(result.audio);
    } catch (error) {
      response.status(404).json({ error: error instanceof Error ? error.message : String(error) });
    }
  });

  router.post("/session/:sessionId/complete", (request, response) => {
    try {
      response.json(sessionService.completeSession(request.params.sessionId));
    } catch (error) {
      response.status(404).json({ error: error instanceof Error ? error.message : String(error) });
    }
  });

  return router;
};
