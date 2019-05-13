#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include "constants.h"
#include "types.h"
#include "sope.h"

int main(int argc, char *argv[]){

    if(argc != 3){
        printf("Wrong number of args!\n");
        return -1;
    }

    if(atoi(argv[1]) <= 99){
        printf("The number of bank offices must be at max 99!\n");
        return -1;
    }

    mkfifo(SERVER_FIFO_PATH, 0666);
    bank_account_t admin_acc;
    admin_acc.account_id = ADMIN_ACCOUNT_ID;
    admin_acc.balance = 0;



    

}
