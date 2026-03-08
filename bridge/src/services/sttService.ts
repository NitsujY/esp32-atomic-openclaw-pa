import { bridgeConfig } from "../config.js";

export type SttResult = {
  text: string;
  provider: string;
  latencyMs: number;
};

export interface SttService {
  transcribe(audio: Buffer, contentType: string): Promise<SttResult>;
}

export class OpenAiSttService implements SttService {
  async transcribe(audio: Buffer, contentType: string): Promise<SttResult> {
    const startedAt = Date.now();
    const audioBytes = audio.buffer.slice(
      audio.byteOffset,
      audio.byteOffset + audio.byteLength,
    ) as ArrayBuffer;

    if (bridgeConfig.openAi.mockMode || !bridgeConfig.openAi.apiKey) {
      return {
        text: `mock transcript (${audio.length} bytes ${contentType})`,
        provider: "openai-mock-stt",
        latencyMs: Date.now() - startedAt,
      };
    }

    const form = new FormData();
    form.append("model", bridgeConfig.openAi.sttModel);
    form.append(
      "file",
      new Blob([audioBytes], { type: contentType }),
      contentType === "audio/wav" ? "utterance.wav" : "utterance.pcm",
    );

    const response = await fetch("https://api.openai.com/v1/audio/transcriptions", {
      method: "POST",
      headers: {
        Authorization: `Bearer ${bridgeConfig.openAi.apiKey}`,
      },
      body: form,
    });

    if (!response.ok) {
      throw new Error(`STT request failed: ${response.status} ${await response.text()}`);
    }

    const payload = (await response.json()) as { text?: string };
    return {
      text: payload.text ?? "",
      provider: "openai-whisper",
      latencyMs: Date.now() - startedAt,
    };
  }
}
