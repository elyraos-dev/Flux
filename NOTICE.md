# NOTICE

Flux Tweaks
Copyright (C) 2024-2026 FebriCahyaa

This product includes software developed as part of the Flux project and
software derived from third-party works, as described below. This NOTICE file
is provided in accordance with Section 4(d) of the Apache License, Version 2.0.

Operational branding (release destinations, artifact names, user-facing product
names, notification text) is Flux-owned. The attributions below are retained for
legal reasons and MUST NOT be removed merely to eliminate upstream references.
Each attribution is removed only after the corresponding component has been
independently reimplemented and validated, and no derived code, assets,
structure, or distinctive expression from that source remains in the distributed
component. See `docs/provenance-matrix.md` for the per-component status.

--------------------------------------------------------------------------------

## 1. Derived work — Encore Tweaks

Portions of Flux are materially derived from:

    Encore Tweaks
    Copyright (C) 2024-2026 Rem01Gaming
    Licensed under the Apache License, Version 2.0
    https://github.com/Rem01Gaming/encore

The following areas of the current Flux distribution contain code, scripts,
data, module structure, or WebUI expression that is adapted from Encore Tweaks
and remains under Apache-2.0 while that derived material is present:

  - the native daemon and supporting modules under `jni/` (excluding the
    SynthesisCore integration and `jni/tests/`, which are Flux-authored);
  - the profiler and utility shell scripts under `scripts/`;
  - the Magisk/KernelSU/APatch module scripts under `module/`;
  - the WebUI application under `webui/src/`;
  - the game-detection data in `gamelist.txt`.

Files adapted from Encore Tweaks carry both the original copyright notice and a
notice that they have been modified by the Flux project. These attributions will
be retired per component as each area is independently rewritten under the Flux
Platform V2 program.

--------------------------------------------------------------------------------

## 2. Bundled third-party components

### spdlog
Copyright (c) 2016 - present, Gabi Melman and spdlog contributors.
Licensed under the MIT License.
Included as a git submodule at `jni/external/spdlog`.
https://github.com/gabime/spdlog

### RapidJSON
Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.
Licensed under the MIT License (binary distribution).
Included as a git submodule at `jni/external/rapidjson`.
https://github.com/Tencent/rapidjson

### Google Sans Flex (WebUI font)
Font files under `webui/src/assets/fonts/googlesansflex-*.woff2`.
"Google Sans" is a proprietary Google typeface. Its redistribution terms are
NOT Apache-2.0 and are unresolved for this distribution. This component is
flagged as a licensing risk in `docs/provenance-matrix.md` and is a candidate
for replacement with an openly licensed typeface before a stable release.

--------------------------------------------------------------------------------

## 3. WebUI runtime dependencies

The WebUI bundles production JavaScript dependencies resolved via `webui/bun.lock`
(including Vue, Vue Router, Pinia, Tailwind CSS, kernelsu, webuix). Each carries
its own license (predominantly MIT); those licenses are preserved by their
respective packages. A full software bill of materials (SBOM) is produced as part
of the release pipeline.

--------------------------------------------------------------------------------

## 4. Flux-owned original work

SynthesisCore (the telemetry provider consumed by Flux), the SynthesisCore
integration layer, the daemon host-side tests, and the SynthesisCore dependency
tooling are original Flux work and are not derived from Encore Tweaks. See the
provenance matrix for the authoritative per-component classification.
