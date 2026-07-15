# Flux Provenance Matrix

Internal engineering and compliance record for the Flux Platform V2 rearchitecture.

**Purpose.** This document records, per component, where the current Flux
distribution stands relative to its origins so that license obligations are met
while derived material remains, and so that attribution is retired only after a
component is genuinely reimplemented. It is the authoritative source referenced
by [`NOTICE.md`](../NOTICE.md).

**What this file is not.** It does not reproduce third-party source. It records
provenance *conclusions*, not comparison dumps or copied excerpts.

## Classification legend

| Category | Meaning | Default action |
| --- | --- | --- |
| **A** | Independently written; not materially derived from Encore | Retain; test; document ownership |
| **B** | Generic/commonplace infrastructure | Evaluate normally; do not rewrite for artificial difference |
| **C** | Materially derived from / adapted from Encore Tweaks | Keep attribution; replace with independent implementation; remove legacy path after validation |
| **D** | Uncertain provenance | Treat conservatively as derived; preserve any header; resolve or replace before stable release |
| **E** | Third-party dependency consumed intentionally | Preserve license; pin version; do not modify vendored code |

Upstream reference for Category C/D: Encore Tweaks, Copyright (C) 2024–2026
Rem01Gaming, Apache-2.0 — https://github.com/Rem01Gaming/encore

## Component matrix

