#include <iostream>
using namespace std;
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstring>

#define MAXTHREAD 32
#define MAXGRID 2048
#define RANGE 1
#define ALLDONE 2
#define GO 3
#define GENDONE 4
#define WRITEROWS 5
#define WRITEDONE 6
#define PARENT 0
#define ALLGOOD 20
#define ALLDEAD 21
#define ALLSAME 22
#define KILL 99

struct msg {
    int iSender; /* sender of the message (0 .. number-of-threads) */
    int type; /* its type */
    int value1; /* first value */
    int value2; /* second value */
};

sem_t *sendsem, *readsem; /* semaphores */
msg *mailboxes;
int nThreads;  
int nGens;
vector<vector<bool>> board;
int nRows, nCols;
bool print = false, input = false;


/* iTo - mailbox to send to */
/* Msg - message to be sent */
void SendMsg(int iTo, struct msg Msg) {
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

bool isValidPos(int row, int col) {
    return row >= 0 && col >= 0 && row < nRows && col < nCols; 
}

bool getNextState(int row, int col) {
    int touching = 0;

    if (isValidPos(row - 1, col - 1)) {
        touching += board[row - 1][col - 1];
    } if (isValidPos(row - 1, col)) {
        touching += board[row - 1][col];
    } if (isValidPos(row - 1, col + 1)) {
        touching += board[row - 1][col + 1];
    } if (isValidPos(row, col - 1)) {
        touching += board[row][col - 1];
    } if (isValidPos(row, col + 1)) {
        touching += board[row][col + 1];
    } if (isValidPos(row + 1, col - 1)) {
        touching += board[row + 1][col - 1];
    } if (isValidPos(row + 1, col)) {
        touching += board[row + 1][col];
    } if (isValidPos(row + 1, col + 1)) {
        touching += board[row + 1][col + 1];
    }  
    return board[row][col] ? touching == 2 || touching == 3 : touching == 3;
}

int waitForMsgType(int selfId, int type) {
    msg inMsg;
    do {
        RecvMsg(selfId, inMsg);
        if (inMsg.type == KILL) {
            SendMsg(PARENT, (msg){selfId, ALLDONE, 0, 0});
            pthread_exit(0);
        }
    } while (inMsg.type != type);
    return inMsg.value1;
}

void *child_thread(void *arg) {
    int selfId = (int)(long)arg;
    msg inMsg;
    int startRow, stopRow;

    do {
        RecvMsg(selfId, inMsg);
    } while (inMsg.type != RANGE);

    startRow = inMsg.value1;
    stopRow = inMsg.value2;
    vector<bool> newRows[stopRow - startRow + 1];

    for (int g = 1; g <= nGens; g++) {
        bool isDifferent = false;

        waitForMsgType(selfId, GO);  // GO to begin calculations

        for (int row = startRow; row <= stopRow; row++) {
            for (int col = 0; col < nCols; col++) {
                bool nextState = getNextState(row, col);
                newRows[row - startRow].push_back(nextState);
                isDifferent |= nextState ^ board[row][col]; // check if cell is same as previous
            }
        }

        SendMsg(PARENT, (msg){selfId, GENDONE, isDifferent ? ALLGOOD : ALLSAME, 0});  // Sync after calculations

        waitForMsgType(selfId, WRITEROWS);  // WRITEROWS to begin writing to the array

        for (int row = startRow; row <= stopRow; row++) {
                board[row] = newRows[row - startRow];
                newRows[row - startRow].clear();
        }

        if (g != nGens) {
            SendMsg(PARENT, (msg){selfId, WRITEDONE, 0, 0});  // sync after write (execept on last iteration)
        }

    }

    SendMsg(PARENT, (msg){selfId, ALLDONE, 0, 0});  // Send ALLDONE and finish

    return(NULL);
}

void parseFile(vector<vector<bool>> &board, char* filename) {
    ifstream infile(filename);
    string line;
    vector<bool> row; 

    while (getline(infile, line)) {  // iterate through the lines
        for (char c : line) {  // iteation through the lines and add elements to the array
            if (c == '0' || c == '1') {
                row.push_back(c == '1');
            }
        }
        board.push_back(row);
        row.clear();
    }
    infile.close();

    if (board.size() > MAXGRID || board[0].size() > MAXGRID) {  // check if it exceeds MAXGRID
        cout << "Maximum size allowed is " << MAXGRID << "x" << MAXGRID << "!\n";
        exit(1);
    }
}

void printBoard() {
    for (vector<bool> row : board) {
        for (bool b : row) {
            printf("%d ", b);
        }
        cout << "\n";
    }
}

void sendAllThreadsMsg(int type) {    
    for (int i = 1; i <= nThreads; i++) {
        SendMsg(i, {PARENT, type, 0, 0});
    }
}

int receiveAllThreadsMsg(int type) {
    int val = -1;
    for (int i = 1; i <= nThreads; i++) {
        int code = waitForMsgType(PARENT, type);

        if (val == -1) {  // check if they all receive the same value, otherwise return ALLGOOD
            val = code;
        } else if (val != code) {
            val = ALLGOOD;
        }
    }
    return val;
}

bool isDead() {
    for (int r = 0; r < nRows; r++) {
        for (int c = 0; c < nCols; c++) {
            if (board[r][c]) {
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (4 > argc || argc > 6) {  // verify input args
        cout << "Expected between 3 and 5 arguments for threads, file, generations, [print], and [input]\n";
        exit(1);
    } else if (argc > 4) {  // parse optional args
        print = !strcmp( argv[4], "print");

        if (argc == 6) {
            input = !strcmp( argv[5], "input");
        }
    }

    if ((nGens = atoi(argv[3])) < 1) {  // verify number of generations is correct
        cout << "Number of generations must be more than 0\n";
        exit(1);

    }

    if ((nThreads = atoi(argv[1])) > MAXTHREAD) {  // verify number of threads is correct
        cout << "Number of threads cannot be more than " << MAXTHREAD << "\n";
        exit(1);
    }

    // parse file into global variables
    parseFile(board, argv[2]);
    nRows = board.size(); 
    nCols = board[0].size(); 

    nThreads = min(nRows, nThreads);

    // Allocate memory for semaphores and mailboxes
    sendsem = (sem_t*)malloc((nThreads + 1) * sizeof(sem_t));
    readsem = (sem_t*)malloc((nThreads + 1) * sizeof(sem_t));
    mailboxes = (msg*)malloc((nThreads + 1) * sizeof(msg));
    pthread_t ids[nThreads]; /* ids of threads */

    // Initialize semaphores and threads
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

    //  --- Start synchronization code ---

    // Send range information to threads
    int start = -1;
    for (int i = 0; i < nThreads; i++) {
        msg msgToSend = {
            PARENT,
            RANGE,
            start + 1,
            i == (nThreads - 1) ? (nRows - 1) : (start += nRows / nThreads)
        };
        SendMsg(i + 1, msgToSend);
    }

    // print first gen 
    cout << "Generation 0:\n";
    printBoard();

    // Event loop
    for (int g = 1; g <= nGens; g++) {
        // wait for input if indicated
        if (input) {
            cout << "[ENTER] to continue: ";
            cin.ignore();
        }

        sendAllThreadsMsg(GO); // start calculations
        int code = receiveAllThreadsMsg(GENDONE); // calculations have stopped
        sendAllThreadsMsg(WRITEROWS); // start recording information

        if (g != nGens) {
            receiveAllThreadsMsg(WRITEDONE); // wait for threads to finish (except for the last generation where it skips this step)
        }

        // check status of the generation, send KILL message if the board didn't change or is all dead
        if (isDead() || code == ALLSAME) {
            sendAllThreadsMsg(KILL);
            nGens = g;
            break;
        } else if (print && g != nGens) {  // print board if specified and if its not the last generation
            cout << "Generation " << g << ":\n";
            printBoard();
        }
    }


    //  --- End synchronization code ---

    receiveAllThreadsMsg(ALLDONE);  // sync with ending threads
    cout << "Generation " << nGens << ":\n";
    printBoard();

    // delete the semaphores and threads
    for (int i = 0; i <= nThreads; i++) {
        (void)sem_destroy(&sendsem[i]);
        (void)sem_destroy(&readsem[i]);
        if (i < nThreads)
            (void)pthread_join(ids[i], NULL);
    }

    // free allocated memory
    free(sendsem);
    free(readsem);
    free(mailboxes);
}        
