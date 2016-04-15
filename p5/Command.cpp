#include "Command.h"

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <wait.h>

#include <fcntl.h>

using std::cout;
using std::endl;

using boost::trim;
using boost::ends_with;
using boost::split;
using boost::is_any_of;

using std::vector;

//#define DEBUG(code) code
#define DEBUG(code)

using std::cerr;

ostream& operator << (ostream& os, const command_s& com)
{
    os << "name: {" << com.name << "}" << endl;

    if (!com.in.empty())  os << "stdin:  {" << com.in << "}" << endl;
    if (!com.out.empty()) os << "stdout: {" << com.out << "}" << endl;

    os << "params: [ ";
    for (auto param : com.params)
    {
        os << "{" <<  param << "}" << " ";
    }
    os << "]" << endl;

    return os;
}

void Command::parse_parts ()
{
    string& current_part = *(parts_.begin ());

    trim (current_part);
    if (ends_with (current_part, "&"))
    {
        parallel_ = true;
        current_part.erase (current_part.length () - 1);
    }

    vector<string> separators = {"&&", "||", "|"};

    for (auto const & separator : separators)
    {
        for (auto i = parts_.begin (); i != parts_.end (); ++ i)
        {
            if (std::find (separators.begin (), separators.end (), *i) != separators.end ())
            {
                continue;
            }

            unsigned long pos;
            while ((pos = (*i).find (separator)) != string::npos)
            {
                string& part = *i;

                auto right = part.substr (pos + separator.length (), string::npos);
                part.erase (pos, string::npos);

                ++ i;

                parts_.insert (i, separator);
                parts_.insert (i, right);

                -- i;
            }
        }
    }
}

command_s Command::parse_command (string command)
{
    command_s result;

    trim (command);

    parse_redirect (command, result);

    trim (command);

    split (result.params, command, is_any_of (L" "), boost::token_compress_on);

    result.name = result.params [0];
    //result.params.erase (result.params.begin ());

    return result;
}

void Command::parse_redirect(string &command, command_s &result)
{
    auto input  = command.find_last_of ("<");
    auto output = command.find_last_of (">");

    bool both = input != string::npos && output != string::npos;

    if ((both && input > output) || (!both && input != string::npos))
    {
        result.in = command.substr (input + 1, string::npos);
        trim (result.in);
        command.erase (input, string::npos);

        if (both) parse_redirect (command, result);
        return;
    }
    if ((both && input < output) || (!both && output != string::npos))
    {
        result.out = command.substr (output + 1, string::npos);
        trim (result.out);
        command.erase (output, string::npos);

        if (both) parse_redirect (command, result);
        return;
    }
}

void Command::execute (string command_text)
{
    parts_.clear ();
    parts_.push_back (command_text);

    parallel_ = false;

    parse_parts ();

    if (parallel_)
    {
        auto main_pid = fork ();
        if (main_pid != 0)
        {
            //main process
            errno = 0;
            perror((string("Spawned child process ") + std::to_string((int) main_pid)).c_str());
            parallel_pids_.push_back(main_pid);

            return;
        }
    }

    int exit_code = execute_rec (parts_.rbegin(), parts_.rend(), parallel_);
}

int Command::execute_rec (list <string>::reverse_iterator rbegin, list <string>::reverse_iterator rend, const bool exit)
{
    DEBUG (cerr << "rec from pid -> " << getpid() << endl);

    if (rend == rbegin)
    {
        if (exit) _exit (0);
        return 0;
    }

    if (!exit)
    {
        auto current_pid = fork ();
        assert (current_pid >= 0);
        if (current_pid == 0)
        {
            //child
            bind_signals (false);
            execute_rec (rbegin, rend, true);
        }

        set_signals_children(current_pid);

        auto exit_code = wait_until_doom (current_pid);

        DEBUG (cerr << "exit fork from pid with pid -> " << getpid() << " " << current_pid << endl);

        set_signals_children();

        return exit_code;
    }

    auto command = parse_command (*rbegin);
    ++ rbegin;

    if (rbegin == rend)
    {
        bind_signals (true);
        command.call_system();
        //end
    }

    auto chain = *rbegin;
    ++ rbegin;

    //and+or
    if (chain == "&&" || chain == "||")
    {
        auto exit_code = execute_rec (rbegin, rend, false);

        if ((chain == "&&" && exit_code == 0) || (chain == "||" && exit_code != 0))
        {
            command.call_system ();
            //end
        }
        else
        {
            _exit (exit_code);
            //end
        }
    }

    //pipe
    if (chain == "|")
    {
        DEBUG (cerr << "create pipe from pid -> " << getpid() << endl);

        const size_t read_end  = 0;
        const size_t write_end = 1;

        int pipes [2];
        pipe (pipes);

        auto left_pid = fork ();
        assert (left_pid >= 0);
        if (left_pid == 0)
        {
            //child
            bind_signals (true);

            dup2 (pipes [write_end], STDOUT_FILENO);
            close (pipes [read_end]);
            close (pipes [write_end]);

            execute_rec (rbegin, rend, true);
        }

        auto right_pid = fork ();
        assert (right_pid >= 0);
        if (right_pid == 0)
        {
            //child
            bind_signals (true);

            dup2 (pipes [read_end], STDIN_FILENO);
            close (pipes [read_end]);
            close (pipes [write_end]);

            command.call_system ();
        }

        close (pipes [read_end]);
        close (pipes [write_end]);

        set_signals_children (left_pid, right_pid);

        auto exit_code = wait_until_doom (left_pid);
        //drop it

        DEBUG (cerr << "spipe from pid with l/r -> " << getpid() << " " << left_pid << '/' << right_pid << endl);

        exit_code = wait_until_doom (right_pid);

        DEBUG (cerr << "pipe from pid with l/r -> " << getpid() << " " << left_pid << '/' << right_pid << endl);

        _exit (exit_code);
        //end
    }

    assert (false);
    return 1;
}

