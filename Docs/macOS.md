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
| `MACOS_CERTIFICATE_PASSWORD` | Password shared by those two encrypted PKCS#12 files |
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

Apple references: [Developer ID certificates](https://developer.apple.com/help/account/certificates/create-developer-id-certificates)
and [customizing notarization](https://developer.apple.com/documentation/security/customizing-the-notarization-workflow).
