#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int interactive_mode(char **path);
int batch_mode(char *fname, char **path);
int execute(char *line, char **path);

char error_message[30] = "an error has occured\n";

int main(int argc, char *argv[]) {
    char *path[64];
    path[0] = strdup("/bin");
    path[1] = NULL;

    if (argc > 2) {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else if (argc > 1) {
        batch_mode(argv[1], path);
        return 0;
    } 
    else {
        interactive_mode(path);
        return 0;
    }

    return 0;
}

int interactive_mode(char **path) {
    char *line = NULL;
    while(1) {
        printf("grsh> "); 
        size_t len; //setup buffer for reading input
        len = 0;
        ssize_t read = getline(&line, &len, stdin); 

        if (read == -1) { // EOF
            printf("\n");
            break;
        }

        //remove trailing newline
        if (read > 0 && line[read - 1] == '\n')
            line[read - 1] = 0;
        
        execute(line, path);
        }
        free(line);
        return 0;
    }

int batch_mode(char *fname, char **path) {
    FILE *f = fopen(fname, "r"); //opens file
    if (!f) { //if theres no file
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    char buffer[1024]; //create a buffer to read lines
    
    while (fgets(buffer, sizeof(buffer), f)) {
        //remove trailing newline
        buffer[strcspn(buffer, "\n")] = 0;
        execute(buffer, path);
    }
    fclose(f);
    return 0;
}

int execute(char *line, char **path) {
    char *token = strtok(line, "&");
    int a = 0;
    char *commands[64];

    while (token != NULL && a < 63) {
        while (*token == ' ') token++;                       
        token[strcspn(token, "\n")] = '\0';                  

        int len = strlen(token);
        while (len > 0 && token[len - 1] == ' ') {
            token[--len] = '\0';
        }
            commands[a++] = strdup(token);
            token = strtok(NULL, "&");
    }
    commands[a] = NULL;

    int p = 0;
    int pids[64];
    for (int c = 0; c < a; c++) {
        int i = 0;
        token = strtok(commands[c], " ");
        int redirect_flag = 0;
        char *redirect_file = NULL;
        char *args[64];

        while (token != NULL && i < 63) {
            if (strcmp(token, ">") == 0) {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    return 1;
                }
                redirect_flag = 1;
                redirect_file = token;

                if (strtok(NULL, " ") != NULL) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    return 1;
                }
                break;
            }
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] == NULL)
            return 0;

        if (strcmp(args[0], "exit") == 0) {
            if(args[1] != NULL)
                write(STDERR_FILENO, error_message, strlen(error_message));
            else
                exit(0);
        }
        else if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            chdir(args[1]);
            continue;
        }
        else if (strcmp(args[0], "path") == 0) {
            int i = 0;
            while (path[i] != NULL) {
                free(path[i]);
                path[i] = NULL;
                i++;
            }
            i = 0;
            if (args[1] == NULL) {
                path[0] = NULL;
                continue;
            }
            else {
                int j = 1;  
                while (args[j] != NULL) {
                    path[i] = strdup(args[j]);
                    i++;
                    j++;
                }
                path[i] = NULL;
            }
            continue;
        }
        int pid = fork();
        if (pid > 0) {
            pids[p++] = pid;
        }
        else if (pid < 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        } 
        else if (pid == 0) {
            // child process
            for (int i = 0; path[i] != NULL; i++) {
                char full_path[128];
                strcpy(full_path, path[i]);
                strcat(full_path, "/");
                strcat(full_path, args[0]);
                if (access(full_path, X_OK) == 0) {
                    if (redirect_flag) {
                        FILE *f = fopen(redirect_file, "w");
                        if (f == NULL) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        }
                        dup2(fileno(f), STDOUT_FILENO);
                        fclose(f);
                    }
                    execv(full_path, args);
                    // if execv returns, it's an error
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            } 
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    for (int ps = 0; ps < p; ps++) {
        waitpid(pids[ps], NULL, 0);
    }
    sync();
    return 0;
}