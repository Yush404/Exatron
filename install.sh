#!/usr/bin/env bash
# =============================================================================
# Exatron Compiler Installer for Linux
# https://github.com/exatron-lang/exatron
# =============================================================================

set -euo pipefail

# -----------------------------------------------------------------------------
# OS Check: Linux Only
# -----------------------------------------------------------------------------
if [[ "$(uname -s)" != "Linux" ]]; then
    echo "Linux only for now" >&2
    exit 1
fi

# -----------------------------------------------------------------------------
# Configuration and Defaults
# -----------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR"

EXATRON_REPO_URL=""
EXATRON_BRANCH=""

PREFIX=""
SYSTEM_INSTALL=false
YES=false
NO_DEPS=false
RUN_TESTS=false
UNINSTALL=false
DRY_RUN=false

BUILD_DIR=""
REPO_TMP_DIR=""
SMOKE_TMP_DIR=""
DETECTED_CXX=""

# -----------------------------------------------------------------------------
# Terminal Colors & Formatting
# -----------------------------------------------------------------------------
if [[ -t 1 && "${TERM:-dumb}" != "dumb" ]]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    CYAN='\033[0;36m'
    BOLD='\033[1m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    CYAN=''
    BOLD=''
    NC=''
fi

info() {
    echo -e "${CYAN}$*${NC}"
}

step() {
    echo -e "\n${BOLD}${BLUE}$*${NC}"
}

success() {
    echo -e "${GREEN}${BOLD}$*${NC}"
}

warn() {
    echo -e "${YELLOW}Warning:${NC} $*" >&2
}

error() {
    echo -e "${RED}Error:${NC} $*" >&2
}

# -----------------------------------------------------------------------------
# Cleanup Trap
# -----------------------------------------------------------------------------
cleanup() {
    local exit_code=$?
    if [[ -n "${BUILD_DIR:-}" && -d "${BUILD_DIR:-}" ]]; then
        rm -rf "$BUILD_DIR"
    fi
    if [[ -n "${REPO_TMP_DIR:-}" && -d "${REPO_TMP_DIR:-}" ]]; then
        rm -rf "$REPO_TMP_DIR"
    fi
    if [[ -n "${SMOKE_TMP_DIR:-}" && -d "${SMOKE_TMP_DIR:-}" ]]; then
        rm -rf "$SMOKE_TMP_DIR"
    fi
    exit "$exit_code"
}
trap cleanup EXIT INT TERM

# -----------------------------------------------------------------------------
# Help / Usage
# -----------------------------------------------------------------------------
show_help() {
    cat << EOF
Exatron Compiler Installer for Linux

Usage:
  ./install.sh [OPTIONS]

Options:
  --system           Install system-wide to /usr/local (requires sudo for install step)
  --prefix DIR       Install to custom prefix DIR (default: ~/.local)
  -y, --yes          Automatic yes to prompts; run non-interactively
  --no-deps          Skip package manager dependency installation
  --run-tests        Run the full test suite after building and installing
  --uninstall        Uninstall Exatron from the specified prefix
  --dry-run          Print every command that would be executed without running it
  --repo URL         Clone and build from git repository URL
  --branch NAME      Branch or tag to clone when using --repo
  -h, --help         Show this help message and exit

Environment Variables:
  EXATRON_CXX        Override C++ compiler (e.g. g++, g++-14)
  EXATRON_FORCE_DISTRO Force simulated distro in --dry-run (ubuntu, fedora, arch)
EOF
}

# -----------------------------------------------------------------------------
# Command Execution Helpers
# -----------------------------------------------------------------------------
run_cmd() {
    if [[ "$DRY_RUN" == true ]]; then
        echo "[dry-run] $*"
    else
        "$@"
    fi
}

run_cmd_root() {
    if [[ "${IS_ROOT:-false}" == true ]]; then
        run_cmd "$@"
    else
        if [[ "$DRY_RUN" == false ]] && ! command -v sudo >/dev/null 2>&1; then
            error "Root privileges are required, but 'sudo' is not installed."
            echo "Please run this command as root: $*" >&2
            exit 1
        fi
        run_cmd sudo "$@"
    fi
}

