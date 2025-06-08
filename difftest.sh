#!/usr/bin/env bash

# difftest.sh - Compare legacy and modern espresso implementations
# Usage: ./difftest.sh <pla_file> [options]

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default paths - will be detected automatically
MODERN_ESPRESSO=""
LEGACY_ESPRESSO=""
TEMP_DIR="/tmp/difftest_$$"

# Options
VERBOSE=false
KEEP_TEMP=false
TIMEOUT=30

usage() {
    echo "Usage: $0 <pla_file> [options]"
    echo ""
    echo "Options:"
    echo "  -v, --verbose     Show detailed output"
    echo "  -k, --keep-temp   Keep temporary files for inspection"
    echo "  -t, --timeout N   Set timeout in seconds (default: 30)"
    echo "  -m, --modern PATH Path to modern espresso executable"
    echo "  -l, --legacy PATH Path to legacy espresso executable"
    echo "  -h, --help        Show this help"
    echo ""
    echo "Example:"
    echo "  $0 examples/examples/b2"
    echo "  $0 examples/examples/t1 --verbose"
}

log() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $*" >&2
}

error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

find_executables() {
    # Try to find modern espresso
    local modern_paths=(
        "build/src/espresso"
    )
    
    for path in "${modern_paths[@]}"; do
        if [[ -x "$path" ]]; then
            MODERN_ESPRESSO="$(realpath "$path")"
            break
        fi
    done
    
    # Try to find legacy espresso
    local legacy_paths=(
        "build/legacy/espresso_legacy"
    )
    
    for path in "${legacy_paths[@]}"; do
        if [[ -x "$path" ]]; then
            LEGACY_ESPRESSO="$(realpath "$path")"
            break
        fi
    done
}

check_prerequisites() {
    if [[ -z "$MODERN_ESPRESSO" ]] || [[ ! -x "$MODERN_ESPRESSO" ]]; then
        error "Modern espresso executable not found or not executable"
        return 1
    fi
    
    if [[ -z "$LEGACY_ESPRESSO" ]] || [[ ! -x "$LEGACY_ESPRESSO" ]]; then
        error "Legacy espresso executable not found or not executable"
        return 1
    fi
    
    if ! command -v timeout >/dev/null 2>&1; then
        error "timeout command not found. Please install coreutils."
        return 1
    fi
    
    log "Found modern espresso: $MODERN_ESPRESSO"
    log "Found legacy espresso: $LEGACY_ESPRESSO"
    return 0
}

setup_temp_dir() {
    mkdir -p "$TEMP_DIR"
    
    if [[ "$KEEP_TEMP" == false ]]; then
        trap 'rm -rf "$TEMP_DIR"' EXIT
    else
        log "Temporary files will be kept in: $TEMP_DIR"
    fi
}

normalize_pla_output() {
    local input_file="$1"
    local output_file="$2"
    
    # Use pla_normalize to normalize PLA output for comparison
    # This provides more accurate normalization than awk
    local pla_normalize_path="build/src/pla_normalize"
    
    if [[ -x "$pla_normalize_path" ]]; then
        if cat "$input_file" | "$pla_normalize_path" > "$output_file" 2>/dev/null; then
            return 0
        else
            echo "pla_normalize failed to normalize $input_file"
            echo "File content to be normalized:"
            cat "$input_file"
            return 1
        fi
    else
        echo "pla_normalize not found at $pla_normalize_path"
        return 1
    fi
}

run_espresso() {
    local executable="$1"
    local input_file="$2"
    local output_file="$3"
    local error_file="$4"
    local name="$5"
    
    log "Running $name espresso..."
    
    local start_time
    start_time=$(date +%s.%N)
    
    # Check if this is the modern espresso (expects filename as argument)
    # or legacy espresso (reads from stdin)
    local cmd_success=false
    if [[ "$name" == "Modern" ]]; then
        # Modern espresso expects filename as argument
        if timeout "$TIMEOUT" "$executable" "$input_file" > "$output_file" 2> "$error_file"; then
            cmd_success=true
        fi
    else
        # Legacy espresso reads from stdin
        if timeout "$TIMEOUT" "$executable" < "$input_file" > "$output_file" 2> "$error_file"; then
            cmd_success=true
        fi
    fi
    
    if [[ "$cmd_success" == true ]]; then
        local end_time
        end_time=$(date +%s.%N)
        local duration
        duration=$(awk "BEGIN {printf \"%.3f\", $end_time - $start_time}")
        
        if [[ "$VERBOSE" == true ]]; then
            log "$name completed in ${duration}s"
        fi
        return 0
    else
        local exit_code=$?
        if [[ $exit_code == 124 ]]; then
            error "$name timed out after ${TIMEOUT}s"
        else
            error "$name failed with exit code $exit_code"
            if [[ -s "$error_file" ]]; then
                error "$name stderr:"
                cat "$error_file" >&2
            fi
        fi
        return $exit_code
    fi
}

