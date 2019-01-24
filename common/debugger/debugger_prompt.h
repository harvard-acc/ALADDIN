#ifndef _DEBUGGER_PROMPT_H_
#define _DEBUGGER_PROMPT_H_

#include <vector>
#include <string>

#include "../ScratchpadDatapath.h"

namespace adb {

enum HandlerRet {
  CONTINUE,
  QUIT,
  HANDLER_NOT_FOUND,
  HANDLER_SUCCESS,
  HANDLER_ERROR,
};

enum ExecutionStatus {
  PRESCHEDULING,
  SCHEDULING,
  POSTSCHEDULING,
};

typedef std::vector<std::string> CommandTokens;
typedef std::map<std::string, int> CommandArgs;

struct Command {
  typedef HandlerRet (*CommandHandler)(const CommandTokens&,
                                       Command*,
                                       ScratchpadDatapath*);

  std::string command;
  CommandHandler handler;
  Command* subcommands;

  bool operator==(const Command& other) {
    return (command == other.command && handler == other.handler &&
            subcommands == other.subcommands);
  }

  bool operator!=(const Command& other) {
    return !(*this == other);
  }
};

HandlerRet dispatch_command(const CommandTokens& tokens,
                            Command* command_list,
                            ScratchpadDatapath* acc);
HandlerRet interactive_mode(ScratchpadDatapath* acc);

void init_debugger();
int parse_command_args(const CommandTokens& command_tokens, CommandArgs& args);

extern ExecutionStatus execution_status;

};  // namespace adb

#endif
