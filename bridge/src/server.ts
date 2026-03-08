import path from "node:path";

import express from "express";

import { bridgeConfig } from "./config.js";
import { createDeviceRouter } from "./routes/device.js";
import { SessionService } from "./services/sessionService.js";
import { OpenAiSttService } from "./services/sttService.js";
import { BotTelegramService } from "./services/telegramService.js";
import { OpenAiTtsService } from "./services/ttsService.js";

const app = express();
const publicDir = path.resolve(process.cwd(), "public");
const sessionService = new SessionService({
  sttService: new OpenAiSttService(),
  telegramService: new BotTelegramService(),
  ttsService: new OpenAiTtsService(),
});

app.use(express.static(publicDir));

app.get("/simulator", (_request, response) => {
  response.sendFile(path.join(publicDir, "simulator.html"));
});

app.get("/health", (_request, response) => {
  response.json({
    status: "ok",
    simulatorPath: "/simulator",
    bridgeStubModes: {
      telegram: bridgeConfig.telegram.mockMode,
      openAi: bridgeConfig.openAi.mockMode,
    },
  });
});

app.use("/device", createDeviceRouter(sessionService));

app.listen(bridgeConfig.port, () => {
  console.info(`OpenClaw bridge listening on :${bridgeConfig.port}`);
});
