#include <cstdlib>
#include <exception>
#include <string>
#include <signal.h>

#include <boost/tokenizer.hpp>

#include "../DDDG.h"
#include "debugger_commands.h"
#include "debugger_prompt.h"
#include "../file_func.h"
#include "../Scratchpad.h"
#include "../ScratchpadDatapath.h"

#ifdef HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

using namespace adb;

// Place this at the end of a set of commands to denote the end.
const Command COMMANDS_END = {"", NULL, NULL};

Command subcmd_print[] = {
  {"node", cmd_print_node, NULL},
  {"edge", cmd_print_edge, NULL},
  {"loop", cmd_print_loop, NULL},
  {"function", cmd_print_function, NULL},
  {"cycle", cmd_print_cycle, NULL},
  COMMANDS_END,
};

Command commands[]  = {
  {"continue", cmd_continue, NULL},
  {"quit", cmd_quit, NULL},
  {"print", cmd_print, subcmd_print},
  {"graph", cmd_graph, NULL},
  {"help", cmd_help, NULL},
  COMMANDS_END,
};

// Determines how SIGINT is handled. If we're waiting for an input, then we
// want to resume the application; otherwise, we want to kill it.
static sig_atomic_t waiting_for_input;

// Whether we compute execution statistics or not depends on where we are in
// the scheduling.
ExecutionStatus adb::execution_status;

std::string get_command() {
  std::string command;
  waiting_for_input = 1;
#ifdef HAS_READLINE
  char* cmd = readline("adb >> ");
  if (cmd && *cmd)
    add_history(cmd);
  command = std::string(cmd);
  free(cmd);
#else
  std::cout << "adb >> ";
  std::getline(std::cin, command);
#endif
  waiting_for_input = 0;
  return command;
}

// Parse a space-separated list of command args of the form param=INT.
//
// The parsed result is placed into @args.
//
// Return:
//  0 on success, -1 otherwise.
int adb::parse_command_args(const CommandTokens& command_tokens,
                            CommandArgs& args) {
  boost::char_separator<char> sep("=");
  for (unsigned i = 0; i < command_tokens.size(); i++) {
    boost::tokenizer<boost::char_separator<char>> tok(command_tokens[i], sep);
    std::string arg_name;
    int value = -1;
    int argnum = 0;
    for (auto it = tok.begin(); it != tok.end(); ++it, ++argnum) {
      if (argnum == 0) {
        arg_name = *it;
        continue;
      } else if (argnum == 1) {
        try {
          value = stoi(*it);
        } catch (const std::invalid_argument &e) {
          std::cerr << "ERROR: Invalid argument " << *it << " to parameter " << arg_name << ".\n";
          return -1;
        }
      }
      if (value == -1) {
        std::cerr << "ERROR: Missing value to parameter " << arg_name << ".\n";
        return -1;
      }
      args[arg_name] = value;
    }
  }
  return 0;
}

HandlerRet adb::dispatch_command(const CommandTokens& command_tokens,
                                 Command* command_list,
                                 ScratchpadDatapath* acc) {
  if (!command_list)
    return HANDLER_NOT_FOUND;
  Command current_command = command_list[0];

  unsigned i = 0;
  while (current_command != COMMANDS_END) {
    if (current_command.command == command_tokens[0]) {
      if (current_command.handler)
        return current_command.handler(
            command_tokens, current_command.subcommands, acc);
      else
        return HANDLER_NOT_FOUND;
    }
    i++;
    current_command = command_list[i];
  }
  return HANDLER_NOT_FOUND;
}

HandlerRet cmd_unknown(std::string& command) {
  std::cout << "\nUnknown command " << command << std::endl;
  return HANDLER_SUCCESS;
}

HandlerRet adb::interactive_mode(ScratchpadDatapath* acc) {
  std::cout << "Entering Aladdin Debugger...\n";
  while (true) {
    std::string command = get_command();

    if (command.empty()) {
      continue;
    }

    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char>> tok(command, sep);
    CommandTokens command_split;
    for (auto it = tok.begin(); it != tok.end(); ++it) {
      command_split.push_back(*it);
    }

    if (command_split.size() == 0) {
      continue;
    }

    HandlerRet ret = dispatch_command(command_split, commands, acc);
    if (ret == QUIT || ret == CONTINUE)
      return ret;
    else if (ret == HANDLER_NOT_FOUND)
      cmd_unknown(command);
  }
}

#ifdef HAS_READLINE
void handle_sigint(int signo) {
  if (waiting_for_input) {
    // Move to a new line and clear the existing one.
    printf("\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
    // Reregister the signal handler.
    signal(SIGINT, handle_sigint);
    return;
  }
  exit(1);
}

void init_readline() {
  // Installs signal handlers to capture SIGINT, so we can move to a new prompt
  // instead of ending the process.
  signal(SIGINT, handle_sigint);
}
#endif

void adb::init_debugger() {
  waiting_for_input = 0;
  adb::execution_status = PRESCHEDULING;
#ifdef HAS_READLINE
  init_readline();
#endif
}
