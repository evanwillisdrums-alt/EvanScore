# Optional local mallet guidance

The assistant is an optional explanation and candidate-selection layer. Normal
notation, Diagnostics, picking, revoicing, audition and Apply work with it off.
It starts off on every panel opening. Nothing is downloaded, inferred or loaded
while EvanScore starts, while you select chords, or when you enable the option.

## Use

1. Install [Ollama](https://ollama.com/download) on the computer running EvanScore.
   Its local service must be running on `127.0.0.1:11434`.
2. Install a small **local** instruction model once, for example with
   `ollama pull qwen2.5:1.5b`. Read the license for the exact model/tag you use.
   EvanScore does not install a runtime or bundle model weights.
3. Open Mallets, select a score chord or pick bars, then open **Player** and enable
   **Optional local AI guidance**. Click **Check local models**, select a model,
   then **Explain this chord**. Discovery lists weights without loading them.
4. Read the benefits, drawbacks and reference IDs alongside ordinary Diagnostics.
   **Preview checked candidate** only previews. **Apply to score** is still the
   separate, undoable action that edits notes/sticking. A `none` answer has no
   preview action. Cancelling, changing score/selection/settings/strike points,
   or closing the panel discards pending and old guidance.

The service is local, needs no API key, and receives only bounded chord facts,
candidate measurements, changes and neighboring-attack timing. It receives no
score file, title, audio, unrelated text or full research PDF. The client allows
only the numeric loopback address, bypasses external proxies for that address,
does not follow redirects and does not send cookies. It excludes cloud aliases,
non-GGUF entries, tiny remote placeholders and weights larger than 2 GiB. Use
genuine locally installed weights; the independently installed service remains
responsible for its own configuration.

## Resource and licensing assessment

Ollama's [runtime license](https://github.com/ollama/ollama/blob/main/LICENSE)
is MIT. Model licenses are separate and must be checked for the exact weights;
runtime licensing does not license every model. No third-party model or runtime
is redistributed by this change. The example model is a setup example, not a
claim that every similarly named model has the same license or musical ability.

A roughly 1 GB quantized 1–2B model is a reasonable starting experiment, with
several additional GB of available RAM. Disk size is not runtime memory use.
The client refuses inference below a conservative available-memory budget of
the larger of 3 GiB and twice the weight size plus 1 GiB on Windows/Linux.
This snapshot cannot guarantee that other apps will not consume memory afterward;
available RAM is not measured on unsupported platforms. CPU inference can be
slow; a compatible GPU is optional and managed by the local runtime.

One request runs at a time, with four CPU threads requested, 4096-token context,
384 output tokens, a conservative 10 KB serialized request budget, 150-second
timeout and 256 KB response limit. The original and earliest fixed-link
revoicings take priority when trimming candidates. At most five candidates are
sent. `keep_alive: 0` asks Ollama to release the model after each response. The
runtime may still use temporary memory during cancellation/cleanup; EvanScore
does not keep a model resident or schedule background inference. Other clients'
model use is outside EvanScore's control.

## What is checked, and what is not

The model may select only a supplied candidate or `none`. A response referencing
an unknown, invalid or uncertain candidate, invented editable fields, malformed
JSON, incomplete output, or unavailable source IDs is rejected. The AI cannot
write notes, change mallet assignments, bypass geometry checks or commit edits.
Preview checks the current score again, preventing old guidance from targeting
a newly selected chord.

Generated prose can still be wrong. Candidate validity means the deterministic
model's checks passed, not that human biomechanics or musical judgment is proven.
The assistant currently cannot invent additional voicings outside the bounded
search, demonstrate full passage motion, or replace a performer's review.

Grounding is a concise, reviewed summary, not model fine-tuning or automatic
ingestion of entire articles:

- **USER:** the latest user specification, fixed identities, revoicing priority,
  E/G–F♯/B♭ exception and adjustable inward preference.
- **SYNTHESIS:** the supplied research syntheses, app specification, preservation
  update, transcripts and grip/reference audits. Center normally; selective edge
  access has tradeoffs; geometry and comfort limits require calibration.
- **MODEL:** actual candidate facts and their limits. Pair-plane tilt is not a
  measured wrist angle; nearby attack estimates do not prove continuous motion.

The primary book chapters and external technique articles listed in
[research sources](mallet-research-sources.md) have not all been independently
retrieved. They are source leads, not fabricated citations. The old preservation
update preferred reassignment before revoicing for unwritten chords; the user's
October 19 clarification supersedes that recommendation order.

## Verification scope

The repository tests use a controlled local HTTP service for discovery, actual
request/response handling, disabled/idle behavior, cancellation, rejection and
resource bounds. They do not establish real language-model musical expertise.
Real model output and speed require a separately installed runtime and weights.
Windows release checks also exercise the real optional toggle without starting
inference. See the development checkpoint for executed results and build status.

The code remains responsible for reading sticking, physical identity, spelling,
voicing generation, geometry checks, audition and score edits. Insufficient RAM
disables only AI inference. **Diagnostics → EvanScore feature diagnostics** shows
live system memory used %, EvanScore's working set in MiB, and that working set
as a percentage of physical RAM. It refreshes these small measurements every two
seconds only while visible; it does not rescan the score on that timer. The JSON
report includes the percentages as a snapshot when refreshed. Ollama's separate
process contributes to system load, not EvanScore's process working set.
