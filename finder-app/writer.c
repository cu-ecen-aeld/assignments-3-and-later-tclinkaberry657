#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  // open the log with facility LOG_USER (generic user level messages)
  openlog("writer", 0, LOG_USER);

  // if the correct number of arguments is not specified throw an error and exit
  if (argc != 3) {
    syslog(LOG_ERR,
           "Incorrect number of arguments specified.\nusage: %s <writefile> "
           "<writestr>",
           argv[0]); // log the error in syslog
    closelog();
    return 1;
  }

  // create constants for the write file and the write string
  const char *writefile = argv[1];
  const char *writestr = argv[2];

  // open the new file to be written to
  int file = open(writefile, O_RDWR | O_CREAT, 0664);
  if (file == -1) {
    syslog(LOG_ERR, "Error opening file %s: %s", writefile,
           strerror(errno)); // log error if error opening
    closelog();
    return 1;
  }

  // syslog message to note we are going to write to the newly created file
  syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

  // write to the file
  ssize_t writeSize;
  writeSize = write(file, writestr, strlen(writestr));
  if (writeSize == -1) {
    syslog(LOG_ERR, "Error writing %s to %s: %s", writestr, writefile,
           strerror(errno)); // log any errors that occur while writing
    closelog();
    close(file);
    return 1;
  }

  // close the file
  if (close(file) == -1) {
    syslog(LOG_ERR, "Error closing file %s: %s", writefile, strerror(errno));
    closelog();
    return 1;
  }

  return 0;
}