| Path | Purpose | Category | Evidence | Notices retained | Replacement status | Release eligibility |
| --- | --- | --- | --- | --- | --- | --- |
| `jni/Main.cpp`, `jni/Profiler.*`, `jni/InotifyHandler.*`, `jni/FluxCLI.*`, `jni/FluxConfigStore.*`, `jni/DeviceMitigationStore.*` | Native daemon entry, profiler loop, inotify handling, CLI, config/mitigation stores | C | Structure, control flow, and identifiers trace to the Encore daemon | Dual header (Rem01Gaming + FebriCahyaa + "Adapted from Encore Tweaks / Modified by the Flux project"); NOTICE §1 | **Pending** — to be replaced by the V2 engine (`telemetry`/`decision`/`execution`/`supervision`) | Ships under Apache-2.0 with attribution; not yet independent |
| `jni/base/DeviceInfo/*`, `jni/base/GameRegistry/*`, `jni/base/InotifyWatcher/*`, `jni/base/PIDTracker/*`, `jni/base/ProfilePolicy/*`, `jni/base/FluxUtility/*` | Device probing, game registry, inotify watcher, PID tracking, profile policy, shell/process utilities | C | Adapted from corresponding Encore modules | Dual header; NOTICE §1 | **Pending** — ProfilePolicy is first target for the pure V2 Decision Engine | Ships under Apache-2.0 with attribution |
| `jni/include/*.hpp` (`Flux.hpp`, `FluxLog.hpp`, `ModuleProperty.hpp`, `ShellUtility.hpp`, `SignalHandler.hpp`, `Write2File.hpp`) | Shared daemon headers/utilities | C | Adapted from Encore headers | Dual header; NOTICE §1 | Pending | Ships under Apache-2.0 with attribution |
| `jni/base/SynthesisCore/*` | SynthesisCore telemetry integration (decode/consume snapshots) | A | Flux-authored; Flux-only copyright header; consumes the SynthesisCore v2 contract that Encore has no equivalent of | Flux copyright only; NOTICE §4 | N/A (already independent) | Eligible |
| `jni/base/LockFile/*` | Single-instance / duplicate-daemon lock | B/D | No header present; standard flock-style pattern; provenance not individually confirmed | None yet | **Provenance-unresolved** — add Flux header after confirming no derived expression, or replace | Add header before stable release |
| `jni/tests/*` (`TelemetryParserTest`, `ProfilePolicyTest`, `InotifyIntegrationTest`, `TestFramework.hpp`, `run_tests.sh`, stubs) | Host-side unit/integration tests | A | Flux-authored; Flux-only header; no Encore counterpart | Flux copyright only; NOTICE §4 | N/A | Eligible |
| `scripts/flux_profiler.sh`, `scripts/flux_utility.sh` | Profiler and utility shell entry points | C | Adapted from Encore `encore_profiler` / `encore_utility` | Dual header; NOTICE §1 | Pending | Ships under Apache-2.0 with attribution |
| `scripts/fetch-synthesiscore.sh`, `scripts/verify-synthesiscore.sh`, `scripts/update-synthesiscore-lock.sh` | SynthesisCore dependency fetch/verify/lock tooling | A | Flux-authored; no Encore analogue | Flux copyright | N/A | Eligible |
| `module/*.sh` (`action`, `cleanup`, `customize`, `service`, `uninstall`, `verify`) | Magisk/KernelSU/APatch module lifecycle scripts | C | Adapted from Encore module scripts; operational branding (paths, process names) changed encore→flux | Dual header; NOTICE §1 | Pending | Ships under Apache-2.0 with attribution; operational branding is Flux-owned |
| `webui/src/**` | Module WebUI (Vue 3 SPA) | C | Adapted from Encore `encore_spa` WebUI; package renamed `encore_spa`→`flux_spa` | NOTICE §1 | **Pending** — full reconception as the independent "Flux Console" | Ships with attribution; not yet independent |
| `webui/src/main.js`, standard Vite/Vue bootstrap files | Framework bootstrap | B | Generic Vue/Vite scaffolding | — | Retain | Eligible |
| `gamelist.txt` | Game-detection package list | C | Data adapted from Encore's game list | NOTICE §1 | Pending — to be regenerated/curated independently | Ships with attribution |
| `jni/external/spdlog` | Logging library (submodule) | E | Upstream MIT | NOTICE §2 (spdlog) | Do not modify | Eligible |
| `jni/external/rapidjson` | JSON parsing (submodule) | E | Upstream MIT | NOTICE §2 (RapidJSON) | Do not modify | Eligible |
| `webui/src/assets/fonts/googlesansflex-*.woff2` | WebUI typeface | E (risk) | "Google Sans" is a proprietary Google typeface; redistribution terms are **not** Apache-2.0 and are unresolved | NOTICE §2 (Google Sans Flex) | **Licensing risk** — replace with an openly licensed typeface before stable release | **Blocked for stable release** until resolved |
| `webui/bun.lock` production deps (Vue, Vue Router, Pinia, Tailwind, kernelsu, webuix) | WebUI runtime dependencies | E | Predominantly MIT, preserved by their packages | NOTICE §3; SBOM at release | Do not vendor-modify | Eligible |
| `dependencies/synthesiscore.lock`, `prebuilt/synthesiscore.apk` | Pinned SynthesisCore artifact + checksum lock | A/E | SynthesisCore is a separate Flux-owned project consumed as a locked, checksum-verified artifact | NOTICE §4 | N/A | Eligible (checksum-gated in CI) |

## Open compliance items (must close before declaring V2 independence)

1. **Google Sans Flex** — unresolved redistribution license. Replace with an
   openly licensed typeface (Category E risk). **Release-blocking.**
2. **`jni/base/LockFile`** — provenance-unresolved (Category B/D). Confirm no
   derived expression and add a Flux header, or replace.
3. All **Category C** components still ship under Apache-2.0 with Encore
   attribution. Attribution is retired *per component* only after independent
   reimplementation is validated and no derived code/assets/structure/expression
   remains — never merely because a file was renamed or partially rewritten.

## Attribution-retirement rule

For any Category C/D component, before removing its Encore attribution:

- the replacement is independently designed against the Flux V2 specification
  (not a line-by-line transform);
- behavior parity is proven by tests;
- the legacy runtime path is removed;
- a similarity check confirms no distinctive Encore expression remains.

Only then is the attribution removed from *that component*, and NOTICE.md
updated accordingly.
