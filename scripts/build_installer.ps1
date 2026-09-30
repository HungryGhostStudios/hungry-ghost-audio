param(
  [Parameter(Mandatory=$true)][string]$ReleaseDirectory,
  [Parameter(Mandatory=$true)][string]$HashManifest
)
$ErrorActionPreference = 'Stop'
$suiteSourceRoot = Split-Path -Parent $PSScriptRoot
$suiteReleaseRoot = (Resolve-Path -LiteralPath $ReleaseDirectory).Path
$suiteHashesPath = (Resolve-Path -LiteralPath $HashManifest).Path
$suiteCmake = Get-Content -LiteralPath (Join-Path $suiteSourceRoot 'CMakeLists.txt') -Raw
$suiteVersionMatch = [regex]::Match($suiteCmake, 'project\(HungryGhostSuite\s+VERSION\s+(\d+\.\d+\.\d+)\b')
if (-not $suiteVersionMatch.Success) { throw 'Missing suite version in CMakeLists.txt.' }
$suiteVersion = $suiteVersionMatch.Groups[1].Value
$suiteManifest = Get-Content -LiteralPath (Join-Path $suiteReleaseRoot 'manifest.json') -Raw | ConvertFrom-Json
if ($suiteManifest.suiteVersion -ne $suiteVersion) { throw 'Release manifest version differs from source.' }
$suiteVersionPath = Join-Path (Split-Path -Parent $suiteHashesPath) 'installer-version.txt'
[IO.File]::WriteAllText($suiteVersionPath, $suiteVersion, [Text.UTF8Encoding]::new($false))
$suiteCompilerPath = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$suiteCompileArguments = @(
  '/nologo', '/target:winexe', '/platform:x64', '/optimize+',
  '/r:System.Windows.Forms.dll', '/r:System.Drawing.dll',
  '/r:System.IO.Compression.dll', '/r:System.IO.Compression.FileSystem.dll',
  ('/win32icon:' + (Join-Path $suiteSourceRoot 'Design\Brand\hungry-ghost.ico')),
  ('/resource:' + (Join-Path $suiteSourceRoot 'Design\Brand\hungry-ghost.ico') + ',brand-icon'),
  ('/resource:' + (Join-Path $suiteSourceRoot 'Design\Brand\hungry-ghost-mark.png') + ',brand-mark'),
  ('/win32manifest:' + (Join-Path $suiteSourceRoot 'Installer\app.manifest')),
  ('/resource:' + (Join-Path $suiteReleaseRoot ("HungryGhostSuite-" + $suiteVersion + "-Windows-VST3.zip")) + ',payload'),
  ('/resource:' + $suiteHashesPath + ',hashes'),
  ('/resource:' + $suiteVersionPath + ',version'),
  ('/out:' + (Join-Path $suiteReleaseRoot ("HungryGhostSuite-" + $suiteVersion + "-Setup.exe"))),
  (Join-Path $suiteSourceRoot 'Installer\Setup.cs')
)
& $suiteCompilerPath @suiteCompileArguments
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
Write-Output 'Windows installer compiled from the verified release package.'
