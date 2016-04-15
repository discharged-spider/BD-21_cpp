#ifndef P5_COMMAND_H
#define P5_COMMAND_H

#include <list>
#include <string>
#include <vector>

#include <unistd.h>
#include <csignal>

using std::list;
using std::string;
using std::vector;
using std::ostream;

struct command_s
{
    string name;
    vector <string> params;
    string in;
    string out;

    void call_system();
};

ostream& operator << (ostream& os, const command_s& com);

class Command
{
private:
    bool parallel_;
    list <string> parts_;

    vector <pid_t> parallel_pids_;

    void parse_parts ();

    command_s parse_command (string command);
    void parse_redirect (string &command, command_s &result);

    int execute_rec (list <string>::reverse_iterator rbegin, list <string>::reverse_iterator rend, const bool exit = false);
public:
    Command () :
        parallel_ (false)
    {}

    void execute (string command);
    void check_pids (bool wait = false);
};

void bind_signals (bool exit);
void set_signals_children(pid_t child1 = 0, pid_t child2 = 0);

int wait_until_doom (pid_t pid);

#endif //P5_COMMAND_H
