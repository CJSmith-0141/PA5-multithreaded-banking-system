#include <pthread.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include "bank.h"

int bank_bootup()
{
  if ((BANK = malloc(sizeof(bank_t))) == NULL) {
    perror("MEMORY ALLOCATION ERROR");
    exit(SIGTERM);
  }
  BANK->count = 0;
  BANK->accounts = NULL;
  pthread_mutex_init(&BANK->mutex, NULL);
  return 1;
}

int accountOpen(char *account_name, account_ptr *current)
{
  account_ptr new;

  if (!validate_name_string(account_name)) {
    return BAD_NAME;
  }
  if (*current != NULL) {
    return ALREADY_STARTED;
  }
  pthread_mutex_lock(&BANK->mutex);
  if (BANK->count == MAX_ACCOUNTS) {
    pthread_mutex_unlock(&BANK->mutex); //ONLY ONE ACCOUNT CAN BE ADDED AT A TIME so the BANK HAS TO BE LOCKED
    return FULL;
  }
  HASH_FIND_STR(BANK->accounts, account_name, new);
  if (new != NULL) {  //The account already exists, can't create a new one
    pthread_mutex_unlock(&BANK->mutex); //IF IT FAILS IT STILL NEEDS TO BE LOCKED
    return DUPLICATE;
  }
  if ((new = malloc(sizeof(account_t))) == NULL) {
    perror("MEMORY ALLOCATION ERROR");
    exit(SIGTERM);
  }
  memset(new, 0, sizeof(account_t));
  memcpy(new->name, account_name, strlen(account_name)+1);
  if (new->name[100] != '\0') {
    free(new);
    pthread_mutex_unlock(&BANK->mutex); //WHEN THE NAME ISN'T PROPPER, THE BANK STILL NEEDS TO BE UNLOCKED 
    return BAD_NAME;
  }
  new->balance = 0;
  new->flag = 0;
  pthread_mutex_init(&new->lock, NULL);
  HASH_ADD_STR(BANK->accounts, name, new);//uthash is seriously amazing isn't it... 
  BANK->count++;
  pthread_mutex_unlock(&BANK->mutex); //WHEN IT' SUCCESSFUL THE BANK MUST BE UNLOCKED
  return 1;
}

int start(char *account_name, account_ptr *current)
{
  if (*current != NULL) {
    return ALREADY_STARTED;
  }
  HASH_FIND_STR(BANK->accounts, account_name, *current);
  if (*current == NULL) {
    return NOT_FOUND;
  }
  if (pthread_mutex_trylock(&(*current)->lock) == 0) {
    (*current)->flag = 1;
    return 1;
  }
  else {
    *current = NULL;
    return BUSY;
  }
}

int credit(char *amount, account_ptr *current)
{
  float a;
  
  if ((a = validate_float_string(amount)) < 0) {
    return BAD_AMOUNT;
  }
  if (*current == NULL) {
    return NOT_STARTED_YET;
  }
  (*current)->balance += a;
  return 1;
}

int debit(char *amount, account_ptr *current)
{
  float a;
  
  if ((a = validate_float_string(amount)) < 0) {
    return BAD_AMOUNT;
  }
  if (*current == NULL) {
    return NOT_STARTED_YET;
  }
  if (a > (*current)->balance) {
    return INSUFFICIENT;
  }
  (*current)->balance -= a;
  return 1;
}

float balance(account_ptr *current)
{
  if (*current == NULL) {
    return NOT_STARTED_YET;
  }
  return (*current)->balance;
}

int finish(account_ptr *current)
{
  if (*current == NULL) {
    return NOT_STARTED_YET;
  }
  (*current)->flag = 0;
  pthread_mutex_unlock(&(*current)->lock);
  (*current) = NULL;
  return 1;
}

int clientExit(account_ptr *current)
{
  if (*current != NULL) {
    finish(current);
  }
  return 1;
}

void print_bank_roll()
{
  account_t *itr;
  pthread_mutex_lock(&BANK->mutex);
  printf("Bank Info:\n");
  if (BANK->count == 0) {
    printf("No Account\n");
  }
  else {
    for(itr = BANK->accounts; itr != NULL; itr=itr->hh.next) {
      printf("%s\t\tbalance: $%.2f", itr->name, itr->balance); //Doesn't change the value, just truncates for later
      if (itr->flag) {
        printf("\t\tIN SERVICE\n");
      }
      else {
        printf("\n");
      }
    }
  }
  pthread_mutex_unlock(&BANK->mutex);
}

char* get_error_info(int error) //THE CALLING FUNCTION IS RESPOSIBLE FOR FREEING THE ALLOCATED MEMORY RETURNED BY THIS FUNCTION
{
  char *ret;
  if ((ret = malloc(sizeof(char)*ERROR_BLURB)) == NULL) { //HMM instead of malloc I could just have it be a static size, make all bytes zero and then put it to save time, but really not a big deal since this is self contained. 
    fprintf(stderr, "server: MEMORY ALLOCATION FAILURE\n");
    exit(SIGTERM);
  }
  memset(ret, 0, ERROR_BLURB);
  switch (error) {
    case FULL: sprintf(ret, "Maximum number of accounts reached..."); break;
    case DUPLICATE: sprintf(ret, "Account already exists, use \"start\" to make customer session"); break;
    case ALREADY_STARTED: sprintf(ret, "CAN ONLY SERVE ONE ACCOUNT AT A TIME use \"finish\"to end customer session"); break;
    case NOT_STARTED_YET: sprintf(ret, "You must \"start\" a customer session before taking customer actions on an account"); break;
    case NOT_FOUND: sprintf(ret, "No Account by that name!"); break;
    case INSUFFICIENT: sprintf(ret, "NOT ENOUGH FUNDS!"); break;
    case BAD_NAME: sprintf(ret, "invalid account name"); break;
    case BAD_AMOUNT: sprintf(ret, "Malformed input, must be a positive number!"); break;
    default: break;
  }
  return ret;
}

int validate_name_string(char *name) {
  char *c;
  int ret;
  
  ret = 1;
  c = name;
  do {
    if (isalnum(*c) != 0) { //cycles through the string, rejects the entire string if any one character is bad
      c++;
    }
    else {
      ret = 0;
      break;
    }
  } while (*c != '\0');
  return ret;
}

float validate_float_string(char *amount) {
  float ret;
  char *endPtr;
  
  if (*amount == '\0') {
    return -1;
  }
  ret = strtof(amount, &endPtr); //strtof converts a string to a float
  if (*endPtr != '\0') { //By convetion in strtof, if the endpointer isn't a null character, the conversion is invalid (in this case)
    ret = -1;
  }
  return ret;
}


char helpHelper[105] = "command syntax:\nopen accountname\nstart accountname\ncredit amount\ndebit amount\nbalance\nfinish\nexit";
