# ADR 0002 — Flux Update Service (accepted requirement, not yet implemented)

**Status:** Accepted as a product requirement. **Deliberately not implemented.**
**Scheduled:** after the Runtime Supervisor and Diagnostics Channel exist; integrates with the
Flux Console WebUI. Explicitly **out of scope** for the Stage 1 telemetry cutover.

## Why this record exists

The update requirement was approved while the V2 telemetry cutover was in progress. It is
recorded here so the decision is not lost and so its implementation does not get mixed into the
cutover, where it would blur an already large runtime change.

## What already exists (do not confuse with this ADR)

A minimal release-advertisement path landed on `main` (commit `e41c391`):

- `compile_zip.sh` stamps `module.prop`'s `updateJson` with
  `https://github.com/<repo>/releases/latest/download/update.json`, derived from
  `GITHUB_REPOSITORY` so a fork advertises its own releases.
- `gen_update_json.sh` emits the manifest (`version`, `versionCode`, `zipUrl`, `changelog`).
- The release step publishes `update.json` + `changelog.md` as release assets and uses the
  changelog as the release notes.

That is **only the manager-poll channel**: Magisk/KernelSU/APatch compare `versionCode` and offer
a direct download+install. It performs no semantic version comparison, no channel selection, no
schema-compatibility check, and no checksum verification. It is a foundation, not the service
described below.

## Requirement

Flux must provide an Update Service and a user-visible notification when a newer **compatible**
release is available, surfaced through:

1. the official Flux GitHub Releases channel,
2. the configured KernelSU-compatible module update channel,
3. the Flux application / WebUI update surface.

## Constraints the implementation must honour

**Provenance**
- Use only Flux-owned release infrastructure.
- Never point at Encore repositories or metadata. There is deliberately no fallback destination.

**Correctness**
- Compare semantic versions safely (never a lexical or numeric-suffix compare).
- Distinguish stable and prerelease channels.
- Verify compatibility with the **installed SynthesisCore schema** before offering an update —
  an update that requires a schema the device's SynthesisCore does not emit must not be offered
  as installable. This is the same `schema_min`/`schema_max` contract the build enforces via
  `dependencies/synthesiscore.lock`.
- Verify release metadata and checksums **before** installation, not after download.

**Consent**
- Never install automatically without explicit user approval.
- Support **download**, **install**, **later**, and **ignore this version**.
- Avoid repeated notification spam: an ignored version stays ignored; "later" must back off
  rather than re-prompt on every poll.

**Resilience**
- Work when either GitHub or the root-manager update source is unavailable.
- Retain a safe offline state — no update source must ever be a precondition for Flux running.
  (This mirrors the telemetry rule: a missing dependency degrades Flux, it does not stop it.)
- Show the release source and changelog so the user can see what they are accepting and from
  where.

**Security**
- Never expose GitHub or Telegram credentials in the module. The module is user-readable on
  device; any token shipped in it is a published token.

**Integration**
- Integrate with the Runtime Supervisor and the Diagnostics Channel once those exist.
- Provide update status in the future Flux Console WebUI.

## Non-goals

- No automatic/silent installation.
- No implementation during the Stage 1 telemetry cutover or the Stage 2 execution cutover.
- No dependency on Encore-owned infrastructure of any kind (see `docs/provenance-matrix.md`,
  Category E).

## Open questions (resolve at design time)

- Where the ignore/later state lives once the Configuration Store exists.
- Whether schema compatibility is read from the installed SynthesisCore at runtime or from the
  release manifest's declared range (probably both: the manifest declares, the device confirms).
- Poll cadence and its interaction with the Runtime Supervisor's scheduling.
