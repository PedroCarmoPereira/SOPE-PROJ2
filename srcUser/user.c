#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include "../utils/constants.h"
#include "../utils/types.h"
#include "../utils/sope.h"

int main(int argc, char *argv[])
{

    if (argc != 6)
    {
        printf("Wrong number of args!\n");
        return -1;
    }

    if(atoi(argv[4]) != 0 && atoi(argv[4]) != 1 && atoi(argv[4]) != 2 && atoi(argv[4]) != 3){
        printf("The operation number must be either 0, 1, 2 or 3\n");
        return -1;
    }

    char str_pid[WIDTH_ID];
    char fifo_name[USER_FIFO_PATH_LEN];
    sprintf(str_pid, "%d", getpid());
    strcpy(fifo_name, USER_FIFO_PATH_PREFIX);
    mkfifo( strcat(fifo_name, str_pid) , 0666);
    op_type_t type;

    
    char* listofargs = argv[5];

    int i = 0;
    char *p = strtok (listofargs, " ");
    char *args[3];

    while (p != NULL)
    {
        args[i++] = p;
        p = strtok (NULL, " ");
    }
    
   
    switch (atoi(argv[4]))
    {
    case 0:
        type = OP_CREATE_ACCOUNT;
        break;
    case 1:
        type = OP_BALANCE;
        break;
    case 2:
        type = OP_TRANSFER;
        break;
    case 3:
        type = OP_SHUTDOWN;
        break;
    }

    
    if (type == OP_CREATE_ACCOUNT){
        if(strlen(args[0]) > WIDTH_ACCOUNT){
            printf("The account id's width must be at max 4 characters!\n");
            return -1;
        }
        if(strlen(args[1]) > WIDTH_BALANCE){
            printf("The balance width must be at max 10 characters!\n");
            return -1;
        }
        if(atoi(args[0]) < 1 || atoi(args[0]) >= MAX_BANK_ACCOUNTS){
            printf("The account id must be a number between 1 and 4096!\n");
            return -1;
        }
        if(strtoul(args[1], NULL, 10) < MIN_BALANCE || strtoul(args[1], NULL, 10) > MAX_BALANCE){
            printf("The balance must be a number between 1 and 1000000000!\n");
            return -1;
        }
        if (strlen(args[2]) < MIN_PASSWORD_LEN || strlen(args[2])  > MAX_PASSWORD_LEN){
            printf("The length of the password must be between 8 and 20 characters!\n");
            return -1;
        }
    }

    if(type == OP_TRANSFER){
        if(strlen(args[0]) > WIDTH_ACCOUNT){
            printf("The account id's width must be at max 4 characters!\n");
            return -1;
        }
        if(strlen(args[1]) > WIDTH_BALANCE){
            printf("The amount to transfer width must be at max 10 characters!\n");
            return -1;
        }
        if(atoi(args[0]) < 1 || atoi(args[0]) >= MAX_BANK_ACCOUNTS){
            printf("The account id must be a number between 1 and 4096!\n");
            return -1;
        }
        if(atoi(args[1]) < 1 || strtoul(args[1], NULL, 10) > MAX_BALANCE){
            printf("The amount to transfer must be a number between 1 and 1000000000!\n");
            return -1;
        }
    }
    
    
    if(type == OP_BALANCE || type == OP_SHUTDOWN ){
        if(strlen(listofargs) != 0){
            printf("This operation accepts no arguments!\n");
            return -1;
        }
    }
    

    tlv_request_t request;
    request.type = type;

    req_header_t req_header;
    req_header.pid = getpid();
    req_header.account_id = atoi(argv[1]);
    strcpy(req_header.password, argv[2]);
    req_header.op_delay_ms = atoi(argv[3]);

    req_value_t req_value;
    req_value.header = req_header;

    if (request.type == OP_CREATE_ACCOUNT)
    {
        
        req_create_account_t create_acc;
        create_acc.account_id = atoi(args[0]);
        create_acc.balance = atoi(args[1]);
        printf("args[2] = %s\n", args[2]);
        strcpy(create_acc.password, args[2]);
        req_value.create = create_acc;
        request.length = sizeof(req_header_t) + sizeof(req_create_account_t);
        
    }
    else if (request.type == OP_TRANSFER)
    {
        
        req_transfer_t transfer;
        transfer.account_id = atoi(args[0]);
        transfer.amount = atoi(args[1]);
        req_value.transfer = transfer;
        request.length = sizeof(req_header_t) + sizeof(req_transfer_t);
        
    }
    else request.length = sizeof(req_header_t);

    request.value = req_value;

    int serverFifo = open(SERVER_FIFO_PATH, O_WRONLY | O_NONBLOCK);

    write(serverFifo, &request, sizeof(op_type_t) + sizeof(uint32_t) + request.length);

}