# Configure and build Snake with CMake + Ninja.
#
# Usage: Scripts\build.ps1 [-Config Debug|Release] [-Tests] [-Clean]
param(
    [ValidateSet('Debug', 'Release')] [string]$Config = 'Debug',
    [switch]$Tests,
    [switch]$Clean
)

$preset = $Config.ToLower()
if ($Tests) { $preset += '-tests' }

Push-Location "$PSScriptRoot/.."
if ($Clean) { Remove-Item -Recurse -Force "build/$preset" -ErrorAction Ignore }

cmake --preset $preset
if ($?) { cmake --build --preset $preset }
if ($? -and $Tests) { ctest --preset $preset }

Pop-Location
exit $LASTEXITCODE
