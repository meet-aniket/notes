// This repl has the code to impose total order of acquiring lock.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_RECORDS 3
#define NUM_CONN 6

typedef struct {
  int attrs[3];
} RecordData;

typedef struct {
  RecordData data;
  pthread_mutex_t lock;
} Record;

typedef struct {
  Record record[NUM_RECORDS];
} Database;

Database db;

void initDB() {
  for (int i = 0; i < NUM_RECORDS; i++) {
    db.record[i].data.attrs[0] = i;                // id
    db.record[i].data.attrs[1] = rand() % 20 + 10; // age
    pthread_mutex_init(&db.record[i].lock, NULL);
  }
}

void acquireLock(int *txn, int recIdx) {
  printf("txn %c: wants to acquire lock on record: %d\n", *txn, recIdx);
  pthread_mutex_lock(&db.record[recIdx].lock);
  printf("txn %c: acquired lock on record: %d\n", *txn, recIdx);
}

void releaseLock(char *txn, int recIdx) {
  pthread_mutex_unlock(&db.record[recIdx].lock);
  printf("txn %c: released lock on record: %d\n", *txn, recIdx);
}

void *mimic_load(void *arg) {
  char *tname = (char *)arg;

  // mimics a transaction
  while (1) {
    // local two records randomly
    int rec1 = rand() % NUM_RECORDS;
    int rec2 = rand() % NUM_RECORDS;

    if (rec1 == rec2) {
      continue;
    }

    // Swapping is used here to impose total order of acquiring the lock.
    if (rec1 > rec2) {
      int temp = rec1;
      rec1 = rec2;
      rec2 = temp;
    }

    acquireLock(tname, rec1);
    acquireLock(tname, rec2);

    sleep(2);

    releaseLock(tname, rec2);
    releaseLock(tname, rec1);

    sleep(1);
  }

  pthread_exit(NULL);
}

int main() {
  srand(time(NULL));
  initDB();

  pthread_t threads[NUM_CONN];
  for (int i = 0; i < NUM_CONN; i++) {
    char *txn = (char *)malloc(1 * sizeof(char));
    txn[0] = i + 'A';
    pthread_create(&threads[i], NULL, mimic_load, txn);
  }

  for (int i = 0; i < NUM_CONN; i++) {
    pthread_join(threads[i], NULL);
  }

  for (int i = 0; i < NUM_RECORDS; i++) {
    pthread_mutex_destroy(&db.record[i].lock);
  }
  return 0;
}