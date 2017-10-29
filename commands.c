#include <memory.h>
#include "commands.h"

command checkCommand(char *command) {
    if (strcmp(command, COMMAND_STRING[reserve]) == 0) return reserve;
    if (strcmp(command, COMMAND_STRING[init]) == 0) return init;
    if (strcmp(command, COMMAND_STRING[status]) == 0) return status;
    if (strcmp(command, "exit") == 0) return EXIT;
    return invalid;
}