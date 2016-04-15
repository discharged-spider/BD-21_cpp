#include <boost/tokenizer.hpp>
#include <iostream>

#include "Command.h"

using std::cin;
using std::cout;
using std::endl;

int main ()
{
    bind_signals (true);

    string command_s = "";

    bool terminal = isatty (fileno (stdout)) == 1;
    if (terminal)
    {
        cout << "Hello from little bash! (pid = " << getpid() << ")" << endl;
    }

    string hello_msg = "$>";

    if (terminal)
    {
        cout << hello_msg;
    }

    Command command;
    while (std::getline (cin, command_s))
    {
        command.check_pids ();

        bind_signals (false);
        command.execute (command_s);
        bind_signals (true);

        if (terminal)
        {
            cout << hello_msg;
        }
    }

    command.check_pids (true);

    return 0;
}
