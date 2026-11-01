$ErrorActionPreference = "Stop"

# ============================================================
# CLI-code-obfuscator Windows Installer
# Repository: TrushankVaghasiya/CLI-code-obfuscator
# ============================================================

$Owner = "TrushankVaghasiya"
$Repo = "CLI-code-obfuscator"
$AssetName = "obfuscate-windows-x64.zip"

$InstallDir = Join-Path $env:LOCALAPPDATA "Programs\obfuscate"
$ExecutablePath = Join-Path $InstallDir "obfuscate.exe"
$ApiUrl = "https://api.github.com/repos/$Owner/$Repo/releases/latest"

$TempDir = Join-Path ([System.IO.Path]::GetTempPath()) `
    ("obfuscate-install-" + [guid]::NewGuid().ToString("N"))

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Failure {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

try {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "  CLI-code-obfuscator Installer"
    Write-Host "========================================"
    Write-Host ""

    # Check Windows
    if ($env:OS -ne "Windows_NT") {
        throw "This installer supports Windows only."
    }

    # Check 64-bit Windows
    if (-not [Environment]::Is64BitOperatingSystem) {
        throw "This release requires 64-bit Windows."
    }

    # Create temporary working directory
    New-Item -ItemType Directory -Path $TempDir -Force |
        Out-Null

    # Get latest published release
    Write-Info "Checking the latest GitHub release..."

    $Headers = @{
        "User-Agent" = "CLI-code-obfuscator-installer"
        "Accept"     = "application/vnd.github+json"
    }

    $Release = Invoke-RestMethod `
        -Uri $ApiUrl `
        -Headers $Headers

    if (-not $Release.assets) {
        throw "The latest release has no downloadable assets."
    }

    # Find the expected Windows package
    $Asset = $Release.assets |
        Where-Object { $_.name -eq $AssetName } |
        Select-Object -First 1

    if (-not $Asset) {
        $AvailableAssets = ($Release.assets |
            ForEach-Object { $_.name }) -join ", "

        throw "Release '$($Release.tag_name)' does not contain '$AssetName'. Available assets: $AvailableAssets. Publish the expected Windows ZIP asset first."
    }

    Write-Info "Release: $($Release.tag_name)"
    Write-Info "Downloading $AssetName..."

    $ZipPath = Join-Path $TempDir $AssetName
    $ExtractDir = Join-Path $TempDir "extracted"

    Invoke-WebRequest `
        -Uri $Asset.browser_download_url `
        -OutFile $ZipPath `
        -Headers @{ "User-Agent" = "CLI-code-obfuscator-installer" }

    if (-not (Test-Path $ZipPath)) {
        throw "The release package was not downloaded."
    }

    # Extract release package
    Write-Info "Extracting the release package..."

    Expand-Archive `
        -Path $ZipPath `
        -DestinationPath $ExtractDir `
        -Force

    # Locate the executable in the package
    $Executable = Get-ChildItem `
        -Path $ExtractDir `
        -Filter "obfuscate.exe" `
        -File `
        -Recurse |
        Select-Object -First 1

    if (-not $Executable) {
        throw "The release ZIP does not contain obfuscate.exe."
    }

    # Install executable
    Write-Info "Installing to $InstallDir..."

    New-Item -ItemType Directory -Path $InstallDir -Force |
        Out-Null

    Copy-Item `
        -Path $Executable.FullName `
        -Destination $ExecutablePath `
        -Force

    # Add install directory to the current user's persistent PATH
    $UserPath = [Environment]::GetEnvironmentVariable(
        "Path",
        [EnvironmentVariableTarget]::User
    )

    $PathEntries = @()

    if (-not [string]::IsNullOrWhiteSpace($UserPath)) {
        $PathEntries = @(
            $UserPath -split ";" |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
    }

    $AlreadyInPath = $PathEntries |
        Where-Object {
            [string]::Equals(
                $_.TrimEnd("\"),
                $InstallDir.TrimEnd("\"),
                [StringComparison]::OrdinalIgnoreCase
            )
        }

    if (-not $AlreadyInPath) {
        $NewUserPath = (@($PathEntries) + $InstallDir) -join ";"

        [Environment]::SetEnvironmentVariable(
            "Path",
            $NewUserPath,
            [EnvironmentVariableTarget]::User
        )

        Write-Info "Added the installation directory to your user PATH."
    }
    else {
        Write-Info "The installation directory is already in your user PATH."
    }

    # Update PATH in this installer process
    $CurrentEntries = @($env:Path -split ";")

    if (-not ($CurrentEntries | Where-Object {
        [string]::Equals(
            $_.TrimEnd("\"),
            $InstallDir.TrimEnd("\"),
            [StringComparison]::OrdinalIgnoreCase
        )
    })) {
        $env:Path = "$InstallDir;$env:Path"
    }

    # Verify executable exists
    if (-not (Test-Path $ExecutablePath)) {
        throw "Installation failed: obfuscate.exe was not found."
    }

    Write-Success "Installation completed!"
    Write-Host ""
    Write-Host "Installed executable: $ExecutablePath"
    Write-Host ""
    Write-Host "Try the following command:"
    Write-Host "  obfuscate --help"
    Write-Host ""
    Write-Host "Example:"
    Write-Host "  obfuscate -i input.cpp -o output.cpp --level 2"
    Write-Host ""
    Write-Host "Open a NEW PowerShell window to use the command."
    Write-Host "========================================"
}
catch {
    Write-Failure $_.Exception.Message
    exit 1
}
finally {
    if (Test-Path $TempDir) {
        Remove-Item -Path $TempDir -Recurse -Force `
            -ErrorAction SilentlyContinue
    }
}