#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *log_file;

    /* Check that both arguments exist before accessing them. */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <username> <message>\n", argv[0]);
        return 1;
    }

    /* Validate inputs before opening or changing the log. */
    size_t username_length = strnlen(argv[1], 65);
    size_t message_length = strnlen(argv[2], 4097);

    if (username_length == 0 || username_length > 64 ||
        message_length == 0 || message_length > 4096) {
        fprintf(stderr,
                "Invalid input: username must be 1-64 bytes "
                "and message 1-4096 bytes.\n");
        return 2;
    }

    if (strspn(argv[1],
               "abcdefghijklmnopqrstuvwxyz"
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
               "0123456789_.-") != username_length) {
        fprintf(stderr, "Invalid username characters.\n");
        return 2;
    }

    /* Reject control characters, including newlines and carriage returns. */
    for (size_t i = 0; i < message_length; ++i) {
        unsigned char c = (unsigned char)argv[2][i];

        if (c < 0x20 || c == 0x7f) {
            fprintf(stderr,
                    "Invalid message: control characters are not allowed.\n");
            return 2;
        }
    }

    /* Create new logs with read/write access for the owner only. */
    int fd = open("userlog.txt", O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        perror("Could not open userlog.txt");
        return 1;
    }

    /* Use the descriptor with the existing stdio logging code. */
    log_file = fdopen(fd, "a");
    if (log_file == NULL) {
        perror("Could not create log stream");
        if (close(fd) < 0) {
            perror("Could not close log descriptor");
        }
        return 1;
    }

    if (fprintf(log_file, "%s: %s\n", argv[1], argv[2]) < 0) {
        perror("Could not write to userlog.txt");
        if (fclose(log_file) == EOF) {
            perror("Could not close userlog.txt");
        }
        return 1;
    }

    /* fclose also checks for errors when buffered output is flushed. */
    if (fclose(log_file) == EOF) {
        perror("Could not close userlog.txt");
        return 1;
    }

    return 0;
}