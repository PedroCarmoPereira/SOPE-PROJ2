#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include "hashsum.h"
#include <fcntl.h>
#include <semaphore.h>
#include "string.h"
#include "requestQueue.h"
#include "constants.h"
#include "types.h"
#include "sope.h"

pthread_t bankoffice[MAX_BANK_OFFICES];
pthread_mutex_t mutex_queue;
account_mutex_t bankaccounts[MAX_BANK_ACCOUNTS];
sem_t full, empty;
bool terminate = false;
uint32_t activeThreads = 0;
int server_log_file;
int thread_count = 0;    
int serverFifo;

#define SHARED 0

bool verifyAccount(u_int32_t id, char* password){
    char hash[HASH_LEN + 1];
    hashGenerator(bankaccounts[id].account.salt, password, hash);
    if (strcmp(hash, bankaccounts[id].account.hash) == 0)
        return true;
    return false;
}

void saltGenerator(char *salt)
{

    char randomSalt[] = "0123456789abcdef";
    for (int i = 0; i < SALT_LEN; i++)
    {
        salt[i] = randomSalt[rand() % (strlen(randomSalt))];
    }

    salt[SALT_LEN] = '\0';
}

ret_code_t create_account(req_value_t rval)
{

    if (!verifyAccount(rval.header.account_id, rval.header.password)) return RC_LOGIN_FAIL;

    if (rval.header.account_id != ADMIN_ACCOUNT_ID) return RC_OP_NALLOW;

    usleep(rval.header.op_delay_ms);
    logSyncDelay(server_log_file, pthread_self(), rval.header.account_id, rval.header.op_delay_ms);
    account_mutex_t newMut;
    if (bankaccounts[rval.create.account_id].account.hash[0] == '\0')
    {   
        newMut.account.account_id = rval.create.account_id;
        newMut.account.balance = rval.create.balance;
        saltGenerator(newMut.account.salt);
        hashGenerator(newMut.account.salt, rval.create.password, newMut.account.hash);
        pthread_mutex_init(&newMut.mutex, NULL);
        bankaccounts[newMut.account.account_id] = newMut;
        logAccountCreation(server_log_file, MAIN_THREAD_ID, &newMut.account);
        return RC_OK;
    }
    else return RC_ID_IN_USE;
}

ret_code_t check_balance(req_value_t rval, int * ptr)
{   
    if (!verifyAccount(rval.header.account_id, rval.header.password)) return RC_LOGIN_FAIL;

    if (rval.header.account_id == ADMIN_ACCOUNT_ID) return RC_OP_NALLOW;

    pthread_mutex_lock(&bankaccounts[rval.header.account_id].mutex);
    logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, rval.header.pid);
    usleep(rval.header.op_delay_ms);
    logSyncDelay(server_log_file, pthread_self(), rval.header.account_id, rval.header.op_delay_ms);
    *ptr = bankaccounts[rval.header.account_id].account.balance;
    pthread_mutex_unlock(&bankaccounts[rval.header.account_id].mutex);
    logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, rval.header.pid);
    return RC_OK;
}

ret_code_t transfer_operation(req_value_t rval, int * ptr)
{
    if (!verifyAccount(rval.header.account_id, rval.header.password)) return RC_LOGIN_FAIL;

    if(rval.transfer.account_id == rval.header.account_id) return RC_SAME_ID;

    if (rval.header.account_id == ADMIN_ACCOUNT_ID) return RC_OP_NALLOW;

    if (bankaccounts[rval.transfer.account_id].account.hash[0] == '\0') return RC_ID_NOT_FOUND;

    pthread_mutex_lock(&bankaccounts[rval.header.account_id].mutex);
    pthread_mutex_lock(&bankaccounts[rval.transfer.account_id].mutex);
    logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, rval.header.pid);
    logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, rval.header.pid);
    usleep(rval.header.op_delay_ms);
    logSyncDelay(server_log_file, pthread_self(), rval.header.account_id, rval.header.op_delay_ms);

    if(rval.transfer.amount > bankaccounts[rval.header.account_id].account.balance){
        pthread_mutex_unlock(&bankaccounts[rval.header.account_id].mutex);
        pthread_mutex_unlock(&bankaccounts[rval.transfer.account_id].mutex);
        return RC_NO_FUNDS;
    }

    if (bankaccounts[rval.transfer.account_id].account.balance + rval.transfer.amount > MAX_BALANCE){
        pthread_mutex_unlock(&bankaccounts[rval.header.account_id].mutex);
        pthread_mutex_unlock(&bankaccounts[rval.transfer.account_id].mutex);
        return RC_TOO_HIGH;
    }

    bankaccounts[rval.header.account_id].account.balance -= rval.transfer.amount;
    bankaccounts[rval.transfer.account_id].account.balance += rval.transfer.amount;
    *ptr = bankaccounts[rval.header.account_id].account.balance;

    pthread_mutex_unlock(&bankaccounts[rval.header.account_id].mutex);
    pthread_mutex_unlock(&bankaccounts[rval.transfer.account_id].mutex);
    logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, rval.header.pid);
    logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, rval.header.pid);
    return RC_OK;
}

