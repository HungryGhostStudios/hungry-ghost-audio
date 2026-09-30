# Releasing the suite

Release versions are defined in `CMakeLists.txt`: the project version is the suite and default product version; `HG_REVERB_VERSION` preserves REVERB's existing higher version history. Current preparation targets suite 0.3.0, REVERB 0.3.1, and 0.3.0 for the other 49 plugins. Keep previous release assets and immutable Mac download paths intact.

## Prepare and validate

1. Finish the native source and UI review. Run `python scripts/catalogue.py` after a version change and `python scripts/release_metadata.py --check`. The generator updates the two catalogues, `Source/Catalogue.h` and `Products.cmake`; it does not rewrite the top-level CMake configuration or custom presets.
2. After a version change, configure in a fresh Windows build directory so JUCE regenerates every plugin's version resource. If reusing a build directory, reconfigure first, then run that build's `juceaide rcfile <current-Info.txt> <target_resources.rc>` for each affected plugin before rebuilding; do not hand-edit generated resource versions. A newer `moduleinfo.json` or an ordinary incremental relink alone does not prove the PE resource was refreshed. Build all 50 Windows VST3 targets with the six native test targets. Run CTest, inspect the native screenshots at supported sizes, and exercise controls and state restoration. Use an isolated trial/test cache as documented by the native tests.
3. Run `python scripts/test_release_metadata.py`, then `python scripts/validate_release.py --build <build-directory> --validator <pluginval-1.0.4.exe> --output <validation-directory> --workers 3`. All 50 exact binaries must pass strictness 5. Before invoking pluginval, the validator independently checks both `moduleinfo.json` and the Windows PE `ProductVersion` against the catalogue; it rejects missing or stale resources and records the verified PE version alongside the exact binary hash.
4. Run `python scripts/catalogue.py --validated <validation-directory>/results.json`. This verifies every binary hash and version before changing public catalogue status/images. Copy reviewed native screenshots into `site/public/assets/plugins/`; do not substitute mockups for product captures.
5. Commit the reviewed release source and push `main`. The Windows and universal Mac workflows run from this checkpoint. Mac CI may start before step 4 finishes: later commits may change only catalogue `status`/`image` metadata, documentation and storefront assets; native build inputs must stay identical to the successful Mac build.

## Windows packages and corresponding source

Use a new versioned release directory and a separate temporary package-work directory:

```text
python scripts/package_release.py --validation <validation-directory>/results.json --output <release-directory> --work <package-work>
powershell -File scripts/build_installer.ps1 -ReleaseDirectory <release-directory> -HashManifest <package-work>/installer-hashes.tsv
python scripts/source_release.py --juce <official-JUCE-9.0.2-checkout> --output <release-directory>/HungryGhostSuite-0.3.0-Complete-Source.zip
```

Build the source archive from the final reviewed checkout, with no unrelated scratch files in the repository. Check archive integrity and verify an offline build uses bundled JUCE. Verify the installer payload hashes and a selected extraction; replacement/rollback behaviour must also be checked when installer logic changes. The generated installer reads its release version from an embedded resource derived from CMake.

Create SHA256SUMS after all files are final. Publish the Windows installer, suite ZIP, 50 individual ZIPs, complete source, manifest and checksums to a new `v0.3.0` GitHub release. Compare each uploaded asset digest with its local checksum. Do not overwrite the previous tag or infer success merely from an upload starting.

## Signed universal Mac installer

1. Wait for the `macOS universal plugins` run for the release checkpoint to succeed. It checks all 100 VST3/AU bundles and native test programs on Intel and Apple Silicon.
2. Dispatch `macos-release.yml` on `main` with `build_run` set to that successful run ID. The workflow verifies native source provenance, imports the existing GitHub signing secrets into a temporary keychain, signs the binaries and revalidates their exact signed bytes on both architectures.
3. The final job builds the selectable installer, signs it, requires Apple's notarization status `Accepted`, staples the ticket, assesses Gatekeeper and verifies selected installation. Download the `mac-signed-release` artifact only after that run succeeds.
4. Keep `macOS-manifest.json` and `notarization.json` beside the final `.pkg`. Run `node scripts/macos/upload_release.mjs <path-to-final.pkg>` using the existing Wrangler login. The helper uses a temporary authenticated Worker to upload the private R2 object and verify its complete remote SHA-256; it writes `macOS-download.json` only after successful verification and removes the temporary helper.
5. Use the exact verified `macOS-download.json` fields when configuring the storefront. Keep source and small validation evidence on GitHub; the large Mac installer uses private R2 because the previous universal package exceeded GitHub's per-asset size limit.

If validation recovery is needed, `macos-validate-built.yml` accepts preserved bundles only when the native sources still match. An unsigned preview is never a substitute for the public signed installer.

## Storefront and customer delivery

1. Change `site/src/release-config.mjs` only after artifact verification. Update all 50 Windows links according to each catalogue version, the suite installer/ZIP/source links, and the Mac `macInstaller` plus complete `macArtifact` metadata. Preserve the 51 Polar product/checkout mappings and licence benefits.
2. Check whether a deployed `STORE_CONFIG` override exists before assuming the source module controls production. The September 30 audit found no such override; only the licence-signing secret was configured. Never print or copy secret values during routine release work.
3. Run all storefront tests, then deploy from `site` with the existing Cloudflare Worker configuration. Verify `/api/config`, `/catalogue.json`, visible download links, current screenshot images and checkout routing. Read back the Windows assets and test Mac HEAD, normal ranges and ranges above 2 GiB against the final artifact.
4. Update the existing Polar file-download benefit with the new Windows and Mac installers while retaining both platforms and all existing product associations. Verify customer access on the existing purchase. Website download publication and a Polar file upload are separate operations; do not report the latter until it has completed and saved.
5. Replace release-candidate wording in the README/release notes and update `Docs/macOS.md` and launch evidence with the actual run IDs, hashes, sizes and tested scope. A new Mac host session in Logic/REAPER is useful follow-up evidence; CI validation alone does not establish every host workflow.

For rollback, point the storefront back to the previous verified artifact URLs and its matching gallery/catalogue. Keep the new release evidence available for diagnosis; do not mutate old installer objects.
