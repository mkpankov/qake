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
                QAKE_INSTALL_DIR=$2
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
                QAKE_URL=$2
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
    [ -z "$QAKE_URL" ] && QAKE_URL="git@github.com:constantius9/qake.git"
    [ -z "$QAKE_INSTALL_DIR" ] && QAKE_INSTALL_DIR="$HOME/qake/"
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
        info "Cloning $QAKE_URL into $QAKE_INSTALL_DIR"
        try git clone $QAKE_URL $QAKE_INSTALL_DIR
    fi
}

maybe_install_make() {
    if $INSTALL_MAKE
    then
        readonly MAKE_FILENAME=make-4.0.tar.bz2
        readonly QAKE_MAKE_INSTALL_DIR="$QAKE_INSTALL_DIR/make-4.0/install"
        readonly MAKE="$QAKE_MAKE_INSTALL_DIR"/bin/make
        info "Downloading Make 4.0 to $QAKE_INSTALL_DIR/$MAKE_FILENAME"
        wget http://ftp.gnu.org/gnu/make/$MAKE_FILENAME \
            -O $QAKE_INSTALL_DIR/$MAKE_FILENAME
        cd $QAKE_INSTALLDIR
        tar -xf $QAKE_INSTALL_DIR/$MAKE_FILENAME
        cd make-4.0
        info "Configuring Make 4.0"
        try ./configure --prefix="$QAKE_MAKE_INSTALL_DIR" > /dev/null
        info "Building Make 4.0"
        try make -j > /dev/null
        info "Installing Make 4.0 to $QAKE_MAKE_INSTALL_DIR"
        try make install -j > /dev/null
        echo "$MAKE" > $QAKE_INSTALL_DIR/.make_path
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
        try echo alias qake="$QAKE_INSTALL_DIR/qake"         >> $SHRC
        try echo export QAKE_INCLUDE_DIR="$QAKE_INSTALL_DIR" >> $SHRC

        info "Please do 'source $SHRC' to finish installation."
    fi
}

parse_arguments $*

[ ! -d $QAKE_INSTALL_DIR ] && mkdir -p $QAKE_INSTALL_DIR
maybe_clone
maybe_install_make
maybe_install_env

congrats "Qake is successfully installed and is ready to work!"