void command_s::call_system()
{
    if (name.empty ())
    {
        _exit (0);
    }

    std::unique_ptr <char*> args (new char* [params.size () + 1]);
    {
        auto i = 0;
        for (auto &arg : params)
        {
            args.get () [i] = const_cast <char*> (arg.c_str());
            i ++;
        }
        args.get () [i] = nullptr;
    }

    if (!in.empty ())
    {
        FILE* input = fopen (in.c_str (), "rb");

        if (!input) _exit (1);

        dup2 (fileno (input), STDIN_FILENO);
        fclose (input);
    }

    if (!out.empty ())
    {
        FILE* output = fopen (out.c_str (), "wb");

        dup2 (fileno (output), STDOUT_FILENO);
        fclose (output);
    }

    if (execvp (name.c_str(), args.get ()) == -1)
    {
        perror ("Bad command");
        _exit (1);
    }
}

void Command::check_pids (bool wait)
{
    for (int i = 0; i < parallel_pids_.size(); i ++)
    {
        int status = 0;
        if (waitpid (parallel_pids_ [i], &status, wait? 0 : WNOHANG | WUNTRACED) == parallel_pids_ [i])
        {
            auto exit_code = WEXITSTATUS (status);

            string msg = "Process ";
            msg += std::to_string (parallel_pids_ [i]);
            msg += " exited: ";
            msg += std::to_string (exit_code);

            perror (msg.c_str());

            parallel_pids_.erase (parallel_pids_.begin() + i);
            i --;
        }
        else
        {
            errno = 0;

            string msg = "Process ";
            msg += std::to_string (parallel_pids_ [i]);
            msg += " still running";

            perror (msg.c_str());
        }
    }
}

namespace SIGNALS_PROCESSOR
{
    bool exit = false;

    int children_n = 0;
    __pid_t children [2] = {};
};

void signal_handler (int signum)
{
    for (int i = 0; i < SIGNALS_PROCESSOR::children_n; i ++)
    {
        kill (SIGNALS_PROCESSOR::children [i], signum);
    }

    if (SIGNALS_PROCESSOR::exit) _exit (signum);
}

void bind_signals (bool exit)
{
    SIGNALS_PROCESSOR::exit = exit;

    struct sigaction new_action;

    //Set the handler in the new_action struct
    new_action.sa_handler = signal_handler;
    //Set to empty the sa_mask. It means that no signal is blocked
    // while the handler run.
    sigemptyset(&new_action.sa_mask);
    //Block the SEGTERM signal.
    // It means that while the handler run, the SIGTERM signal is ignored
    sigaddset(&new_action.sa_mask, SIGTERM);
    //Remove any flag from sa_flag. See documentation for flags allowed
    new_action.sa_flags = 0;

    assert (sigaction (SIGINT, &new_action, nullptr) != -1);
    assert (sigaction (SIGHUP, &new_action, nullptr) != -1);
}

void set_signals_children (pid_t child1, pid_t child2)
{
    SIGNALS_PROCESSOR::children_n = 0;

    if  (child1 != 0)
    {
        SIGNALS_PROCESSOR::children_n = 1;
        SIGNALS_PROCESSOR::children [0] = child1;
    }
    if  (child2 != 0)
    {
        SIGNALS_PROCESSOR::children_n = 2;
        SIGNALS_PROCESSOR::children [1] = child2;
    }
}

int wait_until_doom (pid_t pid)
{
    int status = 0;

    while (true)
    {
        auto res = waitpid (pid, &status, WUNTRACED);

        //pass signal to child, all ok
        if (res == -1 && errno == EINTR)
        {
            errno = 0;
            continue;
        }

        assert (res == pid);

        break;
    }

    return WEXITSTATUS (status);
}