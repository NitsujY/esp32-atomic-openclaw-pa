import { randomUUID } from "node:crypto";

import { DeviceMode } from "../config.js";
import { SttService } from "./sttService.js";
import { TelegramService } from "./telegramService.js";
import { TtsService } from "./ttsService.js";

export type SessionStatus =
  | "idle"
  | "uploading"
  | "waiting_response"
  | "ready"
  | "completed"
  | "error";

export type SessionView = {
  sessionId: string;
  deviceId: string;
  mode: DeviceMode;
  status: SessionStatus;
  createdAt: string;
  updatedAt: string;
  transcript?: string;
  replyText?: string;
  responseBytes?: number;
  error?: string;
};

type SessionRecord = SessionView & {
  responseAudio?: Buffer;
  responseContentType?: string;
};

type SessionDependencies = {
  sttService: SttService;
  telegramService: TelegramService;
  ttsService: TtsService;
};

export class SessionService {
  private readonly sessions = new Map<string, SessionRecord>();

  constructor(private readonly deps: SessionDependencies) {}

  startSession(deviceId: string, mode: DeviceMode): SessionView {
    const now = new Date().toISOString();
    const session: SessionRecord = {
      sessionId: randomUUID(),
      deviceId,
      mode,
      status: "idle",
      createdAt: now,
      updatedAt: now,
    };

    this.sessions.set(session.sessionId, session);
    return this.toView(session);
  }

  getSession(sessionId: string): SessionView | undefined {
    const session = this.sessions.get(sessionId);
    return session ? this.toView(session) : undefined;
  }

  async submitUtterance(
    sessionId: string,
    audio: Buffer,
    contentType: string,
  ): Promise<SessionView> {
    const session = this.requireSession(sessionId);
    session.status = "uploading";
    session.updatedAt = new Date().toISOString();

    void this.processSession(session, audio, contentType);
    return this.toView(session);
  }

  getAudio(sessionId: string): { audio: Buffer; contentType: string } | undefined {
    const session = this.requireSession(sessionId);
    if (!session.responseAudio || !session.responseContentType) {
      return undefined;
    }

    return {
      audio: session.responseAudio,
      contentType: session.responseContentType,
    };
  }

  completeSession(sessionId: string): SessionView {
    const session = this.requireSession(sessionId);
    session.status = "completed";
    session.updatedAt = new Date().toISOString();
    return this.toView(session);
  }

  private async processSession(
    session: SessionRecord,
    audio: Buffer,
    contentType: string,
  ): Promise<void> {
    try {
      session.status = "waiting_response";
      session.updatedAt = new Date().toISOString();

      const stt = await this.deps.sttService.transcribe(audio, contentType);
      const telegram = await this.deps.telegramService.sendPrompt(
        session.mode,
        session.sessionId,
        stt.text,
      );
      const tts = await this.deps.ttsService.synthesize(telegram.replyText);

      session.status = "ready";
      session.updatedAt = new Date().toISOString();
      session.transcript = stt.text;
      session.replyText = telegram.replyText;
      session.responseAudio = tts.audio;
      session.responseContentType = tts.contentType;
      session.responseBytes = tts.audio.length;

      console.info(
        `[session ${session.sessionId}] ready stt=${stt.latencyMs}ms telegram=${telegram.latencyMs}ms tts=${tts.latencyMs}ms bytes=${tts.audio.length}`,
      );
    } catch (error) {
      session.status = "error";
      session.updatedAt = new Date().toISOString();
      session.error = error instanceof Error ? error.message : String(error);
      console.error(`[session ${session.sessionId}] failed`, error);
    }
  }

  private requireSession(sessionId: string): SessionRecord {
    const session = this.sessions.get(sessionId);
    if (!session) {
      throw new Error(`Unknown session ${sessionId}`);
    }

    return session;
  }

  private toView(session: SessionRecord): SessionView {
    return {
      sessionId: session.sessionId,
      deviceId: session.deviceId,
      mode: session.mode,
      status: session.status,
      createdAt: session.createdAt,
      updatedAt: session.updatedAt,
      transcript: session.transcript,
      replyText: session.replyText,
      responseBytes: session.responseBytes,
      error: session.error,
    };
  }
}
