#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
    MAX_USERNAME_LEN = 64,
    MAX_MESSAGE_LEN = 4096,
    RECORD_CAPACITY = MAX_USERNAME_LEN + 1 + MAX_MESSAGE_LEN + 1 + 1
};

/* Measure an argument without reading past its permitted maximum. */
static size_t
bounded_length(const char *text, size_t limit)
{
    size_t length = 0;

    while (length < limit && text[length] != '\0') {
        ++length;
    }
    return length;
}

/* Keep usernames predictable and prevent log-record injection. */
static int
valid_username(const char *username, size_t length)
{
    if (length == 0 || length > MAX_USERNAME_LEN) {
        return 0;
    }

    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)username[i];

        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.')) {
            return 0;
        }
    }
    return 1;
}

/* Reject control characters so the message cannot add forged log lines. */
static int
valid_message(const char *message, size_t length)
{
    if (length == 0 || length > MAX_MESSAGE_LEN) {
        return 0;
    }

    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)message[i];

        if (c < 0x20 || c == 0x7f) {
            return 0;
        }
    }
    return 1;
}

/* Write the entire record, retrying interrupted and partial writes. */
static int
write_all(int fd, const char *buffer, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t result = write(fd, buffer + written, length - written);

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (result == 0) {
            errno = EIO;
            return -1;
        }
        written += (size_t)result;
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s USERNAME MESSAGE\n", argv[0]);
        return 2;
    }

    size_t username_length = bounded_length(argv[1], MAX_USERNAME_LEN + 1);
    size_t message_length = bounded_length(argv[2], MAX_MESSAGE_LEN + 1);

    if (!valid_username(argv[1], username_length)) {
        fprintf(stderr, "Invalid username: use 1-64 letters, digits, '.', '_' or '-'.\n");
        return 2;
    }
    if (!valid_message(argv[2], message_length)) {
        fprintf(stderr, "Invalid message: use 1-4096 characters without control characters.\n");
        return 2;
    }

    char record[RECORD_CAPACITY];
    int record_length = snprintf(record, sizeof(record), "%s: %s\n",
                                 argv[1], argv[2]);
    if (record_length < 0 || (size_t)record_length >= sizeof(record)) {
        fprintf(stderr, "Could not format log record.\n");
        return 1;
    }

    /* Do not follow a symlink; create new log files with owner-only access. */
    int fd = open("userlog.txt",
                  O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW,
                  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("open userlog.txt");
        return 1;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) < 0) {
        perror("fstat userlog.txt");
        close(fd);
        return 1;
    }

    /* Refuse files that could expose log contents or redirect them elsewhere. */
    if (!S_ISREG(file_info.st_mode) ||
        file_info.st_uid != geteuid() ||
        file_info.st_nlink != 1 ||
        (file_info.st_mode & (S_IRWXG | S_IRWXO)) != 0) {
        fprintf(stderr, "userlog.txt must be a private, singly-linked regular file owned by this user.\n");
        close(fd);
        return 1;
    }

    /* Serialize cooperating writers so their records do not interleave. */
    if (flock(fd, LOCK_EX) < 0) {
        perror("flock userlog.txt");
        close(fd);
        return 1;
    }

    int write_result = write_all(fd, record, (size_t)record_length);
    int saved_errno = errno;

    if (close(fd) < 0 && write_result == 0) {
        perror("close userlog.txt");
        return 1;
    }

    if (write_result < 0) {
        errno = saved_errno;
        perror("write userlog.txt");
        return 1;
    }

    return 0;
}