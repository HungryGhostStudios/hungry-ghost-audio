param(
  [Parameter(Mandatory=$true)][string]$ReleaseDirectory,
  [Parameter(Mandatory=$true)][string]$HashManifest
)
$ErrorActionPreference = 'Stop'
$suiteSourceRoot = Split-Path -Parent $PSScriptRoot
$suiteReleaseRoot = (Resolve-Path -LiteralPath $ReleaseDirectory).Path
$suiteHashesPath = (Resolve-Path -LiteralPath $HashManifest).Path
$suiteCompilerPath = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$suiteCompileArguments = @(
  '/nologo', '/target:winexe', '/platform:x64', '/optimize+',
  '/r:System.Windows.Forms.dll', '/r:System.Drawing.dll',
  '/r:System.IO.Compression.dll', '/r:System.IO.Compression.FileSystem.dll',
  ('/win32manifest:' + (Join-Path $suiteSourceRoot 'Installer\app.manifest')),
  ('/resource:' + (Join-Path $suiteReleaseRoot 'HungryGhostSuite-0.1.0-Windows-VST3.zip') + ',payload'),
  ('/resource:' + $suiteHashesPath + ',hashes'),
  ('/out:' + (Join-Path $suiteReleaseRoot 'HungryGhostSuite-0.1.0-Setup.exe')),
  (Join-Path $suiteSourceRoot 'Installer\Setup.cs')
)
& $suiteCompilerPath @suiteCompileArguments
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
Write-Output 'Windows installer compiled from the verified release package.'
