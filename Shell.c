#include <stdio.h>    
#include <unistd.h>    
#include <stdlib.h>    
#include <string.h>    
#include <sys/wait.h>
#include <fcntl.h>     
#include <signal.h>    

#define MAXLINE 80     // Defines the maximum length of the command line

// structure for managing the command history
struct History {
    char **lines;   // Array of command lines
    int max_size;   // Maximum number of lines in history
    int begin;      // Index of the first element in the circular buffer
    int size;       // Current number of elements in history
};

// global history object
struct History history;

// Function to initialize the command history structure
void initialize_history(struct History *history, int m_size) {
    history->max_size = m_size;                    // Set the maximum size of the history
    history->lines = malloc(m_size * sizeof(char *)); // Allocate memory for the command lines
    for (int i = 0; i < m_size; ++i) {             // Initialize each line in the history to NULL
        history->lines[i] = NULL;
    }
    history->size = 0;                             // Initialize the size of the history to 0
    history->begin = 0;                            // Initialize the beginning index of the history to 0
}

// Function to add a command to the history
void add_to_history(struct History *history, char *commandline) {
    if (history->lines[history->begin] != NULL) {  // If there is already a command at the current beginning
        free(history->lines[history->begin]);      // Free the memory of the existing command
    }
    history->lines[history->begin] = commandline;  // Add the new command at the current beginning
    history->begin = (history->begin + 1) % history->max_size; // Update the beginning index to the next position
    if (history->size < history->max_size) {       // If the history is not yet full
        history->size++;                           // Increase the size of the history
    }
}

// Function to print the command history
void print_history(struct History *history) {
    for (int i = 0; i < history->size; ++i) {      // Loop through the history
        int index = (history->begin + i) % history->max_size; // Calculate the index in the circular buffer
        if (history->lines[index] != NULL) {       // If there is a command at the current index
            printf("%d %s\n", i + 1, history->lines[index]); // Print the command with its index
        }
    }
}

// Function to free the memory allocated for the command history
void free_history(struct History *history) {
    for (int i = 0; i < history->max_size; ++i) {  // Loop through the history
        if (history->lines[i] != NULL) {           // If there is a command at the current index
            free(history->lines[i]);               // Free the memory of the command
        }
    }
    free(history->lines);                          // Free the memory allocated for the lines array
}

// Signal handler for SIGINT and SIGTSTP
void signal_handler(int signal_number) {
    if (signal_number == SIGINT) {                 // If the signal is SIGINT (Ctrl+C)
        printf("\nCaught SIGINT\n");               // Print a message
    } else if (signal_number == SIGTSTP) {         // If the signal is SIGTSTP (Ctrl+Z)
        printf("\nCaught SIGTSTP\n");              // Print a message
    }
}

// Function to free the memory allocated for command arguments
void freeArgs(char *args[], int argv) {
    for (int i = 0; i < argv; ++i) {               // Loop through the arguments
        free(args[i]);                             // Free the memory of each argument
    }
}

// Function to read a command from the user and parse it into arguments
void readCommandFromUser(char *args[], int *hasAmp, int *argv, int *exitFlag) {
    char userCommand[MAXLINE];                     // Buffer to store the user command
    int length = read(STDIN_FILENO, userCommand, MAXLINE); // Read the command from stdin

    if (length <= 0) {                             // If no command is read
        return;                                    // Return without doing anything
    }

    if (userCommand[length - 1] == '\n') {         // If the last character is a newline
        userCommand[length - 1] = '\0';            // Replace it with a null terminator
    }

    if (strcmp(userCommand, "hist") == 0) {          // If the command is "hist"
        if (history.size == 0) {                   // If there are no commands in history
            printf("No commands in history.\n");   // Print a message
            return;                                // Return without doing anything
        } else {
            strcpy(userCommand, history.lines[(history.begin - 1 + history.max_size) % history.max_size]); // Copy the last command from history
            printf("%s\n", userCommand);           // Print the last command
        }
    } else {
        char *commandline = strdup(userCommand);   // Duplicate the user command
        add_to_history(&history, commandline);     // Add the command to history
    }

    if (strcmp(userCommand, "exit") == 0 || strcmp(userCommand, "quit") == 0) { // If the command is "exit" or "quit"
        *exitFlag = 1;                             // Set the exit flag to 1
        return;                                    // Return without doing anything
    }

    freeArgs(args, *argv);                         // Free the previous arguments
    *argv = 0;                                     // Reset the argument count to 0
    *hasAmp = 0;                                   // Reset the background flag to 0

    char *ptr = strtok(userCommand, " ");          // Tokenize the user command
    while (ptr != NULL) {                          // Loop through the tokens
        if (strcmp(ptr, "&") == 0) {               // If the token is "&"
            *hasAmp = 1;                           // Set the background flag to 1
        } else {
            args[*argv] = strdup(ptr);             // Duplicate the token and store it in the arguments array
            (*argv)++;                             // Increment the argument count
        }
        ptr = strtok(NULL, " ");                   // Get the next token
    }

    args[*argv] = NULL;                            // Null-terminate the arguments array
}

