$files = @(git ls-files -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx')

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($files.Count -eq 0) {
    Write-Error 'No tracked C/C++ files found.'
    exit 1
}

& clang-format --dry-run --Werror --style=file -- $files
exit $LASTEXITCODE