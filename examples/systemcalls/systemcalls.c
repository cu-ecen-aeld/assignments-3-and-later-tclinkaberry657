#include <stdarg.h>
#include <stdbool.h>
#include <sys/syslog.h>
#define _XOPEN_SOURCE
#include "systemcalls.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */
bool do_system(const char *cmd) {

  /*
   * TODO  add your code here
   *  Call the system() function with the command set in the cmd
   *   and return a boolean true if the system() call completed with success
   *   or false() if it returned a failure
   */

  // Return failed execution if the command passed is empty
  if (cmd == NULL) {
    syslog(LOG_ERR, "Error: empty command");
    return false;
  }

  // Perform system call to command passed to function
  int ret = system(cmd);
  if (ret == -1) { // return unsuccessful if error executig system call
    syslog(LOG_ERR, "Error with sys call: %s", strerror(errno));
    return false;
  }

  // return error if child process failed to call exit;
  if (!WIFEXITED(ret)) {
    syslog(LOG_ERR, "Error exiting command %s: %s", cmd, strerror(errno));
    return false;
  }

  // error if there was an error executing the command
  if (WIFEXITED(ret) && WEXITSTATUS(ret)) {
    syslog(LOG_ERR, "Error executing child process %s: %s", cmd,
           strerror(errno));
    return false;
  }

  syslog(LOG_INFO, "Successfully executed command %s", cmd);
  return true;
}

/**
 * @param count -The numbers of variables passed to the function. The variables
 * are command to execute. followed by arguments to pass to the command Since
 * exec() does not perform path expansion, the command to execute needs to be an
 * absolute path.
 * @param ... - A list of 1 or more arguments after the @param count argument.
 *   The first is always the full path to the command to execute with execv()
 *   The remaining arguments are a list of arguments to pass to the command in
 * execv()
 * @return true if the command @param ... with arguments @param arguments were
 * executed successfully using the execv() call, false if an error occurred,
 * either in invocation of the fork, waitpid, or execv() command, or if a
 * non-zero return value was returned by the command issued in @param arguments
 * with the specified arguments.
 */

bool do_exec(int count, ...) {
  va_list args;
  va_start(args, count);
  char *command[count + 1];
  int i;
  for (i = 0; i < count; i++) {
    command[i] = va_arg(args, char *);
  }
  command[count] = NULL;

  /*
   * TODO:
   *   Execute a system command by calling fork, execv(),
   *   and wait instead of system (see LSP page 161).
   *   Use the command[0] as the full path to the command to execute
   *   (first argument to execv), and use the remaining arguments
   *   as second argument to the execv() command.
   *
   */

  // create child process
  int status;
  pid_t pid = fork();

  // return error if error creating child process
  if (pid == -1) {
    syslog(LOG_ERR, "Error creating child process: %s", strerror(errno));
    va_end(args);
    return false;

  } else if (pid == 0) { // child process
    execv(command[0], command);
    syslog(LOG_ERR, "Error with execv: %s", strerror(errno));
    exit(-1); // exit child process with error
    //
  } else { // parent process
    if (waitpid(pid, &status, 0) == -1) {
      syslog(LOG_ERR, "Error with waitpid: %s", strerror(errno));
      va_end(args);
      return false;
    }

    if (!WIFEXITED(status)) {
      syslog(LOG_ERR, "Error exiting process: %s", strerror(errno));
      va_end(args);
      return false;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status)) {
      syslog(LOG_ERR, "Error executing process: %s", strerror(errno));
      va_end(args);
      return false;
    }
  }

  syslog(LOG_INFO, "Successfully executed command %s", command[0]);
  va_end(args);
  return true;
}

/**
 * @param outputfile - The full path to the file to write with command output.
 *   This file will be closed at completion of the function call.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...) {
  va_list args;
  va_start(args, count);
  char *command[count + 1];
  int i;
  for (i = 0; i < count; i++) {
    command[i] = va_arg(args, char *);
  }
  command[count] = NULL;

  /*
   * TODO
   *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624
   * as a refernce, redirect standard out to a file specified by outputfile. The
   * rest of the behaviour is same as do_exec()
   *
   */

  int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd == -1) {
    syslog(LOG_ERR, "Error opening file %s: %s", outputfile,
           strerror(errno)); // log error if error opening
    return false;
  }

  int status;
  pid_t pid = fork();
  if (pid == -1) {
    syslog(LOG_ERR, "Error creating child process: %s", strerror(errno));
    va_end(args);
    return false;

  } else if (pid == 0) {
    if (dup2(fd, STDOUT_FILENO) < 0) {
      syslog(LOG_ERR, "Failed on dup2: %s", strerror(errno));
      close(fd);
      exit(-1);
    }
    close(fd);

    execv(command[0], command);
    syslog(LOG_ERR, "Error with execv: %s", strerror(errno));
    exit(-1);
  } else {
    if (waitpid(pid, &status, 0) == -1) {
      syslog(LOG_ERR, "Error with waitpid: %s", strerror(errno));
      va_end(args);
      return false;
    }

    if (!WIFEXITED(status)) {
      syslog(LOG_ERR, "Error exiting process: %s", strerror(errno));
      va_end(args);
      return false;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status)) {
      syslog(LOG_ERR, "Error executing process: %s", strerror(errno));
      va_end(args);
      return false;
    }
  }

  syslog(LOG_INFO, "Successfully executed command %s", command[0]);
  va_end(args);
  return true;
}
