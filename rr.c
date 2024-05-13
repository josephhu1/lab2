#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  bool start_flag;
  u32 start_execute_time;
  u32 time_left;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  struct process* prev_process = NULL;
  u32 time_elapsed = 0;

  for (size_t i = 0; i < size; i++) {
    struct process* create_process = (struct process*) malloc(sizeof(struct process));
    create_process->pid = data[i].pid;
    create_process->arrival_time = data[i].arrival_time;
    create_process->burst_time = data[i].burst_time;
    create_process->time_left = data[i].burst_time;
    create_process->start_flag = false;
    TAILQ_INSERT_TAIL(&list, create_process, pointers);
  }

  struct process_list waiting_list;
  TAILQ_INIT(&waiting_list);

  for (;;) {
    if (prev_process == NULL && TAILQ_EMPTY(&waiting_list) && TAILQ_EMPTY(&list))
      break;

    if (!TAILQ_EMPTY(&list)) {
      struct process* task;
      while(!TAILQ_EMPTY(&list)) {
        if (time_elapsed < TAILQ_FIRST(&list)->arrival_time)
          break;
        else
          task = TAILQ_FIRST(&list);
        
        TAILQ_REMOVE(&list, task, pointers);
        TAILQ_INSERT_TAIL(&waiting_list, task, pointers);
      }
    }

    if (prev_process != NULL)
      TAILQ_INSERT_TAIL(&waiting_list, prev_process, pointers);

    if (!TAILQ_EMPTY(&waiting_list)) {
      struct process* cur_process = TAILQ_FIRST(&waiting_list);
      TAILQ_REMOVE(&waiting_list, cur_process, pointers);

      if (!cur_process->start_flag) {
        cur_process->start_execute_time = time_elapsed;
        total_response_time += time_elapsed - cur_process->arrival_time;
        cur_process->start_flag = true;
      }

      if (quantum_length < cur_process->time_left) {
        cur_process->time_left -= quantum_length;
        prev_process = cur_process;
      } else {
        u32 end_time = time_elapsed + cur_process->time_left;
        total_waiting_time += end_time - cur_process->arrival_time - cur_process->burst_time;

        if (quantum_length > cur_process->time_left)
          time_elapsed -= quantum_length - cur_process->time_left;

        free(cur_process);
        prev_process = NULL;
      }
    }
    time_elapsed += quantum_length;
  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
