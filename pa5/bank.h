#ifndef BANK_H
#define BANK_H

#include <stdio.h>
#include <pthread.h>
#include "uthash.h"

//size macros
#define MAX_ACCOUNTS 20
#define ACCOUNT_NAME_MAX_LENGTH 101 //101 so there is always room for the null character SO LONG AS IT'S WELL FORMED
#define ERROR_BLURB 85

//error macros
#define FULL -2
#define DUPLICATE -3
#define BUSY -4
#define ALREADY_STARTED -5
#define NOT_STARTED_YET -6
#define NOT_FOUND -7
#define INSUFFICIENT -8
#define BAD_NAME -9
#define BAD_AMOUNT -10

struct account{
  char name[ACCOUNT_NAME_MAX_LENGTH];
  float balance;
  int flag;
  pthread_mutex_t lock; //EVERY ACCOUNT HAS A LOCK, HOW DOPE IS THAT
  UT_hash_handle hh;  //WHERE HAS THIS BEEN MY WHOLE LIFE AHHHHHHHHHHH so FREAKING DOPE
};

typedef struct account account_t;
typedef account_t *account_ptr; //explicitly typedefs account pointer 


struct bank {
  account_ptr accounts;
  int count;
  pthread_mutex_t mutex; // THE BANK ITSELF HAS A LOCK AS WELL
};

typedef struct bank bank_t;

bank_t *BANK;
extern char helpHelper[105]; //this is a string that can be sent to the client to tell them the command syntax

int bank_bootup();
int accountOpen(char*, account_ptr*);
int start(char*, account_ptr*);
int credit(char*, account_ptr*);
int debit(char*, account_ptr*);
float balance(account_ptr*);
int finish(account_ptr*);
int clientExit(account_ptr*);
int destroy_bank();
void print_bank_roll();
char* get_error_info(int);

int validate_name_string(char*);
float validate_float_string(char*);

#endif 
