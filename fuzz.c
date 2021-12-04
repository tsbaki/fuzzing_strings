#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "dirent.h"
#include "unistd.h"
#include "string.h"

#include "sys/wait.h"

#define CORPUS_DIR "./corpus/"
#define TARGET_PATH "./strings"
#define FILE_COUNT 1000

#define LOG(s) printf("%s\n", #s);


char* compute_hash() { 
    int length = 20;
    char *hash;
    char charset[] = "0123456789" 
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    hash = (char*)malloc(length);

    while (length-- > 0) {
        // Get a random index
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *hash++ = charset[index];
    }

    return hash;
}

void fuzz(char *file_name) {
    char *input_path;
    char *command;
    char *output_file;
    int ret;
    FILE *fp;
    pid_t child_pid, wpid;

    input_path = (char *) malloc(255);
    strcpy(input_path, CORPUS_DIR);
    strcat(input_path, file_name);

    command = (char *)malloc(255);
    sprintf(command, "%s %s", TARGET_PATH, input_path);

    // Spawn a new child-process
    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        
        // Execute program
        execl(command, NULL);

    } else {
        int status;
        do {
            wpid = waitpid(child_pid, &status, WUNTRACED
#ifdef WCONTINUED
                    | WCONTINUED /* Not all implementations support this */
#endif
                    );

            if (wpid == -1) { perror("waitpid"); }

            if (WIFEXITED(status)) {
                printf("Child exited with status %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child killed with status %d\n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("Child stopped with status %d\n", WSTOPSIG(status));
            }
#ifdef WCONTINUED
            else if (WIFCONTINUED(status)) {
                printf("Child continued...\n");
            }
#endif
            else { 
                printf("Unexpected status (0x%x)\n", status);
            }

            exit(0);

        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

}

void load_files(char *file_names[]) {
    static struct dirent *files;
    DIR *dir = opendir(CORPUS_DIR);

    if (dir == NULL) {
        LOG(Could not open directory...);
    }

    for (int i, j = 0; (files = readdir(dir)) != NULL && i 
            <= FILE_COUNT; i++) {
        // This code is skipping the '.' and '..' that comes
        // with reading the directory
        if (j < 2)  { j++; i = 0; continue; } 
        file_names[i-1] = files->d_name;
    }

    LOG(Files loaded into memory...);

}

int main(void){
    char *file_names[FILE_COUNT];

    // Load the file names to memory
    load_files(file_names);

    for (int i = 0; i < FILE_COUNT; i++){
        fuzz(file_names[i]);
    }

    return 0;
}


