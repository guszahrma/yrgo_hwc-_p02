# Runs clang-format at the version pinned in version.txt, installing it into .venv if needed.
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$version = (Get-Content (Join-Path $PSScriptRoot 'version.txt') -Raw).Trim()
$venv = Join-Path $root '.venv'
$bin = Join-Path $venv 'Scripts\clang-format.exe'
$python = Join-Path $venv 'Scripts\python.exe'

if (-not (Test-Path $bin) -or -not ((& $bin --version) -match "version $([regex]::Escape($version))([^0-9.]|$)")) {
    Write-Host "Installing clang-format $version into $venv"
    if (-not (Test-Path $python)) {
        & python -m venv $venv
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
    & $python -m pip install --quiet --disable-pip-version-check "clang-format==$version"
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

& $bin @args
exit $LASTEXITCODE
