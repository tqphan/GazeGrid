# Windows build, integration and MSI releases

Supported CI platforms are Linux x64 (Arch Linux userspace) and Windows x64
(MSVC 2022, Qt 6.8). macOS and Windows ARM64 are not currently supported.
The root CMake project now builds the full application on Windows. The earlier
`platform/windows` project remains available for isolated input-adapter tests.

## Integrated application

Windows uses SendInput for keyboard/pointer output, primary-screen desktop
capture for magnification, and SAPI speech on the speech worker thread. Existing
pause, cancellation, tracking-loss and shutdown paths release owned input.
Primary-display gaze coordinates are translated to physical virtual-desktop
coordinates before SendInput; capture uses that same primary display. Qt's
non-focus-taking, topmost window flags apply to the keyboard and target overlay.
Settings uses Qt Quick Controls without KDE QML modules. Keyboard layouts,
editor, prediction and dwell share the Linux application implementation.
Windows executes the XML pack's original shortcuts and bypasses Linux mappings.
Desktop input connects automatically during normal startup.

The installed application finds its keyboard pack beside the executable, rather
than relying on the source checkout or working directory. Program Files is not
writable by standard users: use the editor's Save As to export modified layouts
rather than expecting in-place edits to the bundled pack to succeed.

Windows virtual-gamepad output uses ViGEmBus and presents an Xbox 360 controller.
Install the [ViGEmBus driver](https://github.com/nefarius/ViGEmBus/releases) before
using gamepad output. GazeGrid links ViGEmClient statically, creates a target on
first output, neutralizes it on reset/pause, and removes it on shutdown or output
failure. Missing-driver errors include setup guidance; a later action retries.
Standard buttons, D-pad/hat axes, two sticks, analog triggers and trigger buttons
are supported. C/Z, Button1–32, Throttle, Rudder and Wheel have no Xbox 360
mapping and are rejected. Rumble feedback is not implemented.
The shared action logic and Xbox report/lifecycle tests use injected output, so
CI never needs to create a virtual device or install a kernel driver. Physical
input currently retains the imported US keyboard semantics. Mixed-DPI displays,
focus retention, elevated application input, real SAPI playback and eye-tracker
hardware require Windows acceptance testing. UIAccess does not grant control of
the secure desktop. Native Windows build and installer results must come from
Windows CI; running the Linux suite does not validate those paths.

## Development build

Install Visual Studio 2022 C++ tools, Windows SDK, CMake 3.24+, and Qt 6.8 MSVC
x64 (including SVG). Put Qt's `bin` on PATH and set `CMAKE_PREFIX_PATH` to its
installation. CMake fetches ViGEmClient at pinned commit
`b66d02d57e32cc8595369c53418b843e958649b4`; for offline builds, pass
`-DFETCHCONTENT_SOURCE_DIR_VIGEMCLIENT=C:/SDK/ViGEmClient` with that checkout.
The MSI includes the statically linked client and its license; the bus driver
remains a separate prerequisite and retains its upstream signature.
In a Visual Studio developer PowerShell:

```powershell
cmake -S . -B build/windows -A x64 -DGAZEGRID_WITH_TOBII=OFF -DGAZEGRID_UIACCESS=OFF
cmake --build build/windows --config Release --parallel
ctest --test-dir build/windows -C Release --output-on-failure
./build/windows/Release/gazegrid.exe
```

UIAccess defaults to ON for Windows. Turn it off for unsigned development builds
launched outside Program Files. Tests use an ordinary executable, then the
release script relinks with UIAccess before signing. The script checks the
embedded manifest, including `asInvoker` and `uiAccess=true`.

`GAZEGRID_WITH_TOBII=OFF` builds the full UI with mouse dwell and reports that
tracker support is unavailable. CI uses this option because proprietary SDK
headers and binaries are not stored in the repository. For eye-tracker builds,
set `GAZEGRID_WITH_TOBII=ON`, `TOBII_SDK` to a matching Windows Stream Engine SDK
with `include/tobii`, and provide its x64 runtime DLL. The existing adapter uses
the Stream Engine 3 API; a different SDK API may require adapting the loader.
Only redistribute SDK files under your vendor license.

## Persistent GazeGrid signing identity

Create the identity once on a Windows signing machine:

```powershell
./platform/windows/New-GazeGridCertificate.ps1
```

The script prompts for an output directory (press Enter to use
`GazeGrid-Signing` under your Windows user profile) and a password, with input
hidden. Save that password for the GitHub signing secret. You can still pass
`-OutputDirectory` and a SecureString `-Password` for automated use.
After creation, the script prints the GitHub environment/secret setup, release
workflow, artifact download, and target-machine certificate trust instructions.
Interactive runs also offer to copy the base64-encoded PFX to the clipboard;
fully parameterized CI calls do not pause for this prompt. The script never
prints the password or encoded PFX.

This creates a self-signed RSA/SHA-256 code-signing certificate with subject and
friendly name GazeGrid, exports a password-protected PFX and a public CER, and
prints its thumbprint. Retain that identity for subsequent releases. Private
keys must stay outside source control and release artifacts; `.gitignore`
excludes PFX/P12 files. The script does not automatically trust the certificate.

The MSI asks whether to trust the public GazeGrid certificate. **Decline** is
selected by default: it installs an unsigned executable with `uiAccess=false`
and warns that the app cannot send input to higher-privilege windows or stay on
top of all windows. Ordinary desktop input remains available. **Accept** installs
the public certificate into **Local Computer / Trusted Root Certification
Authorities** and installs the signed `uiAccess=true` executable. Both variants
install under `C:\Program Files\GazeGrid`. The MSI itself is always signed.

Verify the certificate thumbprint before accepting. Trust applies to all users
and other software signed by that identity; it does not establish public trust
or SmartScreen reputation. Only the public certificate is embedded in the MSI.
Trust remains after uninstall so existing trust and other signed applications
are not disrupted. To revoke it, an administrator can use `certlm.msc`, open
Trusted Root Certification Authorities / Certificates, verify the thumbprint,
and remove that GazeGrid certificate.

Unattended installation defaults to the unsigned app. Administrators who have
reviewed and accepted this trust can explicitly opt in:

```powershell
msiexec /i GazeGrid.msi /qn GAZEGRID_TRUST_CERTIFICATE=1
```

Repairs retain the installed mode. Uninstall and reinstall to change modes;
major upgrades ask again and unattended upgrades default to decline. Declining
never removes existing certificate trust, but still installs the unsigned app.

Microsoft documents the signature and secure-location requirements in
[UIAccess policy](https://learn.microsoft.com/en-us/windows/security/threat-protection/security-policy-settings/user-account-control-allow-uiaccess-applications-to-prompt-for-elevation-without-using-the-secure-desktop).

## Build a signed MSI

Install WiX 3 and put its `bin` directory on PATH. Import the persistent PFX into
Current User / Personal on the signing machine; ensure its public certificate
is trusted there so Authenticode verification can succeed. Then run:

```powershell
./platform/windows/Build-Release.ps1 -CertificateThumbprint YOUR_THUMBPRINT
# Tracker-enabled package:
./platform/windows/Build-Release.ps1 -CertificateThumbprint YOUR_THUMBPRINT `
  -TobiiSdk C:/SDK/StreamEngine -TobiiRuntime C:/SDK/StreamEngine/lib/x64/tobii_stream_engine.dll
```

The script builds/tests, relinks the UIAccess executable, checks its embedded
manifest, signs and verifies the executable, deploys Qt/QML, creates the MSI via
CPack/WiX, then signs and verifies the MSI with the same GazeGrid certificate.
It exports only the public certificate alongside the MSI. Packages are under
`build/windows`. Signatures use SHA-256; no timestamp service is configured, so
renew and re-sign releases before the signing certificate expires.

## CI

`.github/workflows/build.yml` builds and tests Linux and Windows on pushes and
pull requests. Windows additionally packages a signed MSI with an ephemeral
**test-only** GazeGrid identity, verifies installation and installed application
startup from outside the build tree, confirms Windows actually granted the
process a UIAccess token via `--diagnose`, and checks the default unsigned mode, repair, and declining with a previously
trusted certificate, then uninstalls. The installed smoke test
removes Qt development search paths. CI artifacts include MSI and public CER,
never the PFX. Do not distribute these changing CI identities as releases.

`.github/workflows/windows-release.yml` runs when a `v*` tag is pushed or from
**Actions → Signed Windows MSI → Run workflow**. It uses the `windows-release`
environment and these secrets:

- `GAZEGRID_SIGNING_PFX_BASE64`: base64-encoded persistent GazeGrid PFX.
- `GAZEGRID_SIGNING_PFX_PASSWORD`: its export password.

The tag must exactly match `v` plus the version in `CMakeLists.txt`, for example
`v0.1.0`. For a new release, update that version, commit and push the change, then
push the matching tag. Alternatively, manually run the workflow on the desired
commit with **Create a draft GitHub release** enabled (the default); it selects
the project version automatically. Uncheck that option for an artifact-only build.

After build, tests, signing and installed UIAccess verification succeed, the
workflow prepares `GazeGrid-VERSION-Windows-x64.msi`, `GazeGrid.cer`, SHA-256
checksums, and release notes. A separate job with repository write permission
attaches the MSI, CER and checksum file to a **draft GitHub release**. Review it
under the repository's Releases page and publish when ready. The release notes
include the source commit, signing certificate thumbprint and runtime prerequisites.
An existing version tag must point to the same source commit. Reruns may refresh
a draft; published release assets are never overwritten.

The release workflow fails if signing secrets are missing; it never silently
creates a replacement identity. Its default artifact is a mouse-dwell build;
use the local script with your licensed SDK for tracker-enabled releases.
Installing SDK provisioning in CI requires an authorized distribution source
for that SDK.