ret_code_t terminationRequest(req_value_t rval)
{
    if (!verifyAccount(rval.header.account_id, rval.header.password))
        return RC_LOGIN_FAIL;

    if (rval.header.account_id != ADMIN_ACCOUNT_ID)
        return RC_OP_NALLOW;
    
    else {
        terminate = true;
        for (unsigned int i = 0; i < activeThreads; i++){ 
            int val;
            sem_getvalue(&full, &val);
            logSyncMechSem(server_log_file, pthread_self(), SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, rval.header.pid, val);
            sem_post(&full);
        }
        usleep(rval.header.op_delay_ms);
        logDelay(server_log_file, pthread_self(), rval.header.op_delay_ms);
        fchmod(serverFifo, 0600);
        return RC_OK;
    }
}

void *officeprocessing(void *requestQueue)
{
    int x = thread_count;
    logBankOfficeOpen(server_log_file, x, pthread_self());
    while (!(terminate && ((reqQ_t *) requestQueue)->front != NULL)){
        sem_wait(&full);
        int val;
        sem_getvalue(&full, &val);
        logSyncMechSem(server_log_file, pthread_self(), SYNC_OP_SEM_WAIT, SYNC_ROLE_CONSUMER, getpid() , val);
        if(((reqQ_t *) requestQueue)->front == NULL && terminate) {
            sem_post(&full);
            sem_getvalue(&full, &val);
            logSyncMechSem(server_log_file, pthread_self(), SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, getpid() , val);
            sem_post(&empty);
            sem_getvalue(&empty, &val);
            logSyncMechSem(server_log_file, pthread_self(), SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, getpid() , val);
            break;
        }

        pthread_mutex_lock(&mutex_queue);
        logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_LOCK, SYNC_ROLE_CONSUMER, getpid()); // HERE??

        if(((reqQ_t *) requestQueue)->front == NULL) continue;
        activeThreads++;
        reqQ_node_t *node = deQueue((reqQ_t *)requestQueue);

        logRequest(server_log_file, x, &node->key);

        pthread_mutex_unlock(&mutex_queue);
        logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, getpid());  // HERE??

        tlv_reply_t reply;
        char str_pid[WIDTH_ID + 1];
        char fifo_name[USER_FIFO_PATH_LEN + 1];
        sprintf(str_pid, "%d", node->key.value.header.pid);
        strcpy(fifo_name, USER_FIFO_PATH_PREFIX);
        strcat(fifo_name, str_pid);
        int user_fifo = open(fifo_name, O_WRONLY);
        int bal = 0;
        switch (node->key.type)
        {
        case 0:
            reply.value.header.ret_code = create_account(node->key.value);
            reply.length = sizeof(rep_header_t);
            reply.type = OP_CREATE_ACCOUNT;
            reply.value.header.account_id = node->key.value.create.account_id;
            break;
        case 1:
            reply.value.header.ret_code = check_balance(node->key.value, &bal);
            reply.length = sizeof(rep_header_t) + sizeof(rep_balance_t);
            reply.type = OP_BALANCE;
            reply.value.header.account_id = node->key.value.header.account_id;
            reply.value.balance.balance = bal;
            break;
        case 2:
            reply.value.header.ret_code = transfer_operation(node->key.value, &bal);
            reply.length = sizeof(rep_header_t) + sizeof(rep_transfer_t);
            reply.type = OP_TRANSFER;
            reply.value.header.account_id = node->key.value.header.account_id;
            reply.value.transfer.balance = bal;
            break;
        case 3:
            reply.value.header.ret_code = terminationRequest(node->key.value);
            reply.length = sizeof(rep_header_t) + sizeof(rep_shutdown_t);
            reply.type = OP_SHUTDOWN;
            reply.value.header.account_id = node->key.value.header.account_id;
            sem_getvalue(&full, &val);
            reply.value.shutdown.active_offices = val;
            break;
        default:
            break;
        }
        free(node);
        activeThreads--;
        if(user_fifo == -1){
            reply.value.header.ret_code = RC_USR_DOWN;
            logReply(server_log_file, pthread_self(), &reply);
        }
        else logReply(server_log_file, pthread_self(), &reply);
        write(user_fifo, &reply, sizeof(op_type_t) + sizeof(uint32_t) + reply.length);
        sem_post(&empty);
        sem_getvalue(&empty, &val);
        logSyncMechSem(server_log_file, pthread_self(), SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, getpid() , val);
    }
    logBankOfficeClose(server_log_file, x, pthread_self());
    pthread_exit(0);
}

