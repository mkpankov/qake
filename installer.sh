set -e

usage() {
    echo "Usage: $0 [OPTION]"
    echo "  -h, --help    Show help."
}

parse_arguments() {
    while [ $# -gt 0 ]
    do
        case $1 in
            -d | --directory)
                BREAK_INSTALL_DIR=$2
                shift 2
                ;;
            -h | --help)
                usage
                exit 0
                ;;
            -nc | --no-color)
                USE_COLORS=false
                shift 1
                ;;
            -s | --source)
                BREAK_URL=$2
                shift 2
                ;;
            *)
                echo "Unknown option: $1"
                shift 1
                ;;
        esac
    done

    [ -z "$BREAK_URL" ] && BREAK_URL="git@github.com:constantius9/break.git"
    [ -z "$BREAK_INSTALL_DIR" ] && BREAK_INSTALL_DIR="$HOME/break/"
    [ -z "$USE_COLORS" ] && USE_COLORS=true
}

info() {
    if [ $USE_COLORS ]; then tput setaf 4; fi
    echo -n "[INFO] "
    if [ $USE_COLORS ]; then tput sgr0; fi
    echo "$1"
}

error() {
    if [ $USE_COLORS ]; then tput setaf 1; fi
    echo -n '[ERROR] '
    if [ $USE_COLORS ]; then tput sgr0; fi
    echo "Failed command: "
    echo "$1"
}

congrats() {
    if [ $USE_COLORS ]; then tput setaf 2; fi
    echo "$1"
    if [ $USE_COLORS ]; then tput sgr0; fi
}

try() {
    if ! $1; then error "$1"; fi
}

parse_arguments $*

info "Cloning $BREAK_URL into $BREAK_INSTALL_DIR"
try "git clone $BREAK_URL $BREAK_INSTALL_DIR"

congrats "Break is successfully installed and is ready to work!"