int main(void) {
    char *args[MAXLINE / 2 + 1]; /* Command line arguments */
    int runFlag = 1; // Flag to keep the shell running
    pid_t pid; // Process ID
    int hasAmp = 0; // Flag to check if command should run in the background
    int argv = 0; // Number of arguments
    int exitFlag = 0; // Flag to check if the user wants to exit the shell

    // Initialize the history with a maximum size of 10
    initialize_history(&history, 10);

    while (runFlag) {
        printf("shell>");
        fflush(stdout);

        // Set up signal handling for SIGINT and SIGTSTP
        signal(SIGINT, signal_handler);
        signal(SIGTSTP, signal_handler);

        // Read command from user
        readCommandFromUser(args, &hasAmp, &argv, &exitFlag);

        // Check if the user wants to exit the shell
        if (exitFlag) {
            printf("Exiting shell...\n");
            break;
        }

        if (argv == 0) {
            continue;
        }

        pid = fork();
        if (pid == 0) { // Child process
            int redirectCase = 0;
            int file;

            for (int i = 0; i < argv; i++) {
                if (strcmp(args[i], "<") == 0) { // Input redirection
                    file = open(args[i + 1], O_RDONLY);
                    if (file == -1 || args[i + 1] == NULL) {
                        perror("Invalid input redirection");
                        exit(1);
                    }
                    dup2(file, STDIN_FILENO);
                    args[i] = NULL;
                    args[i + 1] = NULL;
                    redirectCase = 1;
                    break;
                } else if (strcmp(args[i], ">") == 0) { // Output redirection
                    file = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open for write only and create if non-existent

                    if (file == -1 || args[i + 1] == NULL) {
                        perror("Invalid output redirection");
                        exit(1);
                    }
                    dup2(file, STDOUT_FILENO);
                    args[i] = NULL;
                    args[i + 1] = NULL;
                    redirectCase = 2;
                    break;
                } else if (strcmp(args[i], "|") == 0) { // Pipe
                    int fd[2];
                    if (pipe(fd) == -1) {
                        perror("Pipe failed");
                        exit(1);
                    }
                    pid_t pid_pipe = fork();
                    if (pid_pipe == 0) { // Child process for first command
                        close(fd[0]);
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[1]);
                        args[i] = NULL;
                        if (execvp(args[0], args) == -1) {
                            perror("Invalid command");
                            exit(1);
                        }
                    } else if (pid_pipe > 0) { // Parent process for second command
                        wait(NULL);
                        close(fd[1]);
                        dup2(fd[0], STDIN_FILENO);
                        close(fd[0]);
                        char *secondCommand[MAXLINE / 2 + 1];
                        int j;
                        for (j = i + 1; j < argv; j++) {
                            secondCommand[j - (i + 1)] = args[j];
                        }
                        secondCommand[j - (i + 1)] = NULL;
                        if (execvp(secondCommand[0], secondCommand) == -1) {
                            perror("Invalid command");
                            exit(1);
                        }
                    }
                    break;
                }
            }

            if (execvp(args[0], args) == -1) {
                perror("Invalid command");
                exit(1);
            }
            if (redirectCase == 1) {
                close(STDIN_FILENO);
            } else if (redirectCase == 2) {
                close(STDOUT_FILENO);
            }
            close(file);
        } else if (pid > 0) { // Parent process
            if (hasAmp) {
                printf("Child process running in the background, pid: %d\n", pid);
            } else {
                wait(NULL);
            }
        } else {
            perror("Fork failed");
            return 1;
        }
    }
    free_history(&history);
    return 0;
}
