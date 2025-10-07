#include "threading.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Optional: use these functions to add debug or error prints to your
// application
#define DEBUG_LOG(msg, ...)
// #define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("threading ERROR: " msg "\n", ##__VA_ARGS__)

#define DEBUG

void *threadfunc(void *thread_param) {

  // TODO: wait, obtain mutex, wait, release mutex as described by thread_data
  // structure hint: use a cast like the one below to obtain thread arguments
  // from your parameter

#ifdef DEBUG /* ifdef DEBUG  */
  DEBUG_LOG("Create thread_data struct with passed thread parameters");
#endif
  struct thread_data *thread_func_args = (struct thread_data *)thread_param;
  if (thread_func_args == NULL) {
    ERROR_LOG("passed empty thread params");
    return thread_param;
  }
#ifdef DEBUG /* ifdef DEBUG */
  DEBUG_LOG("Set thread thread_complete_success to false");
#endif
  int rc;
  thread_func_args->thread_complete_success = false;

// wait
#ifdef DEBUG /* ifdef DEBUG */
  DEBUG_LOG("wait to obtain mutex");
#endif
  if (usleep(thread_func_args->wait_to_obtain_ms * 1000) != 0) {
    ERROR_LOG("failed on wait to obtain mutex");
    return thread_param;
  }

  // obtain mutex
#ifdef DEBUG /* ifdef DEBUG */
  DEBUG_LOG("obtain mutex");
#endif
  rc = pthread_mutex_lock(thread_func_args->mutex);
  if (rc != 0) {
    ERROR_LOG("mutex lock failed: %d", rc);
    return thread_param;
  }

// wait to release mutex
#ifdef DEBUG /* ifdef DEBUG */
  DEBUG_LOG("wait to release mutex");
#endif
  if (usleep(thread_func_args->wait_to_release_ms * 1000) != 0) {
    ERROR_LOG("failed on wait to release mutex");
    return thread_param;
  }

// release mutex
#ifdef DEBUG
  DEBUG_LOG("release mutex");
#endif /* ifdef DEBUG */
  rc = pthread_mutex_unlock(thread_func_args->mutex);
  if (rc != 0) {
    ERROR_LOG("failed to release mutex: %d", rc);
    return thread_param;
  }

#ifdef DEBUG
  DEBUG_LOG("thread completed successfully");
#endif /* ifdef DEBUG */
  thread_func_args->thread_complete_success = true;
  return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
                                  int wait_to_obtain_ms,
                                  int wait_to_release_ms) {
  /**
   * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass
   * thread_data to created thread using threadfunc() as entry point.
   *
   * return true if successful.
   *
   * See implementation details in threading.h file comment block
   */

  if (thread == NULL || mutex == NULL) {
    ERROR_LOG("start thread passed empty thread or mutex");
    return false;
  }

#ifdef DEBUG
  DEBUG_LOG("allocate memory for thread params");
#endif /* ifdef DEBUG */
  struct thread_data *thread_param = malloc(sizeof(struct thread_data));
  if (thread_param == NULL) {
    ERROR_LOG("failed to allocate memory for thread params");
    return false;
  }

#ifdef DEBUG
  DEBUG_LOG("set thread params");
#endif /* ifdef DEBUG */
  thread_param->mutex = mutex;
  thread_param->thread_complete_success = false;
  thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
  thread_param->wait_to_release_ms = wait_to_release_ms;

#ifdef DEBUG
  DEBUG_LOG("create thread");
#endif /* ifdef DEBUG */
  int rc = pthread_create(thread, (void *)0, threadfunc, thread_param);
  if (rc != 0) {
    ERROR_LOG("failed to create thread: %d", rc);
    free(thread_param);
    return false;
  }

  return true;
}
