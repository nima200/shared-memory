#ifndef ASSIGNMENT2_RESERVATION_INFORMATION_H
#define ASSIGNMENT2_RESERVATION_INFORMATION_H
#define MAX_RESERVATION_COUNT 10
typedef enum { false, true } bool;
typedef struct {
    int table_number;
    bool reserved;
    char reservee[20];
} res_info;
typedef struct {
    res_info reservations[MAX_RESERVATION_COUNT];
} res_table;
#endif //ASSIGNMENT2_RESERVATION_INFORMATION_H
