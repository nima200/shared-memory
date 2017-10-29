#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <memory.h>
#include <ctype.h>
#include <semaphore.h>
#define MAX_RESERVATION_COUNT 10
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

// Struct declarations
typedef enum { false, true } bool;

typedef struct {
    int table_number;
    bool reserved;
    char reservee[20];
} res_info;

typedef struct {
    res_info reservations[MAX_RESERVATION_COUNT];
} res_table;

typedef struct {
    int count;
} db_reader;

// Function declarations
int initResTable(char *, res_table **, int, sem_t **);
int reserveTable(res_table **, int, char *, sem_t **);
void initNullArr(char**, int);
int getCmd(char*, char**);
int parseCmd(char **, char **);
int execute(char **, res_table **, sem_t **[2], sem_t **[2], db_reader **);
void validateReservation(const char *, const char *, int);
void executeFile(char const *, res_table **, sem_t **[2], sem_t **[2], db_reader **);
void printReservations(res_table **, char *, sem_t **w_lock, sem_t **, db_reader **reader);
int initDBReaders(char *, db_reader **, sem_t **);

int main(int argc, char **argv) {
    res_table* tableA;
    res_table* tableB;
    db_reader* dbr_tableA;
    db_reader* dbr_tableB;
    // Allocate space to the semaphores
    sem_t** x_lock_A = malloc(sizeof(sem_t));
    sem_t** x_lock_B = malloc(sizeof(sem_t));
    sem_t** r_lock_A = malloc(sizeof(sem_t));
    sem_t** r_lock_B = malloc(sizeof(sem_t));
    // Create a semaphore for each section's reservation tables
    *x_lock_A = sem_open("x_LockA", O_CREAT, 0644, 1);
    *x_lock_B = sem_open("x_LockB", O_CREAT, 0644, 1);
    // Create a semaphore for each section's reader count
    *r_lock_A = sem_open("s_lockA", O_CREAT, 0644, 1);
    *r_lock_B = sem_open("s_lockB", O_CREAT, 0644, 1);
    // Initialize shared memory space for the tables of each section
    initResTable("260606511_A", &tableA, 100, x_lock_A);
    initResTable("260606511_B", &tableB, 200, x_lock_B);
    // Initialize shared memory space for reader counter, for each section
    initDBReaders("260606511_A_r", &dbr_tableA, r_lock_A);
    initDBReaders("260606511_B_r", &dbr_tableB, r_lock_B);
    res_table* tables[] = {tableA, tableB};
    db_reader* readers[] = {dbr_tableA, dbr_tableB};
    sem_t** db_locks[2] = {x_lock_A, x_lock_B};
    sem_t** r_locks[2] = {r_lock_A, r_lock_B};
    if (argc > 1) {
        executeFile(argv[1], tables, db_locks, r_locks, readers);
    }
    while (1) {
        char *args[4];
        initNullArr(args, 4);
        getCmd("\u03BB ", args);
        if (execute(args, tables, db_locks, r_locks, readers) == -1) {
            printf("ERROR: Invalid command.\n");
        }
    }
}

void
executeFile(char const *fileName, res_table **tables, sem_t **db_locks[2], sem_t **r_locks[2], db_reader **readers) {
    FILE *file = fopen(fileName, "r");
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *args[4];
        initNullArr(args, 4);
        char *lineCopy = line;
        parseCmd(args, &lineCopy);
        execute(args, tables, db_locks, r_locks, readers);
    }
}

int initResTable(char *name, res_table **tables, int startingTable, sem_t **x_lock) {
    struct stat s;
    // Pass O_EXCL to know if it exists before -> fd = -1
    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
    // New reservation table
    if (fd < 0) {
        fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);
        // Error occurred opening the shared memory
        if (fd < 0) {
            printf("ERROR: Could not open shared memory space\n");
            return -1;
        }
    }
    if (fstat(fd, &s) == -1) {
        printf("ERROR: Could not verify size of shared memory object\n");
        return -1;
    }
    (*tables) = mmap(NULL, sizeof(res_table), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(res_table));
    close(fd);
    // WRITE TABLES START
    sem_wait((*x_lock));
    for (int i = 0; i < MAX_RESERVATION_COUNT; i++) {
        (*tables)->reservations[i].table_number = startingTable + i;
        (*tables)->reservations[i].reserved = false;
        strcpy((*tables)->reservations[i].reservee, "None");
    }
    sem_post((*x_lock));
    // WRITE TABLES END
    return 0;
}


