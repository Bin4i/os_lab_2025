#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

static void *thread_a(void *arg) {
  (void)arg;
  printf("A: пытаюсь захватить м1\n");
  pthread_mutex_lock(&m1);
  printf("A: захватил м1\n");

  sleep(1);

  printf("A: пытаюсь захватить м2\n");
  pthread_mutex_lock(&m2);
  printf("A: захватил м2\n");

  pthread_mutex_unlock(&m2);
  pthread_mutex_unlock(&m1);
  printf("A: завершился\n");
  return NULL;
}

static void *thread_b(void *arg) {
  (void)arg;
  printf("Б: пытаюсь захватить м2\n");
  pthread_mutex_lock(&m2);
  printf("Б: захватил м2\n");

  sleep(1);

  printf("Б: пытаюсь захватить м1\n");
  pthread_mutex_lock(&m1);
  printf("Б: захватил м1\n");

  pthread_mutex_unlock(&m1);
  pthread_mutex_unlock(&m2);
  printf("Б: завершился\n");
  return NULL;
}

int main(void) {
  pthread_t ta, tb;

  pthread_create(&ta, NULL, thread_a, NULL);
  pthread_create(&tb, NULL, thread_b, NULL);

  pthread_join(ta, NULL);
  pthread_join(tb, NULL);

  printf("main: оба потока завершились\n");
  return 0;
}
