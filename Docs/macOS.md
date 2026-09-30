# macOS release

## Published and verified 0.3.0 installer

Suite 0.3.0 includes REVERB 0.3.1 and version 0.3.0 of the other 49 effects. Its new interfaces and BOND Precision have passed universal validation, Developer ID signing and Apple notarization. The installer is published after complete remote checksum verification. The previous 0.2.0 artifact below remains available at its immutable URL.

[Download the signed Mac 0.3.0 installer](https://hungryghostaudio.com/downloads/macos/0.3.0/2138d85a37b0c36b49334ee2a6c55e37f87a7ca46625dc7e2a19cba263b0e24d/HungryGhostSuite-0.3.0-macOS-Universal.pkg).

- [Universal build 36679329788](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36679329788): native revision `c49a3de580e33d8159ddc4647050e629ec8a13f5`.
- [Signed release 36683669264](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36683669264): revision `475944b3d3a82577187fe6ffbad2a05a040ac750`, using the verified native inputs from that universal build.
- All 100 signed VST3 / AU bundles passed Intel and Apple Silicon validation. The downloaded reports independently match every executable hash and product version.
- Apple notarization returned Accepted; stapling and Gatekeeper assessment passed. Apple's installer exposed all 100 choices and installed exactly CRUSH and REEL in both formats, with matching signed hashes and no unselected bundles.

The final artifact and its manifest agree on the following evidence. The complete uploaded object was read back from private R2 storage, with its size and SHA-256 matching the local installer.

| Artifact field | Verified value |
| --- | --- |
| Installer | `HungryGhostSuite-0.3.0-macOS-Universal.pkg` |
| Bytes | `2615474937` |
| SHA-256 | `2138d85a37b0c36b49334ee2a6c55e37f87a7ca46625dc7e2a19cba263b0e24d` |
| Notarization ID | `cf100466-6800-4b8b-b966-53312e5eb121` |
| Notarization result | `Accepted` |
| Validation evidence | `HungryGhostSuite-0.3.0-macOS-validation.json` |

Website deployment `8887a60d-e42a-48c9-87ef-de8de8a9a93b` exposes the verified installer. Live HEAD requests for both 0.3.0 and the retained 0.2.0 download returned 200; three range requests, including one above 2 GiB, returned 206 with exact bytes.

The 31 storefront/API tests cover both current and historical installer delivery, including range and conditional requests; eight release-metadata checks pass. Refreshing Polar's existing customer download attachments remains pending. CI checks do not substitute for a new customer session in Logic Pro or REAPER.

Versioned filenames come from `CMakeLists.txt` and the staged release manifest. `scripts/release_metadata.py --check` verifies both catalogues before CI builds. Mac bundle validation also checks each bundle's actual version, and the signing workflow rejects a build whose native source differs from the selected successful run. Only catalogue `status` and `image` metadata may change while validation and gallery preparation finish.

See [the release procedure](Releasing.md) for exact build, publication and storefront update order.

## Verified 0.2.0 baseline

The [signed 0.2.0 Mac release](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36408294229)
passed the checks below, including all 100 universal VST3 / AU bundles on Intel
and Apple Silicon, Apple notarization, stapling and selected-installation checks.
The exact 2,604,372,347-byte installer was transferred to private R2 storage and
read back in full; its SHA-256 is
`e2aa01a2f787934097f53179d7f943c72aef7889fcf28da19d6c2b792a10af4e`.
This verified installer remains available at its immutable URL after the 0.3.0 publication.

[Download the previous signed Mac 0.2.0 installer](https://hungryghostaudio.com/downloads/macos/0.2.0/e2aa01a2f787934097f53179d7f943c72aef7889fcf28da19d6c2b792a10af4e/HungryGhostSuite-0.2.0-macOS-Universal.pkg).

## Formats and architectures

The Mac CMake configuration builds VST3 and Audio Unit v2 bundles for all 50
effects. Every bundle must contain both `arm64` and `x86_64` slices, with a minimum
deployment target of macOS 11. Audio Units are the format for Logic Pro; VST3 is
available for REAPER and other compatible hosts. Product IDs and licence benefit
IDs are shared with Windows, so existing purchases cover the Mac version.

`macos.yml` runs the native check targets and pluginval 1.0.4 at strictness 5 on
Intel, runs Apple's `auval` on all 50 Audio Units, then validates those same
universal files on an Apple Silicon runner. The architecture, actual executable
hashes and validator results are saved in the build artifacts. A successful
compile alone is not enough to ship.

On disposable CI Macs, validation installs exact AU copies into the system
Components folder, refreshes AudioComponentRegistrar and verifies that all 50
identifiers appear in `auval -a` before DSP validation. Registry logs are retained.
`macos-validate-built.yml` can recover bundles from a completed `macos.yml` run
whose compile step succeeded. It requires unchanged native sources and repeats
all native and plugin checks on Intel and Apple Silicon before packaging.

## Installer

`macos-package.yml` accepts a successful universal build run ID and makes a
clearly named unsigned installer preview for packaging checks. Apple's actual
Installer must expose all 100 plugin/format choices. An isolated CI Mac installs
only CRUSH and REEL in both formats and verifies their installed hashes.

The native installer installs selected bundles in
`/Library/Audio/Plug-Ins/VST3` and `/Library/Audio/Plug-Ins/Components`. It requires
administrator approval for those shared folders. Close audio hosts first.
Existing project files and licence caches are outside the installer payload.

## Signing credentials

The signed release workflow is manually dispatched on `main`. Use GitHub Actions
secrets in **HungryGhostStudios/hungry-ghost-audio**, never public repository files:

| Secret | Value |
| --- | --- |
| `MACOS_APPLICATION_P12` | Base64 of the Developer ID Application certificate and matching private key in an encrypted PKCS#12 file |
| `MACOS_INSTALLER_P12` | Base64 of the Developer ID Installer certificate and matching private key in an encrypted PKCS#12 file |
| `MACOS_CERTIFICATE_PASSWORD` | Password protecting the Application PKCS#12 file; also used for Installer unless its separate password is supplied |
| `MACOS_INSTALLER_CERTIFICATE_PASSWORD` | Optional separate password for the Installer PKCS#12 file |
| `MACOS_APPLICATION_IDENTITY` | Full `Developer ID Application: …` identity from the certificate |
| `MACOS_INSTALLER_IDENTITY` | Full `Developer ID Installer: …` identity from the certificate |
| `MACOS_APPLE_ID` | Apple ID used for notarization |
| `MACOS_TEAM_ID` | Apple Developer team ID |
| `MACOS_NOTARY_PASSWORD` | An Apple app-specific password for notarization |

Apple's certificate download contains the public certificate. The private key
remains on the machine that created its certificate request. Existing identities
must be exported with their private keys, or a dedicated CI certificate pair
must be issued using new certificate requests. Creating new certificates and
new account credentials requires the account owner's authorization; do not
revoke existing certificates to make room automatically.

Certificates are imported into a temporary CI keychain. Plugins receive a
Developer ID Application signature, secure timestamp and hardened runtime.
The exact signed files are revalidated on Intel and Apple Silicon before the
installer is signed with Developer ID Installer, submitted with `notarytool`,
checked for an Accepted result, stapled and assessed with Gatekeeper. Temporary
keychains are removed after each signing job.

The signed installer is published only after all checks succeed. Add the Mac
installer as a second file benefit in Polar without removing the Windows file;
the same benefit is already attached to all 51 products. The website should
offer explicitly labelled Windows and macOS downloads.

## Large installer downloads

The verified 0.3.0 installer is 2,615,474,937 bytes. GitHub release
assets must each be smaller than 2 GiB, so universal `.pkg` installers are hosted in the
private Cloudflare R2 bucket `hungry-ghost-audio-releases`, bound to the storefront
as `RELEASES`. Keep the source archive and small validation reports on GitHub.
Do not make the bucket itself public or upload signing material into it.

After the signed workflow succeeds, verify the final installer SHA-256 against
`macOS-manifest.json` and the Accepted notarization report. Upload that exact
stapled file using multipart transfer, verify the downloaded object's complete
checksum, and only then add these fields to the trusted release configuration:

`node scripts/macos/upload_release.mjs /path/to/the/final.pkg` performs this
transfer using a temporary, isolated Worker and the existing Wrangler login.
Its bearer credential expires after two hours, each 32 MiB part is checksummed,
and the full remote file is read back to verify SHA-256. The client waits for
secret deployment to reach the edge and allows a long verification download.
An existing object at the exact checksum key must pass the same complete
verification before it can be configured publicly. The helper Worker and
its credential are removed afterward; the object stays private until configured
for the storefront. The helper writes `macOS-download.json` next to the release
only after full verification. It rejects preview manifests and missing Accepted
notarization reports. Never pass signing secrets to the upload service.

```js
downloads: {
  macInstaller: 'https://hungryghostaudio.com/downloads/macos/0.3.0/<sha256>/HungryGhostSuite-0.3.0-macOS-Universal.pkg',
  macArtifact: {
    path: '/downloads/macos/0.3.0/<sha256>/HungryGhostSuite-0.3.0-macOS-Universal.pkg',
    key: 'macos/0.3.0/<sha256>/HungryGhostSuite-0.3.0-macOS-Universal.pkg',
    bytes: /* exact final file size */,
    sha256: /* exact final file SHA-256 */,
    signed: true,
    notarized: true
  },
  macArtifacts: [
    /* Complete, previously verified artifact records with distinct immutable paths. */
  ]
}
```

Before promoting a new current artifact, retain the complete previous verified record
in `macArtifacts`. Each immutable path must occur exactly once across that history
and `macArtifact`; duplicate paths fail closed. No path is inferred from a version
number or discovered by listing the bucket.

The download route requires matching versioned paths, SHA-256 metadata and size.
It streams the file and supports single byte ranges for resumed downloads,
HEAD and conditional requests. With no published artifact configured it returns
404. The route does not publish or authenticate artifacts itself: the release
operator must finish the signing, notarization and upload checks first.

References: [GitHub release size limits](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
and [Cloudflare R2 Workers API](https://developers.cloudflare.com/r2/api/workers/workers-api-reference/).

Apple references: [Developer ID certificates](https://developer.apple.com/help/account/certificates/create-developer-id-certificates)
and [customizing notarization](https://developer.apple.com/documentation/security/customizing-the-notarization-workflow).
