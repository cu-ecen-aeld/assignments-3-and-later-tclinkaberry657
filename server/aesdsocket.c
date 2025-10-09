#include <stddef.h>
#include <sys/syslog.h>
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUF_SIZE 1024
#define BACKLOG 10

#define IPV6
// #define IPV4

#ifdef IPV4
#define DOMAIN PF_INET
#define IP_ADDR_LEN INET_ADDRSTRLEN
#endif /* ifdef IPV4  */

#ifdef IPV6
#define DOMAIN PF_INET6
#define IP_ADDR_LEN INET6_ADDRSTRLEN
#endif /* ifdef IPV6 */

#define SOCK_TYPE SOCK_STREAM
// #define SOCK_TYPE SOCK_DGRAM

#define PROTOCOL 0

#define AI_FLAGS AI_PASSIVE
// #define AI_FAMILY AF_UNSPEC

int sock_fd = -1;
int shutdown_flag = 0;
int waiting_for_connection = 0;

void signal_handler(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    syslog(LOG_INFO, "Caught signal, exiting: %d", sig);
    shutdown_flag = 1;
    if (waiting_for_connection) {
      close(sock_fd);
      exit(0);
    }
  }
}

int write_to_data_file(const char *file, const char *writestr, ssize_t len) {
  int fd;
  ssize_t write_size;
  fd = open(file, O_RDWR | O_CREAT | O_APPEND | O_SYNC, 0664);
  if (fd == -1) {
    syslog(LOG_ERR, "Error opening data file: %s", strerror(errno));
    return -1;
  }

  write_size = write(fd, writestr, len);
  if (write_size == -1) {
    syslog(LOG_ERR, "Error writing to data file: %s", strerror(errno));
    close(fd);
    return -1;
  }

  if (close(fd) == -1) {
    syslog(LOG_ERR, "Error closing data file: %s", strerror(errno));
    return -1;
  }

  return write_size;
}

void start_daemon(void) {
  pid_t pid;

  pid = fork();
  if (pid < 0) {
    syslog(LOG_ERR, "Daemon failed to fork: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  umask(0);

  if (setsid() < 0) {
    syslog(LOG_ERR, "Failed starting new session: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (chdir("/") < 0) {
    syslog(LOG_ERR, "Failed changing directory: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

int main(int argc, char *argv[]) {
  openlog("aesdsocket", LOG_PID, LOG_USER);

  int run_as_daemon = 0;
  if (argc == 2 && strcmp(argv[1], "-d") == 0) {
    run_as_daemon = 1;
  } else if (argc > 1) {
    syslog(LOG_ERR, "usage: %s [-d]", argv[0]);
    return -1;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  struct sockaddr_storage client_addr;
  socklen_t addr_size;
  struct addrinfo hints, *res, *p;
  int client_fd;
  char client_ip[IP_ADDR_LEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_FLAGS;
  hints.ai_socktype = SOCK_TYPE;
  hints.ai_family = AF_UNSPEC;

  int status;
  if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
    syslog(LOG_ERR, "getaddrinfo error: %s", strerror(errno));
    return -1;
  }

  for (p = res; p != NULL; p = p->ai_next) {

    sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sock_fd == -1) {
      syslog(LOG_ERR, "Failed opening socket: %s", strerror(errno));
      continue;
    }

    if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      syslog(LOG_ERR, "binding socket: %s", strerror(errno));
      close(sock_fd);
      continue;
    }
    break;
  }

  freeaddrinfo(res);
  if (p == NULL) {
    syslog(LOG_ERR, "Failed binding to socket");
    return -1;
  }

  if (run_as_daemon) {
    start_daemon();
  }

  if (listen(sock_fd, BACKLOG) == -1) {
    syslog(LOG_ERR, "Listen error: %s", strerror(errno));
    close(sock_fd);
    return -1;
  }

  while (!shutdown_flag) {

    waiting_for_connection = 1;
    addr_size = sizeof(client_addr);
    client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_size);
    if (client_fd == -1) {
      syslog(LOG_ERR, "Error accepting connection: %s", strerror(errno));
      close(sock_fd);
      return -1;
    }
    waiting_for_connection = 0;

#ifdef IPV6
    struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)&client_addr;
    struct in6_addr ip_addr = ipv6_addr->sin6_addr;
    inet_ntop(AF_INET6, &ip_addr, client_ip, IP_ADDR_LEN);
#endif /* ifdef IPV6 */

#ifdef IPV4
    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)&client_addr;
    struct in_addr ip_addr = ipv4_addr->sin_addr;
    inet_ntop(AF_INET, &ip_addr, client_ip, IP_ADDR_LEN);
#endif /* ifdef IPV4  */

    syslog(LOG_INFO, "Accepted connection from %s", client_ip);

    // Receive data
    ssize_t bytes_received;
    size_t rec_size = 0;
    char *rec_buf = NULL;
    char buf[BUF_SIZE];
    char *newline_pos = NULL;
    while (!newline_pos) {
      bytes_received = recv(client_fd, buf, BUF_SIZE, 0);
      if (bytes_received == -1) {
        syslog(LOG_ERR, "Error receiving data: %s", strerror(errno));
        close(sock_fd);
        close(client_fd);
        return -1;
      }

      char *temp_buf = realloc(rec_buf, rec_size + bytes_received + 1);
      if (!temp_buf) {
        syslog(LOG_ERR, "Error allocating memory for packet: %s",
               strerror(errno));
        free(rec_buf);
        close(sock_fd);
        close(client_fd);
        return -1;
      }
      rec_buf = temp_buf;

      memcpy(rec_buf + rec_size, buf, bytes_received);
      rec_size += bytes_received;
      rec_buf[rec_size] = '\0';

      newline_pos = strchr(rec_buf, '\n');
      if (newline_pos) {
        ssize_t packet_size = newline_pos - rec_buf + 1;

        int write_size = write_to_data_file(DATA_FILE, rec_buf, packet_size);
        if (write_size == -1) {
          syslog(LOG_ERR, "Error opening or writing to data file: %s",
                 strerror(errno));
          close(sock_fd);
          close(client_fd);
          return -1;
        }

        free(rec_buf);
        rec_buf = NULL;
        rec_size = 0;
      }
    }

    int fd = open(DATA_FILE, O_RDONLY);
    if (fd == -1) {
      syslog(LOG_ERR, "Error opening data file: %s", strerror(errno));
      close(sock_fd);
      close(client_fd);
      free(rec_buf);
      return -1;
    }

    char send_buf[BUF_SIZE] = {0};
    ssize_t read_size;
    while ((read_size = read(fd, send_buf, BUF_SIZE)) > 0) {
      ssize_t total_bytes_sent = 0;
      while (total_bytes_sent < read_size) {
        ssize_t bytes_sent = send(client_fd, send_buf + total_bytes_sent,
                                  read_size - total_bytes_sent, 0);
        if (bytes_sent == -1) {
          syslog(LOG_ERR, "Error sending data: %s", strerror(errno));
          close(fd);
          close(sock_fd);
          close(client_fd);
          return -1;
        }
        total_bytes_sent += bytes_sent;
      }
    }
    if (read_size == -1) {
      syslog(LOG_ERR, "Error reading data file: %s", strerror(errno));
      close(fd);
      close(sock_fd);
      close(client_fd);
      return -1;
    }
    close(fd);

    close(client_fd);
    syslog(LOG_INFO, "Closed connection from %s", client_ip);
  }
  close(sock_fd);
  closelog();
  remove(DATA_FILE);
  return 0;
}
