#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  long long start;
  long long end;
  long long mod;
  long long local;
} task_t;

/* Общий результат и мьютекс для его защиты */
static long long g_result = 1;
static pthread_mutex_t g_mut = PTHREAD_MUTEX_INITIALIZER;

static void *worker(void *arg) {
  task_t *t = (task_t *)arg;

  long long local = 1 % t->mod;   /* на случай mod == 1 */
  for (long long i = t->start; i <= t->end; i++) {
    local = (local * (i % t->mod)) % t->mod;
  }
  t->local = local;

  /* Критическая секция: обновляем общий результат */
  pthread_mutex_lock(&g_mut);
  g_result = (g_result * local) % t->mod;
  pthread_mutex_unlock(&g_mut);

  return NULL;
}

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s -k <k> --pnum=<N> --mod=<M>\n"
          "  -k <k>        number whose factorial to compute\n"
          "  --pnum=<N>    number of threads\n"
          "  --mod=<M>     modulus\n",
          prog);
}

int main(int argc, char **argv) {
  long long k = -1, mod = -1;
  int pnum = -1;

  /* --- Разбор аргументов --- */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-k") == 0) {
      if (i + 1 >= argc) { usage(argv[0]); return 1; }
      k = atoll(argv[++i]);
    } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
      pnum = atoi(argv[i] + 7);
    } else if (strncmp(argv[i], "--mod=", 6) == 0) {
      mod = atoll(argv[i] + 6);
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      usage(argv[0]);
      return 1;
    }
  }

  if (k < 0 || pnum <= 0 || mod <= 0) {
    fprintf(stderr, "Invalid arguments.\n");
    usage(argv[0]);
    return 1;
  }

  /* --- Особые случаи --- */
  if (mod == 1) {
    /* всё по модулю 1 = 0 */
    printf("%lld! mod %lld = 0\n", k, mod);
    return 0;
  }
  if (k == 0 || k == 1) {
    printf("%lld! mod %lld = %lld\n", k, mod, 1 % mod);
    return 0;
  }

  if ((long long)pnum > k) pnum = (int)k;

  /* --- Готовим задачи для потоков --- */
  pthread_t *threads = calloc(pnum, sizeof(pthread_t));
  task_t *tasks = calloc(pnum, sizeof(task_t));
  if (!threads || !tasks) {
    perror("calloc");
    free(threads); free(tasks);
    return 1;
  }

  long long chunk = k / pnum;
  long long rem = k % pnum;
  long long cur = 1;

  for (int i = 0; i < pnum; i++) {
    long long size = chunk + (i < rem ? 1 : 0);
    tasks[i].start = cur;
    tasks[i].end = cur + size - 1;
    tasks[i].mod = mod;
    tasks[i].local = 1 % mod;
    cur = tasks[i].end + 1;

    if (pthread_create(&threads[i], NULL, worker, &tasks[i]) != 0) {
      perror("pthread_create");
      /* дождёмся уже созданных */
      for (int j = 0; j < i; j++) pthread_join(threads[j], NULL);
      free(threads); free(tasks);
      return 1;
    }
  }

  for (int i = 0; i < pnum; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("pthread_join");
      free(threads); free(tasks);
      return 1;
    }
  }

  printf("%lld! mod %lld = %lld\n", k, mod, g_result);

  free(threads);
  free(tasks);
  return 0;
}
