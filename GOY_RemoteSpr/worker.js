// GOY IA worker (Cloudflare Workers) — puente entre el script y Google Gemini.
// La clave NUNCA va en el script: guárdala en el worker como secreto GEMINI_KEY
//   (Cloudflare > tu worker > Settings > Variables and Secrets > Add > Secret).
// Opcional: variable GEMINI_MODEL (por defecto gemini-2.5-flash).
//
// El script envía:  POST { kind, message, context?, game, user, history? }  (message ya trae la instrucción del botón)
// El worker responde: { reply: "..." }

const SYSTEM = [
  "Eres la IA de GOY, un asistente experto en Roblox Luau, dentro de un panel de chat.",
  "Responde siempre en español, claro y directo, como un buen compañero programador: primero la respuesta, luego lo necesario.",
  "Puedes conversar, responder preguntas, explicar señales que el servidor envió al jugador, y revisar, corregir o escribir código que el usuario te pida.",
  "Cuando des código, ponlo completo dentro de un solo bloque ```lua ... ``` sin partes omitidas, y después explica en 1 a 3 líneas qué cambiaste.",
  "En Scripts del servidor del propio usuario añade validaciones de tipos, rangos, cooldown y dueño.",
  "Reglas para que el código funcione a la primera: usa Luau moderno de Roblox (task.wait/task.spawn, game:GetService, :WaitForChild para lo que tarda en cargar),",
  "nada de funciones obsoletas (wait, spawn, delay), protege con pcall lo que puede fallar, revisa que cada function/if/for cierre con end,",
  "di dónde va (LocalScript, Script o ModuleScript y en qué carpeta), y no inventes nombres del juego: si no los sabes, ponlos como variables arriba para que el usuario los cambie.",
  "Puedes escribir respuestas y códigos largos (hasta 10,000 líneas). Si no te cabe, detente al final de una línea completa: el panel te pedirá que continúes y unirá las partes.",
  "Cuando el usuario te pase un script, léelo entero: di para qué sirve cada parte (servicios, remotos, funciones, botones del menú), qué partes se parecen o hacen lo mismo, y usa esos mismos nombres y estilo al escribir código nuevo.",
  "Usa **negrita** solo para lo importante. Si algo no está claro, pregunta una sola cosa.",
].join(" ");

const PREFIX = {
  chat: "",
  fix_code: "Corrige este código y devuélvelo completo:\n",
  analyze_signal: "Explica qué significa esta señal que el servidor me envió:\n",
};

function json(obj, status = 200) {
  return new Response(JSON.stringify(obj), {
    status,
    headers: { "Content-Type": "application/json", "Access-Control-Allow-Origin": "*" },
  });
}

export default {
  async fetch(request, env) {
    if (request.method !== "POST") return json({ reply: "GOY IA worker activo." });
    if (!env.GEMINI_KEY) return json({ reply: "Falta el secreto GEMINI_KEY en el worker." }, 500);

    let body;
    try { body = await request.json(); } catch { return json({ reply: "JSON inválido" }, 400); }
    const kind = String(body.kind || "chat");
    const message = String(body.message || "").slice(0, 400000);
    if (!message) return json({ reply: "Mensaje vacío" }, 400);

    const contents = [];
    if (Array.isArray(body.history)) {
      for (const m of body.history.slice(-8)) {
        if (!m || !m.content) continue;
        contents.push({ role: m.role === "assistant" ? "model" : "user", parts: [{ text: String(m.content).slice(0, 8000) }] });
      }
    }
    contents.push({ role: "user", parts: [{ text: message }] });
    const context = String(body.context || "").slice(0, 400000);

    const model = env.GEMINI_MODEL || "gemini-2.5-flash";
    const url = `https://generativelanguage.googleapis.com/v1beta/models/${model}:generateContent`;
    const r = await fetch(url, {
      method: "POST",
      headers: { "Content-Type": "application/json", "x-goog-api-key": env.GEMINI_KEY },
      body: JSON.stringify({ systemInstruction: { parts: [{ text: context ? SYSTEM + "\n\n" + context : SYSTEM }] }, contents,
        generationConfig: { maxOutputTokens: 65536 } }),
    });
    if (!r.ok) return json({ reply: `Gemini respondió ${r.status}: ${(await r.text()).slice(0, 300)}` }, 502);
    const data = await r.json();
    const cand = data.candidates?.[0] || {};
    const reply = (cand.content?.parts || []).map((p) => p.text || "").join("") || "(sin respuesta)";
    // truncated = la respuesta se cortó por largo; el script pide la siguiente parte.
    return json({ reply, truncated: cand.finishReason === "MAX_TOKENS" });
  },
};
