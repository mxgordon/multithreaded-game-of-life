/* pcthreads.cpp */
#include <iostream>
using namespace std;
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXTHREAD 10
#define MAXGRID 40
#define RANGE 1
#define ALLDONE 2
#define PARENT 0

struct msg {
    int iSender; /* sender of the message (0 .. number-of-threads) */
    int type; /* its type */
    int value1; /* first value */
    int value2; /* second value */
};

sem_t *sendsem, *readsem; /* semaphores */
msg *mailboxes;
int nThreads;  


/* iTo - mailbox to send to */
/* Msg - message to be sent */
void SendMsg(int iTo, struct msg &Msg) {
    sem_wait(&sendsem[iTo]);

    mailboxes[iTo] = Msg;

    sem_post(&readsem[iTo]);
}

/* iFrom - mailbox to receive from */
/* Msg - message structure to fill in with received message */
void RecvMsg(int iFrom, struct msg &Msg) {
    sem_wait(&readsem[iFrom]);

    Msg = mailboxes[iFrom];

    sem_post(&sendsem[iFrom]);
}

void *child_thread(void *arg) {
    int selfId = (int)(long)arg;
    msg Msg;

    do {
        RecvMsg(selfId, Msg);
    } while (Msg.type != RANGE);

    int sum = 0;
    for (int v = Msg.value1; v < Msg.value2 + 1; v++) {
        sum += v;
    }

    msg msgToSend = {
        selfId,
        ALLDONE,
        sum,
        0
    };

    SendMsg(PARENT, msgToSend);

    return(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Expected 2 arguments for thread count and number to add to\n";
        exit(1);
    }
    nThreads = atoi(argv[1]);
    if (nThreads > MAXTHREAD) {
        cout << "Number of threads cannot be more than " << MAXTHREAD << "\n";
        exit(1);
    }
    int countTo = atoi(argv[2]);


    sendsem = (sem_t*)malloc((nThreads + 1) * sizeof(sem_t));
    readsem = (sem_t*)malloc((nThreads + 1) * sizeof(sem_t));
    mailboxes = (msg*)malloc((nThreads + 1) * sizeof(msg));

    pthread_t ids[nThreads]; /* ids of threads */
    void *child_thread(void *);

    for (int i = 0; i <= nThreads; i++) {
        if (sem_init(&readsem[i], 0, 0) < 0) {
            perror("sem_init");
            exit(1);
        }
        if (sem_init(&sendsem[i], 0, 1) < 0) {
            perror("sem_init");
            exit(1);
        }
        if (i > 0 && pthread_create(&ids[i-1], NULL, child_thread, (void *)(long)i)) {
            perror("pthread_create");
            exit(1);
        }
    }

    int start = 0;
    for (int i = 0; i < nThreads; i++) {
        msg msgToSend = {
            PARENT,
            RANGE,
            start + 1,
            i == (nThreads - 1) ? countTo : (start += countTo / nThreads)
        };
        SendMsg(i + 1, msgToSend);
    }

    int sum = 0;
    for (int i = 0; i < nThreads; i++) {
        msg receivedMsg;
        do {
            RecvMsg(PARENT, receivedMsg);
        } while (receivedMsg.type != ALLDONE);
        sum += receivedMsg.value1;
    }

    cout << "The total for 1 to " << countTo << " using " << nThreads << " threads is " << sum << ".\n";


    for (int i = 0; i <= nThreads; i++) {
        (void)sem_destroy(&sendsem[i]);
        (void)sem_destroy(&readsem[i]);
        if (i < nThreads)
            (void)pthread_join(ids[i], NULL);
    }

    free(sendsem);
    free(readsem);
    free(mailboxes);
}        
