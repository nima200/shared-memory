#ifndef ASSIGNMENT2_COMMANDS_H
#define ASSIGNMENT2_COMMANDS_H
#define FOREACH_COMMAND(command) \
        command(reserve), \
        command(init), \
        command(status), \
        command(invalid), \
        command(EXIT) \

#define GENERATE_ENUM(ENUM) ENUM
#define GENERATE_STRING(STRING) #STRING

typedef enum { FOREACH_COMMAND(GENERATE_ENUM) } command;
static const char *COMMAND_STRING[] = {FOREACH_COMMAND(GENERATE_STRING)};
command checkCommand(char *);
#endif //ASSIGNMENT2_COMMANDS_H
