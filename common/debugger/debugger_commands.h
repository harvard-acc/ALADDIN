#ifndef _DEBUGGER_COMMANDS_H_
#define _DEBUGGER_COMMANDS_H_

#include "debugger_prompt.h"

namespace adb {

HandlerRet cmd_print(const CommandTokens& tokens,
                     Command* subcmd_list,
                     ScratchpadDatapath* acc);
HandlerRet cmd_print_node(const CommandTokens& tokens,
                          Command* subcmd_list,
                          ScratchpadDatapath* acc);
HandlerRet cmd_print_edge(const CommandTokens& tokens,
                          Command* subcmd_list,
                          ScratchpadDatapath* acc);
HandlerRet cmd_print_loop(const CommandTokens& tokens,
                          Command* subcmd_list,
                          ScratchpadDatapath* acc);
HandlerRet cmd_print_function(const CommandTokens& tokens,
                              Command* subcmd_list,
                              ScratchpadDatapath* acc);
HandlerRet cmd_print_cycle(const CommandTokens& tokens,
                           Command* subcmd_list,
                           ScratchpadDatapath* acc);
HandlerRet cmd_graph(const CommandTokens& tokens,
                     Command* subcmd_list,
                     ScratchpadDatapath* acc);
HandlerRet cmd_help(const CommandTokens& tokens,
                    Command* subcmd_list,
                    ScratchpadDatapath* acc);
HandlerRet cmd_continue(const CommandTokens& tokens,
                        Command* subcmd_list,
                        ScratchpadDatapath* acc);
HandlerRet cmd_quit(const CommandTokens& tokens,
                    Command* subcmd_list,
                    ScratchpadDatapath* acc);

};  // namespace adb

#endif
