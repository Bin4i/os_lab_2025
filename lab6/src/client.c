#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "common.h"

struct Server {
  char ip[255];
  int port;
};

// Структура для передачи данных в поток
struct ThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

// Функция, которая выполняется в отдельном потоке для каждого сервера
void *ThreadWorker(void *arg) {
  struct ThreadArgs *targs = (struct ThreadArgs *)arg;
  
  struct hostent *hostname = gethostbyname(targs->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", targs->server.ip);
    targs->result = 1; // Возвращаем нейтральный элемент, если сервер упал
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(targs->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    targs->result = 1;
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", targs->server.ip, targs->server.port);
    close(sck);
    targs->result = 1;
    return NULL;
  }

  // Формируем задачу
  char task[sizeof(uint64_t) * 3];
  memcpy(task, &targs->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &targs->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &targs->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    close(sck);
    targs->result = 1;
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed\n");
    close(sck);
    targs->result = 1;
    return NULL;
  }

  memcpy(&targs->result, response, sizeof(uint64_t));
  close(sck);
  
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // Чтение файла с серверами
  FILE *file = fopen(servers, "r");
  if (!file) {
    fprintf(stderr, "Could not open servers file: %s\n", servers);
    return 1;
  }

  struct Server *to = malloc(sizeof(struct Server) * 100); // макс 100 серверов
  unsigned int servers_num = 0;
  char line[255];

  while (fgets(line, sizeof(line), file)) {
    // Убираем символ переноса строки
    line[strcspn(line, "\r\n")] = 0;
    if (strlen(line) == 0) continue;

    char *colon = strchr(line, ':');
    if (colon != NULL) {
      *colon = '\0';
      strncpy(to[servers_num].ip, line, 254);
      to[servers_num].port = atoi(colon + 1);
      servers_num++;
    }
  }
  fclose(file);

  if (servers_num == 0) {
    fprintf(stderr, "No servers found in file!\n");
    free(to);
    return 1;
  }

  printf("Loaded %u servers\n", servers_num);

  // Распределение работы между серверами
  pthread_t threads[servers_num];
  struct ThreadArgs args[servers_num];
  
  uint64_t total_range = k; // от 1 до k
  uint64_t chunk = total_range / servers_num;
  uint64_t remainder = total_range % servers_num;
  uint64_t current = 1;

  for (unsigned int i = 0; i < servers_num; i++) {
    args[i].server = to[i];
    args[i].begin = current;
    
    uint64_t my_size = chunk + (i < remainder ? 1 : 0);
    args[i].end = current + my_size - 1;
    
    // Если серверов больше, чем чисел, отправляем пустой диапазон
    if (my_size == 0) {
      args[i].begin = 1;
      args[i].end = 0;
    }
    
    args[i].mod = mod;
    args[i].result = 1;
    current = args[i].end + 1;

    // Запускаем поток для каждого сервера
    if (pthread_create(&threads[i], NULL, ThreadWorker, (void *)&args[i])) {
      fprintf(stderr, "Error: pthread_create failed!\n");
      return 1;
    }
  }

  // Собираем результаты
  uint64_t final_answer = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    printf("Server %s:%d returned: %llu\n", args[i].server.ip, args[i].server.port, (unsigned long long)args[i].result);
    final_answer = MultModulo(final_answer, args[i].result, mod);
  }

  printf("Final answer: %llu\n", (unsigned long long)final_answer);

  free(to);
  return 0;
}
