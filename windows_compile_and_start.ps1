# MultiAgent Windows: compile and start in one script
#
# Usage: .\windows_compile_and_start.ps1 [compile options] [-- editor args]
#
# Compile options:
#   -c, -Config <config>   Build config: Debug, Development, Shipping (default: Development)
#   -r, -Rebuild           Clean Intermediate/Binaries before build
#   -g, -Game              Build game target only (MultiAgent)
#   -h, -Help              Show this help
#
# Examples:
#   .\windows_compile_and_start.ps1
#   .\windows_compile_and_start.ps1 -c Debug
#   .\windows_compile_and_start.ps1 -r -- -ResX=1920 -ResY=1080

$ErrorActionPreference = 'Stop'

# Paths (edit UE5_ROOT for your machine)
$UE5_ROOT = "C:\Program Files\Epic Games\UE_5.7"
$PROJECT_ROOT = $PSScriptRoot
$PROJECT_FILE = Join-Path $PROJECT_ROOT "unreal_project\MultiAgent.uproject"
$CONFIG_PATH = Join-Path $PROJECT_ROOT "config\simulation.json"

# Windows specific paths
$BUILD_SCRIPT = Join-Path $UE5_ROOT "Engine\Build\BatchFiles\Build.bat"
$EDITOR_BIN = Join-Path $UE5_ROOT "Engine\Binaries\Win64\UnrealEditor.exe"

# Defaults
$TARGET = "MultiAgentEditor"
$PLATFORM = "Win64"
$CONFIG = "Development"
$REBUILD = $false

function Show-Help {
    Write-Host "Usage: .\windows_compile_and_start.ps1 [compile options] [-- editor args]"
    Write-Host ""
    Write-Host "Compile options:"
    Write-Host "  -c, -Config <config>   Build config: Debug, Development, Shipping (default: Development)"
    Write-Host "  -r, -Rebuild           Clean Intermediate/Binaries before build"
    Write-Host "  -g, -Game              Build game target only (MultiAgent)"
    Write-Host "  -h, -Help              Show this help"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\windows_compile_and_start.ps1"
    Write-Host "  .\windows_compile_and_start.ps1 -c Debug"
    Write-Host "  .\windows_compile_and_start.ps1 -r -- -ResX=1920 -ResY=1080"
}

# Parse args; everything after -- goes to UnrealEditor
$EDITOR_ARGS = @()
for ($i = 0; $i -lt $args.Count; $i++) {
    switch ($args[$i]) {
        { $_ -in '-c', '-Config', '--config' } {
            $CONFIG = $args[++$i]
        }
        { $_ -in '-r', '-Rebuild', '--rebuild' } {
            $REBUILD = $true
        }
        { $_ -in '-g', '-Game', '--game' } {
            $TARGET = "MultiAgent"
        }
        { $_ -in '-h', '-Help', '--help' } {
            Show-Help
            exit 0
        }
        '--' {
            $EDITOR_ARGS = $args[($i + 1)..($args.Count - 1)]
            $i = $args.Count
        }
        default {
            Write-Host "Unknown option: $($args[$i])" -ForegroundColor Red
            Show-Help
            exit 1
        }
    }
}

if ($CONFIG -notmatch '^(Debug|Development|Shipping)$') {
    Write-Host "Error: invalid config '$CONFIG'" -ForegroundColor Red
    Write-Host "Valid configs: Debug, Development, Shipping"
    exit 1
}

if (-not (Test-Path $BUILD_SCRIPT)) {
    Write-Host "Error: UE5 build script not found: $BUILD_SCRIPT" -ForegroundColor Red
    Write-Host "Please check if UE5_ROOT is configured correctly: $UE5_ROOT" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path $PROJECT_FILE)) {
    Write-Host "Error: project file not found: $PROJECT_FILE" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $EDITOR_BIN)) {
    Write-Host "Error: UnrealEditor binary not found: $EDITOR_BIN" -ForegroundColor Red
    Write-Host "Please check if UE5_ROOT is configured correctly: $UE5_ROOT" -ForegroundColor Yellow
    exit 1
}

if ($REBUILD) {
    Write-Host "Cleaning Intermediate/Binaries..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue (Join-Path $PROJECT_ROOT "unreal_project\Intermediate")
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue (Join-Path $PROJECT_ROOT "unreal_project\Binaries")
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "  MultiAgent Windows Build" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Target:   $TARGET" -ForegroundColor Yellow
Write-Host "Platform: $PLATFORM" -ForegroundColor Yellow
Write-Host "Config:   $CONFIG" -ForegroundColor Yellow
Write-Host ""

$START_TIME = Get-Date
& $BUILD_SCRIPT $TARGET $PLATFORM $CONFIG "-Project=$PROJECT_FILE" -WaitMutex
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed (exit code $LASTEXITCODE)" -ForegroundColor Red
    exit $LASTEXITCODE
}

$DURATION = [int]((Get-Date) - $START_TIME).TotalSeconds

Write-Host ""
Write-Host "Build succeeded! (took ${DURATION}s)" -ForegroundColor Green

$DEFAULT_MAP = ""
if (Test-Path $CONFIG_PATH) {
    try {
        $DEFAULT_MAP = (Get-Content -Raw $CONFIG_PATH | ConvertFrom-Json).DefaultMap
    } catch {
        $DEFAULT_MAP = ""
    }
}

$LAUNCH_ARGS = @()
if ($DEFAULT_MAP) {
    Write-Host "Starting with map: $DEFAULT_MAP"
    $LAUNCH_ARGS = @($DEFAULT_MAP)
}

Write-Host "Launching UnrealEditor..." -ForegroundColor Green
& $EDITOR_BIN $PROJECT_FILE @LAUNCH_ARGS @EDITOR_ARGS
