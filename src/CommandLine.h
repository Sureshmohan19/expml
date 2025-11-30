#ifndef EXPML_COMMANDLINE_H
#define EXPML_COMMANDLINE_H

typedef enum {
   CMD_SUCCESS,
   CMD_ERROR,
   CMD_EXIT
} CommandStatus;

int CommandLine_run(int argc, char** argv);

#endif