void create_admin_acc(char *password)
{
    bankaccounts[ADMIN_ACCOUNT_ID].account.account_id = ADMIN_ACCOUNT_ID;
    bankaccounts[ADMIN_ACCOUNT_ID].account.balance = 0;
    saltGenerator(bankaccounts[ADMIN_ACCOUNT_ID].account.salt);
    hashGenerator(bankaccounts[ADMIN_ACCOUNT_ID].account.salt, password, bankaccounts[ADMIN_ACCOUNT_ID].account.hash);
}



int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        printf("Wrong number of args!\n");
        return -1;
    }

    if (atoi(argv[1]) > 99)
    {
        printf("The number of bank offices must be at max 99!\n");
        return -1;
    }

    if (strlen(argv[2]) < MIN_PASSWORD_LEN || strlen(argv[2])  > MAX_PASSWORD_LEN){
        printf("The length of the password must be between 8 and 20 characters!\n");
        return -1;
    }

    create_admin_acc(argv[2]);
    server_log_file = open(SERVER_LOGFILE, O_CREAT | O_WRONLY | O_APPEND, 0666);
    sem_init(&empty, SHARED, atoi(argv[1]));
    logSyncMechSem(server_log_file, MAIN_THREAD_ID, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, getpid(), atoi(argv[1]));
    sem_init(&full, SHARED, 0);
    logSyncMechSem(server_log_file, MAIN_THREAD_ID, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, getpid(), 0);
    reqQ_t *requestQueue = createQueue();

    if (mkfifo(SERVER_FIFO_PATH, 0666) == -1)
    {
        return -1;
    }

    serverFifo = open(SERVER_FIFO_PATH, O_RDONLY | O_NONBLOCK);

    int dummyFifo = open(SERVER_FIFO_PATH, O_WRONLY);
    if (dummyFifo == -1)
        return -2;

    pthread_mutex_init(&mutex_queue, NULL);  

    for(thread_count = 0; thread_count <= atoi(argv[1]); thread_count++) pthread_create(&bankoffice[thread_count], NULL, officeprocessing, requestQueue);
    tlv_request_t request;
    request.type = 0;
    do
    {
        int bytesRead;
        bytesRead = read(serverFifo, &request.type, sizeof(op_type_t));
        if (bytesRead > 0)
        {
            int getVal;
            sem_wait(&empty);
            sem_getvalue(&empty, &getVal);
            logSyncMechSem(server_log_file, MAIN_THREAD_ID, SYNC_OP_SEM_WAIT, SYNC_ROLE_PRODUCER, getpid(),getVal);
            read(serverFifo, &request.length, sizeof(uint32_t));
            read(serverFifo, &request.value, request.length);
            logRequest(server_log_file, MAIN_THREAD_ID, &request);

            pthread_mutex_lock(&mutex_queue);
            logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, getpid()); // HERE??

            enQueue(requestQueue, request);

            pthread_mutex_unlock(&mutex_queue);
            logSyncMech(server_log_file, pthread_self(), SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_PRODUCER, getpid()); // HERE??

            sem_post(&full);
            sem_getvalue(&full, &getVal);
            logSyncMechSem(server_log_file, MAIN_THREAD_ID, SYNC_OP_SEM_POST, SYNC_ROLE_PRODUCER, getpid(),getVal);
        }
    } while (!terminate);

    for(int i = 0; i <= server_log_file; i++) pthread_join(bankoffice[i], NULL);
    close(serverFifo);
    close(dummyFifo);
    unlink(SERVER_FIFO_PATH);
    return 0;
}