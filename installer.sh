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
            -na | --no-alias)
                INSTALL_ALIAS=false
                shift 1
                ;;
            -nc | --no-color)
                USE_COLORS=false
                shift 1
                ;;
            -nC | --no-cloning)
                DO_CLONE=false
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

    [ -z "$DO_CLONE" ] && DO_CLONE=true
    [ -z "$INSTALL_ALIAS" ] && INSTALL_ALIAS=true
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
    echo "$@"
}

congrats() {
    if [ $USE_COLORS ]; then tput setaf 2; fi
    echo "$1"
    if [ $USE_COLORS ]; then tput sgr0; fi
}

try() {
    if ! "$@"; then error "$@"; exit 1; fi
}

maybe_clone() {
    if $DO_CLONE
    then
        info "Cloning $BREAK_URL into $BREAK_INSTALL_DIR"
        try git clone $BREAK_URL $BREAK_INSTALL_DIR
    fi
}

maybe_install_alias() {
    if $INSTALL_ALIAS
    then
        case $(basename $SHELL) in
            "zsh")
                SHRC="$HOME/.zshrc"
                ;;
            "bash")
                SHRC="$HOME/.bashrc"
                ;;
            *)
                warning "Unknown shell $SHELL, skipping alias installation..."
                return
                ;;
        esac
        try echo alias break="$BREAK_INSTALL_DIR/break" >> $SHRC
        try $SHELL -c "source $SHRC"
    fi
}

parse_arguments $*
maybe_clone
maybe_install_alias

congrats "Break is successfully installed and is ready to work!"
