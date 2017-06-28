#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include <vector>
#include <string>

#include "ScratchpadDatapath.h"

enum HandlerRet {
  CONTINUE,
  QUIT,
  HANDLER_NOT_FOUND,
  HANDLER_SUCCESS,
  HANDLER_ERROR,
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

extern const Command COMMANDS_END;

HandlerRet dispatch_command(const CommandTokens& tokens,
                            Command* command_list,
                            ScratchpadDatapath* acc);

int parse_command_args(const CommandTokens& command_tokens, CommandArgs& args);

#endif
