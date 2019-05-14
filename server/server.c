#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include "hashsum.h"
#include "string.h"
#include "../utils/constants.h"
#include "../utils/types.h"
#include "../utils/sope.h"

char* saltGenerator(char *salt){

    char randomSalt[] = "0123456789abcdef";
    for(int i = 0; i < SALT_LEN; i++){
        salt[i] = randomSalt[rand() % (strlen(randomSalt))];
    }

    salt[SALT_LEN] = '\0';

    return salt;
}

int main(int argc, char *argv[]){

    if(argc != 3){
        printf("Wrong number of args!\n");
        return -1;
    }

    if(atoi(argv[1]) > 99){
        printf("The number of bank offices must be at max 99!\n");
        return -1;
    }

    mkfifo(SERVER_FIFO_PATH, 0666);
    bank_account_t admin_acc;
    admin_acc.account_id = ADMIN_ACCOUNT_ID;
    admin_acc.balance = 0;


    saltGenerator(admin_acc.salt);

    printf("ola\n");
    hashGenerator(admin_acc.salt, argv[2]);
    

}
