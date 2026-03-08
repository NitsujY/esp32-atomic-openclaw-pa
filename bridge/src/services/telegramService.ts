import { DeviceMode, bridgeConfig, routeForMode } from "../config.js";

type TelegramUpdate = {
  update_id: number;
  message?: {
    text?: string;
    chat?: {
      id: number | string;
    };
    from?: {
      is_bot?: boolean;
    };
  };
};

export type TelegramReply = {
  replyText: string;
  provider: string;
  latencyMs: number;
};

export interface TelegramService {
  sendPrompt(mode: DeviceMode, correlationId: string, text: string): Promise<TelegramReply>;
}

export class BotTelegramService implements TelegramService {
  private updateOffset = 0;

  async sendPrompt(mode: DeviceMode, correlationId: string, text: string): Promise<TelegramReply> {
    const startedAt = Date.now();
    const route = routeForMode(mode);

    if (bridgeConfig.telegram.mockMode || !bridgeConfig.telegram.botToken || !route.chatId) {
      return {
        replyText: `mock reply for ${mode}: ${text}`,
        provider: "telegram-mock",
        latencyMs: Date.now() - startedAt,
      };
    }

    const prompt = `${route.prefix} [${correlationId}]\n${text}`;
    await this.callTelegram("sendMessage", {
      chat_id: route.chatId,
      text: prompt,
    });

    const replyText = await this.waitForReply(route.chatId, correlationId);
    return {
      replyText,
      provider: "telegram-bot-api",
      latencyMs: Date.now() - startedAt,
    };
  }

  private async waitForReply(chatId: string, correlationId: string): Promise<string> {
    const deadline = Date.now() + bridgeConfig.telegram.replyTimeoutMs;

    while (Date.now() < deadline) {
      const payload = (await this.callTelegram("getUpdates", {
        offset: this.updateOffset,
        timeout: 10,
      })) as { result?: TelegramUpdate[] };
      const updates = payload.result ?? [];

      for (const update of updates) {
        this.updateOffset = update.update_id + 1;

        const message = update.message;
        if (!message?.text || String(message.chat?.id ?? "") !== String(chatId)) {
          continue;
        }

        if (message.from?.is_bot) {
          continue;
        }

        if (!message.text.includes(correlationId)) {
          continue;
        }

        return message.text.replace(correlationId, "").trim();
      }

      await new Promise((resolve) => {
        setTimeout(resolve, bridgeConfig.telegram.pollIntervalMs);
      });
    }

    throw new Error(`Timed out waiting for Telegram reply for correlation ${correlationId}`);
  }

  private async callTelegram(method: string, payload: Record<string, unknown>): Promise<unknown> {
    const response = await fetch(
      `https://api.telegram.org/bot${bridgeConfig.telegram.botToken}/${method}`,
      {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(payload),
      },
    );

    if (!response.ok) {
      throw new Error(`Telegram request failed: ${response.status} ${await response.text()}`);
    }

    return response.json();
  }
}
