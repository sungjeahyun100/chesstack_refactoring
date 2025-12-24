<#
build_windows.ps1

PowerShell helper to configure and build the project on Windows (Visual Studio)
- Installs pybind11 into the selected Python (venv) if missing
- Runs CMake configure + build for the `chess_ext` pybind11 module
- Copies the built .pyd to the Python site-packages (venv) for immediate import

Usage (PowerShell):
.\build_windows.ps1 -PythonExe ".\.venv\Scripts\python.exe" -Generator "Visual Studio 17 2022" -Arch x64

#>
param(
    [string]$PythonExe = "python",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Arch = "x64",
    [string]$Config = "Release"
)

set -e

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "")
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "Project root: $ProjectRoot"
Write-Host "Build dir: $BuildDir"
Write-Host "Python executable: $PythonExe"
Write-Host "CMake generator: $Generator, Arch: $Arch, Config: $Config"

# Ensure pip and pybind11 are installed
Write-Host "Ensuring pip and pybind11 are installed in target Python..."
& $PythonExe -m pip install --upgrade pip setuptools wheel pybind11
if ($LASTEXITCODE -ne 0) { throw "pip install failed" }

# get pybind11 cmake dir
$pybindCMakeDir = & $PythonExe -m pybind11 --cmakedir 2>$null
if (-not $pybindCMakeDir) {
    throw "Unable to determine pybind11 cmake dir. Ensure pybind11 is installed for $PythonExe"
}
Write-Host "pybind11 cmake dir: $pybindCMakeDir"

# Configure CMake
if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
Push-Location $BuildDir
Write-Host "Configuring project with CMake..."
cmake .. -G "$Generator" -A $Arch -DBUILD_PYBINDING=ON -Dpybind11_DIR="$pybindCMakeDir" -DPYTHON_EXECUTABLE="$PythonExe"
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "CMake configure failed" }

# Build
Write-Host "Building target chess_ext..."
cmake --build . --config $Config --target chess_ext
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "Build failed" }

# Find built extension (.pyd)
Write-Host "Locating built python extension..."
$pattern = "chess_ext*.pyd"
$mods = Get-ChildItem -Path . -Recurse -Filter $pattern -File -ErrorAction SilentlyContinue
if (-not $mods -or $mods.Count -eq 0) {
    # try .so (WSL or mingw builds)
    $mods = Get-ChildItem -Path . -Recurse -Filter "chess_ext*.so" -File -ErrorAction SilentlyContinue
}
if (-not $mods -or $mods.Count -eq 0) {
    Pop-Location
    throw "Built extension not found in build tree"
}
$modfile = $mods[0].FullName
Write-Host "Found module: $modfile"

# Determine site-packages path for target Python
$siteDir = & $PythonExe -c "import sysconfig, json; print(sysconfig.get_paths()['purelib'])"
if (-not $siteDir) { Pop-Location; throw "Unable to get site-packages path for $PythonExe" }
Write-Host "Target site-packages: $siteDir"

# Copy module into site-packages
Copy-Item -Path $modfile -Destination $siteDir -Force
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "Failed to copy module to site-packages" }

Write-Host "Module installed to $siteDir"
Pop-Location
Write-Host "Done. You can now run:"
Write-Host "  & $PythonExe -c \"import chess_ext; print(chess_ext)\""

exit 0