int initDBReaders(char *name, db_reader **reader, sem_t **s_lock) {
    struct stat s;
    // Pass O_EXCL to know if it exists before or not -> fd = -1
    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
    // If it exists from before
    if (fd < 0)  {
        fd  = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);
        // Error occurred opening the shared memory
        if (fd < 0) {
            printf("ERROR: Could not open shared memory space\n");
            return -1;
        }
    }
    if (fstat(fd, &s) == -1) {
        printf("ERROR: Could not verify size of shared memory object\n");
        return -1;
    }
    (*reader)  = mmap(NULL, sizeof(db_reader), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(db_reader));
    // Get exclusive access to the reader object, to initialize it
    sem_wait((*s_lock));
    // CRITICAL SECTION START
    (*reader)->count = 0;
    // CRITICAL SECTION END
    sem_post((*s_lock));
    return 0;
}

/**
 * Attempts to reserve a table at the reservation table provided, given a table number and a reservee name.
 * @param res_table The reservation table to search through
 * @param tableNumber The table number to reserve. If unspecified and does not matter which table, pass -1
 * @param reservee The name of the person to reserve the table for
 * @return  -1 if table number was more than capacity
 *          -2 if table was already reserved
 *          'table number' if reservation was successful
 */
int reserveTable(res_table **res_table, int tableNumber, char *reservee, sem_t **w_lock) {
    if (tableNumber >= 100 && tableNumber < 200) {
        tableNumber = tableNumber - 100;
    } else if (tableNumber >= 200) {
        tableNumber = tableNumber - 200;
    }
    if (tableNumber > MAX_RESERVATION_COUNT - 1 || tableNumber < -1) {
        // -1 for invalid table number
        return -1;
    }
    // -1 if table was not specified by user
    if (tableNumber == -1) {
         // Assume no tables available in this section for reservation
        int reservation = -3;
        // WRITE TABLE START
        sem_wait((*w_lock));
        for (int i = 0; i < MAX_RESERVATION_COUNT; i++) {
            if (!(*res_table)->reservations[i].reserved) {
                (*res_table)->reservations[i].reserved = true;
                strcpy((*res_table)->reservations[i].reservee, reservee);
                reservation = (*res_table)->reservations[i].table_number;
                break;
            }
        }
        // WRITE TABLE END
        sem_post((*w_lock));
        return reservation;
    } else {
        // Assume the table is already reserved
        int reservation = -2;
        // WRITE TABLE START
        sem_wait((*w_lock));
        if ((*res_table)->reservations[tableNumber].reserved == false) {
            (*res_table)->reservations[tableNumber].reserved = true;
            strcpy((*res_table)->reservations[tableNumber].reservee, reservee);
            reservation = (*res_table)->reservations[tableNumber].table_number;
        }
        sem_post((*w_lock));
        // WRITE TABLE END
        return reservation;
    }
}
/**
 * Initializes a char* array with NULL
 * @param arr The array to initialize
 * @param size The size of the array
 */
void initNullArr(char *arr[], int size) {
    for (int i = 0; i < size; ++i) {
        arr[i] = NULL;
    }
}

int getCmd(char *prompt, char **args) {
    size_t lineSize = strlen(prompt);
    char *line = NULL;
    __ssize_t length = 0;
    printf("%s >", prompt);
    length = getline(&line, &lineSize, stdin);
    // Should never happen
    if (length <= 0) {
        printf("ERROR: A fatal error has occurred. Exiting");
        exit(1);
    }
    char *line_copy = line;
    return parseCmd(args, &line_copy);
}

/**
 * Parses a command line to a set of arguments, looking for 'SPACE' or other non-text ASCII characters as
 * the delimiter for argument separation.
 * @note                    The returned value includes the command to be executed + the number flags
 * @param args              Pointer to the array to store the extracted arguments in
 * @param line              Pointer to the character array that contains the command line
 * @return                  The number of arguments in the user's command
 */
