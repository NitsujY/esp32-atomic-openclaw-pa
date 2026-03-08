import "dotenv/config";

export type DeviceMode = "work" | "kid";

export type ModeRoute = {
  chatId: string;
  prefix: string;
};

const intFromEnv = (name: string, fallback: number): number => {
  const value = process.env[name];
  if (!value) {
    return fallback;
  }

  const parsed = Number.parseInt(value, 10);
  return Number.isNaN(parsed) ? fallback : parsed;
};

const boolFromEnv = (name: string, fallback: boolean): boolean => {
  const value = process.env[name];
  if (!value) {
    return fallback;
  }

  return value.toLowerCase() === "true";
};

export const bridgeConfig = {
  port: intFromEnv("PORT", 3001),
  apiKey: process.env.BRIDGE_API_KEY ?? "",
  telegram: {
    botToken: process.env.TELEGRAM_BOT_TOKEN ?? "",
    workChatId: process.env.TELEGRAM_WORK_CHAT_ID ?? "",
    kidChatId: process.env.TELEGRAM_KID_CHAT_ID ?? "",
    workPrefix: process.env.TELEGRAM_WORK_PREFIX ?? "[WORK]",
    kidPrefix: process.env.TELEGRAM_KID_PREFIX ?? "[KID]",
    pollIntervalMs: intFromEnv("TELEGRAM_POLL_INTERVAL_MS", 1500),
    replyTimeoutMs: intFromEnv("TELEGRAM_REPLY_TIMEOUT_MS", 90000),
    mockMode: boolFromEnv("TELEGRAM_MOCK_MODE", true),
  },
  openAi: {
    apiKey: process.env.OPENAI_API_KEY ?? "",
    sttModel: process.env.OPENAI_STT_MODEL ?? "whisper-1",
    ttsModel: process.env.OPENAI_TTS_MODEL ?? "gpt-4o-mini-tts",
    ttsVoice: process.env.OPENAI_TTS_VOICE ?? "alloy",
    mockMode: boolFromEnv("OPENAI_MOCK_MODE", true),
  },
};

export const routeForMode = (mode: DeviceMode): ModeRoute => {
  if (mode === "kid") {
    return {
      chatId: bridgeConfig.telegram.kidChatId,
      prefix: bridgeConfig.telegram.kidPrefix,
    };
  }

  return {
    chatId: bridgeConfig.telegram.workChatId,
    prefix: bridgeConfig.telegram.workPrefix,
  };
};
