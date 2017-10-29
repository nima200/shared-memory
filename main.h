#ifndef ASSIGNMENT2_MAIN_H
#define ASSIGNMENT2_MAIN_H

#include "reservation_information.h"
#include "readers.h"

int initResTable(char *, res_table **, int, sem_t **);
int reserveTable(res_table **, int, char *, sem_t **, sem_t **r_lock, db_reader **reader);
void initNullArr(char**, int);
int getCmd(char*, char**);
int parseCmd(char **, char **);
int execute(char **, res_table **, sem_t **[2], sem_t **r_locks[2], db_reader **readers);
void validateReservation(const char *, const char *, int);
void executeFile(char const *, res_table **, sem_t **[2], sem_t **r_locks[2], db_reader **readers);
void printReservations(res_table **, char *, sem_t **);
int initDBReaders(char *, db_reader **, sem_t **);
#endif //ASSIGNMENT2_MAIN_H
