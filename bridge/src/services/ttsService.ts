import { bridgeConfig } from "../config.js";

export type TtsResult = {
  audio: Buffer;
  contentType: string;
  provider: string;
  latencyMs: number;
};

export interface TtsService {
  synthesize(text: string): Promise<TtsResult>;
}

const writeWavHeader = (pcmByteLength: number, sampleRate: number): Buffer => {
  const header = Buffer.alloc(44);

  header.write("RIFF", 0);
  header.writeUInt32LE(pcmByteLength + 36, 4);
  header.write("WAVEfmt ", 8);
  header.writeUInt32LE(16, 16);
  header.writeUInt16LE(1, 20);
  header.writeUInt16LE(1, 22);
  header.writeUInt32LE(sampleRate, 24);
  header.writeUInt32LE(sampleRate * 2, 28);
  header.writeUInt16LE(2, 32);
  header.writeUInt16LE(16, 34);
  header.write("data", 36);
  header.writeUInt32LE(pcmByteLength, 40);

  return header;
};

const generateMockTone = (): Buffer => {
  const sampleRate = 16000;
  const durationMs = 700;
  const sampleCount = Math.floor((sampleRate * durationMs) / 1000);
  const pcm = Buffer.alloc(sampleCount * 2);
  const period = Math.max(1, Math.floor(sampleRate / 520));

  for (let index = 0; index < sampleCount; index += 1) {
    const step = index % period;
    const sample = Math.floor((step / period) * 16000) - 8000;
    pcm.writeInt16LE(sample, index * 2);
  }

  return Buffer.concat([writeWavHeader(pcm.length, sampleRate), pcm]);
};

export class OpenAiTtsService implements TtsService {
  async synthesize(text: string): Promise<TtsResult> {
    const startedAt = Date.now();

    if (bridgeConfig.openAi.mockMode || !bridgeConfig.openAi.apiKey) {
      return {
        audio: generateMockTone(),
        contentType: "audio/wav",
        provider: "openai-mock-tts",
        latencyMs: Date.now() - startedAt,
      };
    }

    const response = await fetch("https://api.openai.com/v1/audio/speech", {
      method: "POST",
      headers: {
        Authorization: `Bearer ${bridgeConfig.openAi.apiKey}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        model: bridgeConfig.openAi.ttsModel,
        voice: bridgeConfig.openAi.ttsVoice,
        input: text,
        response_format: "wav",
      }),
    });

    if (!response.ok) {
      throw new Error(`TTS request failed: ${response.status} ${await response.text()}`);
    }

    const payload = Buffer.from(await response.arrayBuffer());
    return {
      audio: payload,
      contentType: "audio/wav",
      provider: "openai-tts",
      latencyMs: Date.now() - startedAt,
    };
  }
}