# -----------------------------------------------------------------------------
# Parse Command Line Arguments
# -----------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --system)
            SYSTEM_INSTALL=true
            shift
            ;;
        --prefix)
            if [[ $# -lt 2 || -z "$2" ]]; then
                error "--prefix requires a directory argument."
                exit 1
            fi
            PREFIX="$2"
            shift 2
            ;;
        -y|--yes)
            YES=true
            shift
            ;;
        --no-deps)
            NO_DEPS=true
            shift
            ;;
        --run-tests)
            RUN_TESTS=true
            shift
            ;;
        --uninstall)
            UNINSTALL=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --repo)
            if [[ $# -lt 2 || -z "$2" ]]; then
                error "--repo requires a URL argument."
                exit 1
            fi
            EXATRON_REPO_URL="$2"
            shift 2
            ;;
        --branch)
            if [[ $# -lt 2 || -z "$2" ]]; then
                error "--branch requires a branch name argument."
                exit 1
            fi
            EXATRON_BRANCH="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            show_help >&2
            exit 1
            ;;
    esac
done

# -----------------------------------------------------------------------------
# Resolve Prefix
# -----------------------------------------------------------------------------
if [[ "$SYSTEM_INSTALL" == true && -n "$PREFIX" && "$PREFIX" != "/usr/local" ]]; then
    error "Cannot specify both --system and a conflicting --prefix."
    exit 1
fi

if [[ "$SYSTEM_INSTALL" == true ]]; then
    PREFIX="/usr/local"
elif [[ -z "$PREFIX" ]]; then
    PREFIX="$HOME/.local"
fi

# Expand tilde and relative paths
PREFIX="${PREFIX/#\~/$HOME}"
if [[ "$PREFIX" != /* ]]; then
    PREFIX="$(pwd)/$PREFIX"
fi
PREFIX="${PREFIX%/}"
BIN_DIR="$PREFIX/bin"

# -----------------------------------------------------------------------------
# Check Root Privileges
# -----------------------------------------------------------------------------
IS_ROOT=false
if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
    IS_ROOT=true
fi

if [[ "$IS_ROOT" == true && "$SYSTEM_INSTALL" == false && "$UNINSTALL" == false ]]; then
    warn "Running as root without --system is not recommended."
    warn "Files will be installed to root's home directory ($PREFIX)."
    if [[ "$YES" == false && "$DRY_RUN" == false ]]; then
        read -r -p "Continue anyway? [y/N] " response
        case "$response" in
            [yY][eE][sS]|[yY]) ;;
            *) echo "Installation aborted."; exit 1 ;;
        esac
    fi
fi

# -----------------------------------------------------------------------------
# Detect Linux Distribution
# -----------------------------------------------------------------------------
OS_ID=""
OS_LIKE=""

if [[ -f /etc/os-release ]]; then
    OS_ID=$(grep -E '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"' | tr '[:upper:]' '[:lower:]' || true)
    OS_LIKE=$(grep -E '^ID_LIKE=' /etc/os-release | cut -d= -f2 | tr -d '"' | tr '[:upper:]' '[:lower:]' || true)
fi

if [[ -n "${EXATRON_FORCE_DISTRO:-}" ]]; then
    if [[ "$DRY_RUN" == true ]]; then
        case "$EXATRON_FORCE_DISTRO" in
            ubuntu|debian)
                OS_ID="ubuntu"
                OS_LIKE="debian"
                ;;
            fedora|rhel)
                OS_ID="fedora"
                OS_LIKE="rhel"
                ;;
            arch)
                OS_ID="arch"
                OS_LIKE=""
                ;;
            *)
                error "Unsupported EXATRON_FORCE_DISTRO='$EXATRON_FORCE_DISTRO'. Use ubuntu, fedora, or arch."
                exit 1
                ;;
        esac
    else
        warn "EXATRON_FORCE_DISTRO is ignored unless --dry-run is specified."
    fi
fi

DISTRO_FAMILY=""
PKG_MANAGER=""
case " $OS_ID $OS_LIKE " in
    *" debian "*|*" ubuntu "*|*" linuxmint "*|*" pop "*)
        DISTRO_FAMILY="debian"
        PKG_MANAGER="apt"
        ;;
    *" fedora "*|*" rhel "*|*" centos "*|*" rocky "*|*" almalinux "*)
        DISTRO_FAMILY="fedora"
        PKG_MANAGER="dnf"
        ;;
    *" arch "*|*" manjaro "*|*" endeavouros "*)
        DISTRO_FAMILY="arch"
        PKG_MANAGER="pacman"
        ;;
    *)
        DISTRO_FAMILY="unknown"
        ;;
esac

# -----------------------------------------------------------------------------
# Uninstall Mode
# -----------------------------------------------------------------------------
do_uninstall() {
    step "Uninstalling Exatron from $PREFIX..."
    local bin_target="$BIN_DIR/exatron"
    if [[ -f "$bin_target" ]]; then
        info "  Removing binary: $bin_target"
        if [[ "$DRY_RUN" == true ]]; then
            echo "[dry-run] rm -f '$bin_target'"
        else
            if [[ ! -w "$BIN_DIR" && "$IS_ROOT" == false ]]; then
                run_cmd_root rm -f "$bin_target"
            else
                rm -f "$bin_target"
            fi
        fi
    else
        info "  Binary not found at $bin_target (skipping)."
    fi

    # Clean rc files
    local rc_files=("$HOME/.profile" "$HOME/.bashrc" "$HOME/.zshrc")
    for rc in "${rc_files[@]}"; do
        if [[ -f "$rc" && -w "$rc" ]]; then
            if grep -Fq "# >>> exatron >>>" "$rc"; then
                info "  Removing Exatron PATH block from $rc"
                if [[ "$DRY_RUN" == true ]]; then
                    echo "[dry-run] sed -i '/# >>> exatron >>>/,/# <<< exatron <<</d' '$rc'"
                else
                    sed -i '/# >>> exatron >>>/,/# <<< exatron <<</d' "$rc"
                fi
            fi
        fi
    done

    # Clean fish config
    local fish_file="$HOME/.config/fish/conf.d/exatron.fish"
    if [[ -f "$fish_file" ]]; then
        info "  Removing fish configuration: $fish_file"
        if [[ "$DRY_RUN" == true ]]; then
            echo "[dry-run] rm -f '$fish_file'"
        else
            rm -f "$fish_file"
            rmdir "$HOME/.config/fish/conf.d" 2>/dev/null || true
            rmdir "$HOME/.config/fish" 2>/dev/null || true
        fi
    fi

    if [[ "$PREFIX" != "/usr" && "$PREFIX" != "/usr/local" && "$PREFIX" != "$HOME/.local" ]]; then
        if [[ "$DRY_RUN" == false ]]; then
            rmdir "$BIN_DIR" 2>/dev/null || true
            rmdir "$PREFIX" 2>/dev/null || true
        fi
    fi

    success "Exatron uninstalled successfully from $PREFIX."
}

if [[ "$UNINSTALL" == true ]]; then
    do_uninstall
    exit 0
fi

# -----------------------------------------------------------------------------
# Feature Detection Functions
# -----------------------------------------------------------------------------
test_compiler_cxx23() {
    local candidate="$1"
    if ! command -v "$candidate" >/dev/null 2>&1; then
        return 1
    fi
    local tmp_src tmp_out ok=1
    tmp_src=$(mktemp /tmp/exatron_cxx_XXXXXX.cpp 2>/dev/null || mktemp)
    tmp_out=$(mktemp /tmp/exatron_bin_XXXXXX 2>/dev/null || mktemp)
    cat << 'EOF' > "$tmp_src"
#include <vector>
#include <string>
int main() {
    auto x = []<typename T>(T v) { return v; }(42);
    return x == 42 ? 0 : 1;
}
EOF
    if "$candidate" -std=c++23 "$tmp_src" -o "$tmp_out" >/dev/null 2>&1; then
        if "$tmp_out" >/dev/null 2>&1; then
            ok=0
        fi
    fi
    rm -f "$tmp_src" "$tmp_out"
    return "$ok"
}

check_cmake_version() {
    if ! command -v cmake >/dev/null 2>&1; then
        return 1
    fi
    local ver ver_major ver_minor
    ver=$(cmake --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
    ver_major=$(echo "$ver" | cut -d. -f1)
    ver_minor=$(echo "$ver" | cut -d. -f2)
    if [[ -z "$ver_major" || -z "$ver_minor" ]]; then
        return 1
    fi
    if [[ "$ver_major" -gt 3 ]] || { [[ "$ver_major" -eq 3 ]] && [[ "$ver_minor" -ge 20 ]]; }; then
        return 0
    fi
    return 1
}

find_cxx23_compiler() {
    if [[ -n "${EXATRON_CXX:-}" ]]; then
        if test_compiler_cxx23 "$EXATRON_CXX"; then
            echo "$EXATRON_CXX"
            return 0
        fi
    fi
    for cand in g++ g++-16 g++-15 g++-14 g++-13; do
        if test_compiler_cxx23 "$cand"; then
            echo "$cand"
            return 0
        fi
    done
    return 1
}

print_cxx23_failure_advice() {
    error "Could not find a C++23-capable C++ compiler."
    if command -v g++ >/dev/null 2>&1; then
        echo "Found default compiler: $(g++ --version | head -n1)" >&2
        echo "This compiler does not support -std=c++23 or required C++23 features." >&2
    else
        echo "No C++ compiler (g++) found on this system." >&2
    fi
    echo "" >&2
    echo "Distro-specific suggestions to install a C++23 compiler (GCC 13 or newer):" >&2
    case "$DISTRO_FAMILY" in
        debian)
            echo "  Ubuntu 24.04+ / Debian 13+: sudo apt update && sudo apt install -y g++" >&2
            echo "  Ubuntu 22.04:               sudo apt update && sudo apt install -y g++-13 (or g++-14)" >&2
            echo "                              (If not available, install via an official Ubuntu toolchain repo or upgrade OS)" >&2
            ;;
        fedora)
            echo "  Fedora / RHEL 9+:           sudo dnf install -y gcc-c++" >&2
            ;;
        arch)
            echo "  Arch Linux:                 sudo pacman -S gcc" >&2
            ;;
        *)
            echo "  Please install GCC 13, 14, 15, or 16 using your system package manager." >&2
            ;;
    esac
}

# -----------------------------------------------------------------------------
# [1/6] System Detection
# -----------------------------------------------------------------------------
step "[1/6] Detecting system and environment..."
info "  Operating System : Linux ($(uname -r))"
info "  Detected Distro  : ${OS_ID:-Unknown} (${DISTRO_FAMILY:-Unknown} family)"
info "  Install Prefix   : $PREFIX"
info "  Install Mode     : $(if [[ "$SYSTEM_INSTALL" == true ]]; then echo "System-wide (/usr/local)"; else echo "User ($PREFIX)"; fi)"

if [[ "$DISTRO_FAMILY" == "unknown" && "$NO_DEPS" == false ]]; then
    error "Unsupported or unrecognized Linux distribution ($OS_ID)."
    cat << 'EOF' >&2

Exatron requires the following build and runtime dependencies:
  - GCC with C++23 support (g++ >= 13)
  - CMake >= 3.20
  - Make
  - Git (if cloning via --repo)

Manual build and install steps:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local
  cmake --build build -j$(nproc)
  cmake --install build
EOF
    exit 1
fi

# -----------------------------------------------------------------------------
# [2/6] Dependency Check & Package Installation
# -----------------------------------------------------------------------------
step "[2/6] Checking dependencies and C++23 compiler..."

DETECTED_CXX=$(find_cxx23_compiler || true)
HAS_CMAKE=false
if check_cmake_version; then
    HAS_CMAKE=true
fi
HAS_MAKE=false
if command -v make >/dev/null 2>&1; then
    HAS_MAKE=true
fi
HAS_GIT=false
if command -v git >/dev/null 2>&1; then
    HAS_GIT=true
fi

if [[ "$NO_DEPS" == true ]]; then
    info "  Skipping package manager dependency check (--no-deps specified)."
elif [[ "$DRY_RUN" == true && -n "${EXATRON_FORCE_DISTRO:-}" ]]; then
    info "  Simulating package installation for $OS_ID ($DISTRO_FAMILY family)..."
    case "$DISTRO_FAMILY" in
        debian)
            run_cmd_root apt-get update
            run_cmd_root apt-get install -y build-essential cmake git
            ;;
        fedora)
            run_cmd_root dnf install -y gcc-c++ make cmake git
            ;;
        arch)
            run_cmd_root pacman -S --needed --noconfirm base-devel cmake git
            ;;
    esac
elif [[ -n "$DETECTED_CXX" && "$HAS_CMAKE" == true && "$HAS_MAKE" == true && ( "$HAS_GIT" == true || -z "$EXATRON_REPO_URL" ) ]]; then
    info "  All dependencies already satisfied:"
    info "    Compiler: $DETECTED_CXX (C++23 verified)"
    info "    CMake   : $(cmake --version | head -n1)"
    info "    Make    : $(command -v make)"
    if command -v git >/dev/null 2>&1; then
        info "    Git     : $(command -v git)"
    fi
else
    # One or more dependencies are missing or insufficient
    info "  Missing or insufficient dependencies detected."
    
    # Prompt user before installing
    if [[ "$YES" == false && "$DRY_RUN" == false ]]; then
        echo "The following package manager action will be taken to install required dependencies:"
        case "$DISTRO_FAMILY" in
            debian) echo "  apt-get update && apt-get install -y build-essential cmake git" ;;
            fedora) echo "  dnf install -y gcc-c++ make cmake git" ;;
            arch)   echo "  pacman -S --needed --noconfirm base-devel cmake git" ;;
        esac
        read -r -p "Proceed with package installation? [y/N] " resp
        case "$resp" in
            [yY][eE][sS]|[yY]) ;;
            *) echo "Installation aborted."; exit 1 ;;
        esac
    fi

    case "$DISTRO_FAMILY" in
        debian)
            run_cmd_root apt-get update
            run_cmd_root apt-get install -y build-essential cmake git
            # Check if default g++ is sufficient, else try candidate packages
            if [[ "$DRY_RUN" == false && -z "$(find_cxx23_compiler || true)" ]]; then
                for cand_pkg in g++-15 g++-14 g++-13; do
                    if apt-cache show "$cand_pkg" >/dev/null 2>&1; then
                        info "  Default g++ lacks C++23; installing $cand_pkg..."
                        run_cmd_root apt-get install -y "$cand_pkg"
                        break
                    fi
                done
            fi
            ;;
        fedora)
            run_cmd_root dnf install -y gcc-c++ make cmake git
            ;;
        arch)
            run_cmd_root pacman -S --needed --noconfirm base-devel cmake git
            ;;
    esac
fi

# Re-check compiler after potential package installation
if [[ -z "$DETECTED_CXX" ]]; then
    DETECTED_CXX=$(find_cxx23_compiler || true)
fi

if [[ -z "$DETECTED_CXX" ]]; then
    if [[ "$DRY_RUN" == true ]]; then
        DETECTED_CXX="${EXATRON_CXX:-g++}"
        info "  [dry-run] Using simulated compiler: $DETECTED_CXX"
    else
        print_cxx23_failure_advice
        exit 1
    fi
else
    info "  Using C++ backend compiler: $DETECTED_CXX"
fi

if [[ "$DRY_RUN" == false ]] && ! check_cmake_version; then
    error "CMake >= 3.20 is required."
    exit 1
fi

# -----------------------------------------------------------------------------
# [3/6] Prepare Source & CMake Configuration
# -----------------------------------------------------------------------------
step "[3/6] Preparing source and configuring build with CMake..."

if [[ -n "$EXATRON_REPO_URL" ]]; then
    info "  Cloning repository: $EXATRON_REPO_URL..."
    if ! command -v git >/dev/null 2>&1; then
        error "'git' is required to clone from --repo."
        exit 1
    fi
    REPO_TMP_DIR=$(mktemp -d /tmp/exatron_repo_XXXXXX)
    local_git_cmd=(git clone --depth 1)
    if [[ -n "$EXATRON_BRANCH" ]]; then
        local_git_cmd+=(--branch "$EXATRON_BRANCH")
    fi
    local_git_cmd+=("$EXATRON_REPO_URL" "$REPO_TMP_DIR")
    run_cmd "${local_git_cmd[@]}"
    SRC_DIR="$REPO_TMP_DIR"
fi

if [[ "$DRY_RUN" == false && ! -f "$SRC_DIR/CMakeLists.txt" ]]; then
    error "Cannot find CMakeLists.txt in source directory: $SRC_DIR"
    exit 1
fi

if [[ "$DRY_RUN" == true ]]; then
    BUILD_DIR="/tmp/exatron_build_dryrun"
else
    BUILD_DIR=$(mktemp -d /tmp/exatron_build_XXXXXX)
fi

info "  Configuring build in $BUILD_DIR..."
run_cmd cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DEXATRON_CXX="$DETECTED_CXX"

# -----------------------------------------------------------------------------
# [4/6] Build Exatron
# -----------------------------------------------------------------------------
step "[4/6] Compiling Exatron..."
NUM_JOBS=$(nproc 2>/dev/null || echo 2)
info "  Building with $NUM_JOBS parallel jobs..."
run_cmd cmake --build "$BUILD_DIR" -j"$NUM_JOBS"

# -----------------------------------------------------------------------------
# [5/6] Install Exatron Binary
# -----------------------------------------------------------------------------
step "[5/6] Installing Exatron to $BIN_DIR..."

INSTALL_SUDO=false
if [[ "$SYSTEM_INSTALL" == true && "$IS_ROOT" == false ]]; then
    INSTALL_SUDO=true
elif [[ -d "$PREFIX" && ! -w "$PREFIX" && "$IS_ROOT" == false ]]; then
    INSTALL_SUDO=true
fi

if [[ "$INSTALL_SUDO" == true ]]; then
    info "  Installing with root privileges..."
    run_cmd_root cmake --install "$BUILD_DIR"
else
    info "  Installing as current user..."
    run_cmd cmake --install "$BUILD_DIR"
fi

# -----------------------------------------------------------------------------
# [6/6] PATH Setup & Verification
# -----------------------------------------------------------------------------
step "[6/6] Configuring PATH and verifying installation..."

path_has_dir() {
    local target="$1"
    target="${target%/}"
    case ":$PATH:" in
        *":$target:"*) return 0 ;;
        *) return 1 ;;
    esac
}

update_rc_file() {
    local rc="$1"
    local bin_path="$2"

    if [[ "$DRY_RUN" == true ]]; then
        echo "[dry-run] Update PATH block in $rc (add '$bin_path')"
        return 0
    fi

    local dir
    dir="$(dirname "$rc")"
    if [[ ! -d "$dir" || ( ! -w "$dir" && ! -w "$rc" ) ]]; then
        warn "Cannot write to $rc (directory not writable); skipping."
        return 1
    fi

    if [[ ! -f "$rc" ]]; then
        if ! touch "$rc" 2>/dev/null; then
            warn "Could not create $rc; skipping."
            return 1
        fi
    fi

    # Strip any existing exatron block to ensure idempotency
    if ! sed -i '/# >>> exatron >>>/,/# <<< exatron <<</d' "$rc" 2>/dev/null; then
        warn "Could not modify $rc; skipping."
        return 1
    fi

    # Ensure trailing newline before appending
    if [[ -s "$rc" && -n "$(tail -c 1 "$rc")" ]]; then
        echo "" >> "$rc"
    fi

    cat << EOF >> "$rc"
# >>> exatron >>>
export PATH="$bin_path:\$PATH"
# <<< exatron <<<
EOF
    return 0
}

setup_path() {
    if [[ "$SYSTEM_INSTALL" == true ]]; then
        info "  System install: /usr/local/bin is typically already on system PATH."
        return 0
    fi

    if path_has_dir "$BIN_DIR"; then
        info "  $BIN_DIR is already in your PATH."
        return 0
    fi

    info "  Configuring shell startup files to include $BIN_DIR in PATH..."
    local rc_files=("$HOME/.profile" "$HOME/.bashrc")
    if [[ -f "$HOME/.zshrc" ]]; then
        rc_files+=("$HOME/.zshrc")
    fi

    for rc in "${rc_files[@]}"; do
        if update_rc_file "$rc" "$BIN_DIR"; then
            info "    Updated: $rc"
        fi
    done

    # Fish shell configuration
    if [[ "$DRY_RUN" == true ]]; then
        echo "[dry-run] mkdir -p '$HOME/.config/fish/conf.d'"
        echo "[dry-run] Write 'fish_add_path \"$BIN_DIR\"' to '$HOME/.config/fish/conf.d/exatron.fish'"
    else
        if mkdir -p "$HOME/.config/fish/conf.d" 2>/dev/null; then
            cat << EOF > "$HOME/.config/fish/conf.d/exatron.fish"
fish_add_path "$BIN_DIR"
EOF
            info "    Updated fish config: $HOME/.config/fish/conf.d/exatron.fish"
        else
            warn "Could not create $HOME/.config/fish/conf.d; skipping fish setup."
        fi
    fi
}

setup_path

# Verification
verify_installation() {
    local exatron_bin="$BIN_DIR/exatron"

    if [[ "$DRY_RUN" == true ]]; then
        echo "[dry-run] '$exatron_bin' --version"
        echo "[dry-run] Compile and run smoke test program with '$exatron_bin'"
        if [[ "$RUN_TESTS" == true ]]; then
            echo "[dry-run] (cd '$SRC_DIR' && bash tests/run_tests.sh)"
        fi
        return 0
    fi

    info "  Verifying installed binary..."
    if [[ ! -x "$exatron_bin" ]]; then
        error "Installed binary not found or not executable: $exatron_bin"
        exit 1
    fi

    local ver_out
    ver_out=$("$exatron_bin" --version 2>&1)
    info "    $ver_out"

    info "  Running smoke test..."
    SMOKE_TMP_DIR=$(mktemp -d /tmp/exatron_smoke_XXXXXX)
    cat << 'EOF' > "$SMOKE_TMP_DIR/hello.ex"
fn main() -> int {
    print("Hello from Exatron!");
    return 0;
}
EOF
    local smoke_out
    smoke_out=$("$exatron_bin" "$SMOKE_TMP_DIR/hello.ex" -o "$SMOKE_TMP_DIR/hello" --run 2>&1)
    if [[ "$smoke_out" != *"Hello from Exatron!"* ]]; then
        error "Smoke test failed. Output was:"
        echo "$smoke_out" >&2
        exit 1
    fi
    success "    Smoke test passed: hello.ex compiled and ran successfully."
    rm -rf "$SMOKE_TMP_DIR"
    SMOKE_TMP_DIR=""

    if [[ "$RUN_TESTS" == true ]]; then
        step "Running Exatron test suite (--run-tests)..."
        (cd "$SRC_DIR" && bash tests/run_tests.sh)
    fi
}

verify_installation

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
if [[ "$DRY_RUN" == true ]]; then
    step "Dry run completed successfully."
    info "No changes were made to your system."
    exit 0
fi

cat << EOF

========================================================================
             Exatron Compiler Installed Successfully!
========================================================================
  Binary location : $BIN_DIR/exatron
  Backend compiler: $DETECTED_CXX
  Install prefix  : $PREFIX

  NOTE: Changes to shell configuration files cannot affect your
  current terminal session.
  To use 'exatron' immediately in this session, run:
    export PATH="$BIN_DIR:\$PATH"
  Or open a new terminal.

  Verify anytime with:
    exatron --version
========================================================================
EOF
