#ifndef ASSIGNMENT2_MAIN_H
#define ASSIGNMENT2_MAIN_H

#include "reservation_information.h"

int initResTable(char *, res_table **, int, sem_t **db_lock);
int reserveTable(res_table **, int, char *, sem_t **db_lock);
void initNullArr(char**, int);
int getCmd(char*, char**);
int parseCmd(char **, char **);
int execute(char **, res_table **, sem_t **[2]);
void validateReservation(const char *, const char *, int);
void executeFile(char const *, res_table **, sem_t **[2]);
void printReservations(res_table **tables, char *section, sem_t **db_lock);
#endif //ASSIGNMENT2_MAIN_H
