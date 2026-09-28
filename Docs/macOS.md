# macOS release preparation

The public 0.2.0 downloads currently contain Windows x64 VST3 builds. A Mac
release must pass the workflows below before its download is added to the site
or Polar. Do not label an unsigned preview as a signed public release.

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

The tested universal installer preview is 2,603,406,456 bytes. GitHub release
assets must each be smaller than 2 GiB, so the final `.pkg` is hosted in the
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
and the full remote file is read back to verify SHA-256. The helper Worker and
its credential are removed afterward; the object stays private until configured
for the storefront. The helper writes `macOS-download.json` next to the release
only after full verification. It rejects preview manifests and missing Accepted
notarization reports. Never pass signing secrets to the upload service.

```js
downloads: {
  macInstaller: 'https://hungryghostaudio.com/downloads/macos/0.2.0/<sha256>/HungryGhostSuite-0.2.0-macOS-Universal.pkg',
  macArtifact: {
    path: '/downloads/macos/0.2.0/<sha256>/HungryGhostSuite-0.2.0-macOS-Universal.pkg',
    key: 'macos/0.2.0/<sha256>/HungryGhostSuite-0.2.0-macOS-Universal.pkg',
    bytes: /* exact final file size */,
    sha256: /* exact final file SHA-256 */,
    signed: true,
    notarized: true
  }
}
```

The download route requires matching versioned paths, SHA-256 metadata and size.
It streams the file and supports single byte ranges for resumed downloads,
HEAD and conditional requests. With no published artifact configured it returns
404. The route does not publish or authenticate artifacts itself: the release
operator must finish the signing, notarization and upload checks first.

References: [GitHub release size limits](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
and [Cloudflare R2 Workers API](https://developers.cloudflare.com/r2/api/workers/workers-api-reference/).

Apple references: [Developer ID certificates](https://developer.apple.com/help/account/certificates/create-developer-id-certificates)
and [customizing notarization](https://developer.apple.com/documentation/security/customizing-the-notarization-workflow).
