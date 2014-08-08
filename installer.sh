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
            -ne | --no-env)
                INSTALL_ENV=false
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
            -nm | --no-make)
                INSTALL_MAKE=false
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
    [ -z "$INSTALL_ENV" ] && INSTALL_ENV=true
    [ -z "$BREAK_URL" ] && BREAK_URL="git@github.com:constantius9/break.git"
    [ -z "$BREAK_INSTALL_DIR" ] && BREAK_INSTALL_DIR="$HOME/break/"
    [ -z "$USE_COLORS" ] && USE_COLORS=true
    [ -z "$INSTALL_MAKE" ] && INSTALL_MAKE=true

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

maybe_install_make() {
    if $INSTALL_MAKE
    then
        readonly MAKE_FILENAME=make-4.0.tar.bz2
        readonly BREAK_MAKE_INSTALL_DIR="$BREAK_INSTALL_DIR/make-4.0/install"
        readonly MAKE="$BREAK_MAKE_INSTALL_DIR"/bin/make
        info "Downloading Make 4.0 to $BREAK_INSTALL_DIR/$MAKE_FILENAME"
        wget http://ftp.gnu.org/gnu/make/$MAKE_FILENAME \
            -O $BREAK_INSTALL_DIR/$MAKE_FILENAME
        tar -C $BREAK_INSTALL_DIR -xaf $MAKE_FILENAME
        cd make-4.0
        info "Configuring Make 4.0"
        try ./configure --prefix="$BREAK_MAKE_INSTALL_DIR" > /dev/null
        info "Building Make 4.0"
        try make -j > /dev/null
        info "Installing Make 4.0 to $BREAK_MAKE_INSTALL_DIR"
        try make install -j > /dev/null
        cd - > /dev/null
        echo "$MAKE" > .make_path
    fi
}

maybe_install_env() {
    if $INSTALL_ENV
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
        try echo alias break="$BREAK_INSTALL_DIR/break"        >> $SHRC
        try echo export BREAK_INCLUDE_DIR="$BREAK_INSTALL_DIR" >> $SHRC

        info "Please do 'source $SHRC' to finish installation."
    fi
}

parse_arguments $*

[ ! -d $BREAK_INSTALL_DIR ] && mkdir -p $BREAK_INSTALL_DIR
maybe_clone
maybe_install_make
maybe_install_env

congrats "Break is successfully installed and is ready to work!"