int parseCmd(char **args, char **line) {
    char *token;
    int i = 0;
    while ((token = strsep(line, " \t\n")) != NULL) {
        // Find the first non standard ASCII Text chars and/or SPACE at and end the token there
        for (int j = 0; j < strlen(token); j++) {
            if (token[j] <= 32) { // 32 = ASCII SPACE
                token[j] = '\0';
            }
        }
        // If the token is not empty, add to set of args and increment i for the next token
        if (strlen(token) > 0) {
            args[i++] = token;
        }
    }
    return i;
}

int execute(char **args, res_table **tables, sem_t **w_locks[2], sem_t **r_locks[2], db_reader **readers) {
    // Check if no args were passed
    if (args[0] == NULL) {
        return -1;
    }
    sleep(1);
    command command = checkCommand(args[0]);
    switch (command) {
        case reserve: {
            // First two arguments are not optional
            if (args[1] == NULL || args[2] == NULL) {
                return -1;
            }
            int tableNumber = args[3] != NULL ? atoi(args[3]) : -1;
            char* name = args[1];
            char* section = args[2];
            char section_lower = (char) tolower(*section);
            int isA = section_lower == 'a';
            int isB = section_lower == 'b';
            int reservationStatus;
            if (isA == 0 && isB == 0) {
                printf("Sorry, that's not a valid section!\n");
                return 0;
            } else if (isA != 0) {
                // Reserve for section A
                reservationStatus = reserveTable(&tables[0], tableNumber, name, &(*w_locks[0]));
            } else {
                // Reserve for section B
                reservationStatus = reserveTable(&tables[1], tableNumber, name, &(*w_locks[1]));
            }
            validateReservation(name, section, reservationStatus);
            return 0;
        }
        case init:
            initResTable("260606511_A", &tables[0], 100, &(*w_locks[0]));
            initResTable("260606511_B", &tables[1], 200, &(*w_locks[1]));
            printf("Successfully re-initialized all reservation tables\n");
            return 0;
        case status:
            printReservations(&tables[0], "A", &(*w_locks[0]), &(*r_locks[0]), &readers[0]);
            printReservations(&tables[1], "B", &(*w_locks[1]), &(*r_locks[1]), &readers[1]);
            break;
        case invalid:
            return -1;
        case EXIT:
            // Free the semaphores
            free(&(*w_locks[0]));
            free(&(*w_locks[1]));
            free(&(*r_locks[0]));
            free(&(*r_locks[1]));
            exit(0);
    }
}

void validateReservation(const char *name, const char *section, int reservationStatus) {
    if (reservationStatus == -1) {
                    printf("Sorry, that is not a valid table number\n");
                } else if (reservationStatus == -2) {
                    printf("Sorry, that table is already reserved!\n");
                } else if (reservationStatus == -3) {
                    printf("Sorry, there are no more tables available in section %s!\n", section);
                } else {
                    char section_upper = (char) toupper(*section);
                    printf("Table %d in section %s successfully reserved for %s\n", reservationStatus, &section_upper, name);
                }
}

/**
 * Prints the reservation status for all tables in an array of reservations
 * @param tables The reservation array
 * @param section The reservation section
 */
void printReservations(res_table **tables, char *section, sem_t **w_lock, sem_t **r_lock, db_reader **reader) {
    // READ TABLES START
    sem_wait((*r_lock));
    (*reader)->count = (*reader)->count + 1;
    if ((*reader)->count == 1) {
        sem_wait((*w_lock));
    }
    sem_post((*r_lock));
    for (int i = 0; i < MAX_RESERVATION_COUNT; i++) {
        printf("[Section %s] ", section);
        printf("[Table %d] ", i);
        printf("[Reservee: %s] ", (*tables)->reservations[i].reservee);
        printf("[Currently reserved: %s]\n", (*tables)->reservations[i].reserved ? "Yes": "No");
    }
    printf("---------------------------------------------------------------\n");
    sem_wait((*r_lock));
    (*reader)->count = (*reader)->count - 1;
    if ((*reader)->count == 0) {
        sem_post((*w_lock));
    }
    sem_post((*r_lock));
    // READ TABLES END
}

command checkCommand(char *command) {
    if (strcmp(command, COMMAND_STRING[reserve]) == 0) return reserve;
    if (strcmp(command, COMMAND_STRING[init]) == 0) return init;
    if (strcmp(command, COMMAND_STRING[status]) == 0) return status;
    if (strcmp(command, "exit") == 0) return EXIT;
    return invalid;
}