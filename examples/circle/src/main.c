#include "ircfunc.h"

int main(int argc, char ** argv)
{
    IRCENV ircenv;

    ircenv = circle_init(argv[0]);
    ircenv.load_args(&ircenv, argc, argv);
    return ircenv.init(&ircenv);
}
