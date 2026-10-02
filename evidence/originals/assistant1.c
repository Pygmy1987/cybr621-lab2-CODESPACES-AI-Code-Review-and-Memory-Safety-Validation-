#include <stdio.h>

int main(int argc, char *argv[])
{
    FILE *log_file;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <username> <message>\n", argv[0]);
        return 1;
    }

    log_file = fopen("userlog.txt", "a");
    if (log_file == NULL) {
        perror("Could not open userlog.txt");
        return 1;
    }

    if (fprintf(log_file, "%s: %s\n", argv[1], argv[2]) < 0) {
        perror("Could not write to userlog.txt");
        fclose(log_file);
        return 1;
    }

    if (fclose(log_file) == EOF) {
        perror("Could not close userlog.txt");
        return 1;
    }

    return 0;
}