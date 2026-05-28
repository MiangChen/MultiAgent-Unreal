#!/bin/bash
# MultiAgent macOS: compile and start in one script

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Paths (edit UE5_ROOT for your machine)
UE5_ROOT="/Users/Shared/Epic Games/UE_5.7"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
PROJECT_FILE="${PROJECT_ROOT}/unreal_project/MultiAgent.uproject"
CONFIG_PATH="${PROJECT_ROOT}/config/simulation.json"
BUILD_SCRIPT="${UE5_ROOT}/Engine/Build/BatchFiles/Mac/Build.sh"
EDITOR_BIN="${UE5_ROOT}/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"

# Defaults
TARGET="MultiAgentEditor"
PLATFORM="Mac"
CONFIG="Development"
REBUILD=false

show_help() {
    echo "Usage: ./example_mac_compile_and_start.sh [compile options] [-- editor args]"
    echo ""
    echo "Compile options:"
    echo "  -c, --config <config>   Build config: Debug, Development, Shipping (default: Development)"
    echo "  -r, --rebuild           Clean Intermediate/Binaries before build"
    echo "  -g, --game              Build game target only (MultiAgent)"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  ./example_mac_compile_and_start.sh"
    echo "  ./example_mac_compile_and_start.sh -c Debug"
    echo "  ./example_mac_compile_and_start.sh -r -- -ResX=1920 -ResY=1080"
}

# Parse args; everything after -- goes to UnrealEditor
EDITOR_ARGS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--config)
            CONFIG="$2"
            shift 2
            ;;
        -r|--rebuild)
            REBUILD=true
            shift
            ;;
        -g|--game)
            TARGET="MultiAgent"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        --)
            shift
            EDITOR_ARGS=("$@")
            break
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

if [[ ! "$CONFIG" =~ ^(Debug|Development|Shipping)$ ]]; then
    echo -e "${RED}Error: invalid config '$CONFIG'${NC}"
    echo "Valid configs: Debug, Development, Shipping"
    exit 1
fi

if [ ! -f "$BUILD_SCRIPT" ]; then
    echo -e "${RED}Error: UE5 build script not found: $BUILD_SCRIPT${NC}"
    exit 1
fi

if [ ! -f "$PROJECT_FILE" ]; then
    echo -e "${RED}Error: project file not found: $PROJECT_FILE${NC}"
    exit 1
fi

if [ ! -x "$EDITOR_BIN" ]; then
    echo -e "${RED}Error: UnrealEditor binary not found: $EDITOR_BIN${NC}"
    exit 1
fi

if [ "$REBUILD" = true ]; then
    echo -e "${YELLOW}Cleaning Intermediate/Binaries...${NC}"
    rm -rf "${PROJECT_ROOT}/unreal_project/Intermediate"
    rm -rf "${PROJECT_ROOT}/unreal_project/Binaries"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  MultiAgent macOS Build${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "Target:   ${YELLOW}${TARGET}${NC}"
echo -e "Platform: ${YELLOW}${PLATFORM}${NC}"
echo -e "Config:   ${YELLOW}${CONFIG}${NC}"
echo ""

START_TIME=$(date +%s)
"$BUILD_SCRIPT" "$TARGET" "$PLATFORM" "$CONFIG" \
    -Project="$PROJECT_FILE" \
    -WaitMutex

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo ""
echo -e "${GREEN}Build succeeded! (took ${DURATION}s)${NC}"

DEFAULT_MAP=""
if [ -f "$CONFIG_PATH" ]; then
    if command -v jq >/dev/null 2>&1; then
        DEFAULT_MAP=$(jq -r '.DefaultMap // empty' "$CONFIG_PATH")
    else
        DEFAULT_MAP=$(grep -o '"DefaultMap"[[:space:]]*:[[:space:]]*"[^"]*"' "$CONFIG_PATH" | sed 's/.*: *"\([^"]*\)"/\1/')
    fi
fi

LAUNCH_ARGS=()
if [ -n "$DEFAULT_MAP" ]; then
    echo "Starting with map: $DEFAULT_MAP"
    LAUNCH_ARGS=("$DEFAULT_MAP")
fi

echo -e "${GREEN}Launching UnrealEditor...${NC}"
"$EDITOR_BIN" "$PROJECT_FILE" "${LAUNCH_ARGS[@]}" "${EDITOR_ARGS[@]}"
