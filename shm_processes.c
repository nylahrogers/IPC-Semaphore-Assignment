
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

typedef struct {
    int BankAccount;
    sem_t mutex;
} SharedData;

int main() {

    // Create shared memory
    SharedData *shared = mmap(
        NULL,
        sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (shared == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // Initialize shared bank account
    shared->BankAccount = 0;

    // Initialize process-shared semaphore
    if (sem_init(&shared->mutex, 1, 1) == -1) {
        perror("sem_init failed");
        exit(1);
    }

    // Create child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    // CHILD PROCESS: POOR STUDENT
    if (pid == 0) {

        srand(time(NULL) ^ getpid());

        int localBalance = 0;

        while (1) {

            sleep(rand() % 6);

            printf("Poor Student: Attempting to Check Balance\n");

            // Safely read shared balance
            sem_wait(&shared->mutex);
            localBalance = shared->BankAccount;
            sem_post(&shared->mutex);

            int randomNumber = rand() % 2;

            if (randomNumber == 0) {

                sem_wait(&shared->mutex);

                localBalance = shared->BankAccount;

                int need = rand() % 50;

                printf("Poor Student needs $%d\n", need);

                if (need <= localBalance) {

                    localBalance -= need;

                    printf("Poor Student: Withdraws $%d / Balance = $%d\n",
                           need, localBalance);

                } else {

                    printf("Poor Student: Not Enough Cash ($%d)\n",
                           localBalance);
                }

                shared->BankAccount = localBalance;

                sem_post(&shared->mutex);

            } else {

                printf("Poor Student: Last Checking Balance = $%d\n",
                       localBalance);
            }

            fflush(stdout);
        }
    }

    // PARENT PROCESS: DEAR OLD DAD
    else {

        srand(time(NULL) ^ getpid());

        int localBalance = 0;

        while (1) {

            sleep(rand() % 6);

            printf("Dear Old Dad: Attempting to Check Balance\n");

            // Safely read shared balance
            sem_wait(&shared->mutex);
            localBalance = shared->BankAccount;
            sem_post(&shared->mutex);

            int randomNumber = rand() % 2;

            if (randomNumber == 0) {

                if (localBalance < 100) {

                    sem_wait(&shared->mutex);

                    localBalance = shared->BankAccount;

                    int amount = rand() % 100;

                    int depositChance = rand() % 2;

                    if (depositChance == 0) {

                        localBalance += amount;

                        printf("Dear old Dad: Deposits $%d / Balance = $%d\n",
                               amount, localBalance);

                        shared->BankAccount = localBalance;

                    } else {

                        printf("Dear old Dad: Doesn't have any money to give\n");
                    }

                    sem_post(&shared->mutex);

                } else {

                    printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n",
                           localBalance);
                }

            } else {

                printf("Dear Old Dad: Last Checking Balance = $%d\n",
                       localBalance);
            }

            fflush(stdout);
        }
    }

    return 0;
}