compare_outputs() {
    local modern_file="$1"
    local legacy_file="$2"
    local modern_norm="$3"
    local legacy_norm="$4"
    
    # Normalize both outputs
    log "Normalizing outputs for comparison..."
    normalize_pla_output "$modern_file" "$modern_norm"
    normalize_pla_output "$legacy_file" "$legacy_norm"
    
    # Compare normalized outputs
    if diff -q "$modern_norm" "$legacy_norm" >/dev/null 2>&1; then
        success "Outputs are identical after normalization"
        return 0
    else
        warn "Outputs differ after normalization"
        
        if [[ "$VERBOSE" == true ]]; then
            echo
            echo "=== DETAILED DIFF ==="
            diff -u "$modern_norm" "$legacy_norm" || true
            echo "===================="
            echo
        fi
        
        # Show basic statistics
        local modern_cubes legacy_cubes modern_inputs legacy_inputs modern_outputs legacy_outputs
        
        modern_cubes=$(grep -v '^#' "$modern_norm" | grep -v '^$' | grep -v '^\.' | wc -l)
        legacy_cubes=$(grep -v '^#' "$legacy_norm" | grep -v '^$' | grep -v '^\.' | wc -l)
        
        modern_inputs=$(grep '^\.i ' "$modern_norm" | awk '{print $2}' || echo "0")
        legacy_inputs=$(grep '^\.i ' "$legacy_norm" | awk '{print $2}' || echo "0")
        
        modern_outputs=$(grep '^\.o ' "$modern_norm" | awk '{print $2}' || echo "0")
        legacy_outputs=$(grep '^\.o ' "$legacy_norm" | awk '{print $2}' || echo "0")
        
        echo "Modern:  inputs=$modern_inputs, outputs=$modern_outputs, cubes=$modern_cubes"
        echo "Legacy:  inputs=$legacy_inputs, outputs=$legacy_outputs, cubes=$legacy_cubes"
        
        return 1
    fi
}

main() {
    local pla_file=""
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -k|--keep-temp)
                KEEP_TEMP=true
                shift
                ;;
            -t|--timeout)
                TIMEOUT="$2"
                shift 2
                ;;
            -m|--modern)
                MODERN_ESPRESSO="$2"
                shift 2
                ;;
            -l|--legacy)
                LEGACY_ESPRESSO="$2"
                shift 2
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            -*)
                error "Unknown option: $1"
                usage
                exit 1
                ;;
            *)
                if [[ -z "$pla_file" ]]; then
                    pla_file="$1"
                else
                    error "Multiple PLA files specified"
                    usage
                    exit 1
                fi
                shift
                ;;
        esac
    done
    
    # Check if PLA file is provided
    if [[ -z "$pla_file" ]]; then
        error "No PLA file specified"
        usage
        exit 1
    fi
    
    # Check if PLA file exists
    if [[ ! -f "$pla_file" ]]; then
        error "PLA file not found: $pla_file"
        exit 1
    fi
    
    # Find executables if not specified
    if [[ -z "$MODERN_ESPRESSO" ]] || [[ -z "$LEGACY_ESPRESSO" ]]; then
        find_executables
    fi
    
    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi
    
    # Setup temporary directory
    setup_temp_dir
    
    log "Testing PLA file: $pla_file"

    # Define temporary files
    local modern_output="$TEMP_DIR/modern_output.pla"
    local legacy_output="$TEMP_DIR/legacy_output.pla"
    local modern_error="$TEMP_DIR/modern_error.log"
    local legacy_error="$TEMP_DIR/legacy_error.log"
    local modern_norm="$TEMP_DIR/modern_normalized.pla"
    local legacy_norm="$TEMP_DIR/legacy_normalized.pla"
    
    # Run both implementations
    local modern_success=true
    local legacy_success=true
    
    if ! run_espresso "$MODERN_ESPRESSO" "$pla_file" "$modern_output" "$modern_error" "Modern"; then
        modern_success=false
    fi
    
    if ! run_espresso "$LEGACY_ESPRESSO" "$pla_file" "$legacy_output" "$legacy_error" "Legacy"; then
        legacy_success=false
    fi
    
    # Check if both succeeded
    if [[ "$modern_success" == false ]] && [[ "$legacy_success" == false ]]; then
        error "Both implementations failed"
        exit 1
    elif [[ "$modern_success" == false ]]; then
        error "Modern implementation failed, but legacy succeeded"
        exit 1
    elif [[ "$legacy_success" == false ]]; then
        error "Legacy implementation failed, but modern succeeded"
        exit 1
    fi
    
    # Compare outputs
    if compare_outputs "$modern_output" "$legacy_output" "$modern_norm" "$legacy_norm"; then
        success "Test passed: Both implementations produce identical results"
        exit 0
    else
        error "Test failed: Implementations produce different results"
        
        echo
        echo "=== MODERN OUTPUT ==="
        cat "$modern_output"
        echo "===================="
        echo
        echo "=== LEGACY OUTPUT ==="
        cat "$legacy_output"
        echo "===================="
        echo
        
        if [[ "$KEEP_TEMP" == true ]]; then
            log "Temporary files preserved in: $TEMP_DIR"
            log "  Modern output: $modern_output"
            log "  Legacy output: $legacy_output"
            log "  Modern normalized: $modern_norm"
            log "  Legacy normalized: $legacy_norm"
        fi
        
        exit 1
    fi
}

# Run main function with all arguments
main "$@"
