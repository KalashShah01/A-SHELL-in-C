#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <limits.h>
#include <fcntl.h>

#define MAXCOM 1000 // max number of letters to be supported
#define MAXLIST 100 // max number of commands to be supported
#define MAX_ENV_VARS 15 // maximum number of environment variables
#define MAX_ENV_VAR_LEN 17 // maximum length of environment variable name
#define MAX_ENV_VAL_LEN 241 // maximum length of environment variable value
#define HASH_TABLE_SIZE 20 // size of the hash table

int processString(char* str, char** parsed, char** parsedpipe, int* background);
void parseSpace(char* str, char** parsed);

// Environment variable structure
typedef struct {
    char name[MAX_ENV_VAR_LEN];
    char value[MAX_ENV_VAL_LEN];
}envVars;

// Hash table node
typedef struct HashNode {
    char name[MAX_ENV_VAR_LEN];
    char value[MAX_ENV_VAL_LEN];
    struct HashNode* next;
}HashNode;

HashNode* hashTable[HASH_TABLE_SIZE]; // Hash table for environment variables

// Global variable to store exit code
int last_exit_status = 0;

// Hash function
unsigned int hashFunction(const char* name) {
    unsigned int hash = 0;
    while (*name) {
        hash = (hash << 5) + *name++;
    }
    return hash % HASH_TABLE_SIZE;
}

// Function to insert a key-value pair into the hash table
void insertIntoHashTable(const char* name, const char* value) {
    unsigned int index = hashFunction(name);
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (!newNode) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    strcpy(newNode->name, name);
    strcpy(newNode->value, value);
    newNode->next = NULL;

    if (hashTable[index] == NULL) {
        hashTable[index] = newNode;
    } else {
        // Collision handling: append at the end
        HashNode* temp = hashTable[index];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;
    }
}

// Function to search for a key in the hash table
char* searchInHashTable(const char* name) {
    unsigned int index = hashFunction(name);
    HashNode* temp = hashTable[index];
    while (temp != NULL) {
        if (strcmp(temp->name, name) == 0) {
            return temp->value;
        }
        temp = temp->next;
    }
    return NULL; // Key not found
}


/*
    Function: printPrompt

    Description:
    This function prints the command prompt to indicate to the user that the shell is ready to receive input.

    Parameters:
    None

    Return:
    None

    Details:
    1. It retrieves the current username using getpwuid() function.
    2. It retrieves the hostname using gethostname() function.
    3. It retrieves the current working directory using getcwd() function.
    4. It prints the command prompt in the format "<username>@<hostname>:<current_working_directory>$ ".
       For example, "user@localhost:/home/user$ ".
*/
void printPrompt() {
    char hostname[1000];
    char cwd[1000];
    struct passwd *pw;
    uid_t uid;

    // Get the current username
    uid = geteuid();
    pw = getpwuid(uid);
    if (pw == NULL) {
        perror("Failed to get username");
        return;
    }

    // Get the hostname
    gethostname(hostname, sizeof(hostname));

    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Failed to get current working directory");
        return;
    }

    // Print the command line prompt
    printf("%s@%s:%s$ ", pw->pw_name, hostname, cwd);
}


/*
    Function: takeInput

    Description:
    This function takes input from the user via stdin (standard input) and stores it in a string buffer.

    Parameters:
    - str: A pointer to a character array where the user input will be stored.

    Return:
    - Returns 0 on success.
    - Returns 1 if there is a failure in reading input.

    Details:
    1. The function prints a prompt to indicate that it is ready to receive input.
    2. It uses fgets() to read a line of input from stdin into the provided string buffer.
    3. If fgets() returns NULL, it indicates a failure in reading input, and an error message is printed.
    4. It removes the newline character ('\n') from the end of the input string if present.
*/
int takeInput(char* str) {
    printPrompt(); // Print prompt for user input
    if (fgets(str, MAXCOM, stdin) == NULL) { // Read input from stdin
        printf("Failed to read input\n"); // Print error message if reading fails
        return 1; // Return failure
    }
    size_t len = strlen(str); // Calculate length of input string
    if (len > 0 && str[len - 1] == '\n'){ // Check if newline character exists at the end of input
        str[len - 1] = '\0'; // Remove newline character by replacing it with null terminator
    }
    return 0; // Return success
}

/*
    Function: execArgs

    Description:
    This function executes a command with arguments passed as an array of strings.
    It optionally supports input and output redirection and can run the command in the background.

    Parameters:
    - parsed: A pointer to an array of strings representing the command and its arguments.
    - background: An integer flag indicating whether the command should run in the background (1) or not (0).

    Return:
    None

    Details:
    1. The function forks a new process to execute the command.
    2. In the child process:
       a. It checks for input and output redirection symbols ("<" and ">") in the parsed command.
       b. If input redirection is detected, it opens the specified file and redirects standard input to it.
       c. If output redirection is detected, it opens the specified file and redirects standard output to it.
       d. It then attempts to execute the command using execvp().
       e. If execvp fails, it prints an error message and exits the child process with a failure status.
    3. In the parent process:
       a. If the command is not set to run in the background:
          - It waits for the child process to finish.
          - It retrieves the exit status of the child process.
          - It stores the exit status in the global variable 'last_exit_status'.
*/
void execArgs(char** parsed, int background) {
    pid_t pid = fork();
    if (pid == -1) {
        printf("Failed to fork child\n");
        return;
    } else if (pid == 0) {
        // Redirect standard input and output if necessary
        for (int i = 0; parsed[i] != NULL; i++) {
            if (strcmp(parsed[i], "<") == 0) {
                // Input redirection
                int fd = open(parsed[i+1], O_RDONLY);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
                parsed[i] = NULL;
                parsed[i+1] = NULL;
                break; // Stop processing further
            } else if (strcmp(parsed[i], ">") == 0) {
                // Output redirection
                int fd = open(parsed[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                parsed[i] = NULL;
                parsed[i+1] = NULL;
                break; // Stop processing further
            }
        }

        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute command\n");
            exit(EXIT_FAILURE); // If execvp fails, child process exits with failure status
        }
    } else {
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                last_exit_status = WEXITSTATUS(status); // Store exit code
            }
        }
    }
}

/* Function: parseEnvVars
Description: This function parses environment variables from the array of parsed arguments and inserts them into a hash table. Each environment variable is expected to be in the form "name=value" within the parsed array. The function iterates through the array, identifying environment variables by locating the "=" separator. It then separates the name and value, inserts them into the hash table, and modifies the parsed array by setting the "=" element to NULL to indicate the end of each environment variable. 
This prevents the separator from being treated as a separate argument in later processing.

 Parameters:
 - parsed: A pointer to an array of strings representing the parsed arguments, where environment variables are expected to be in the format "name=value".
 Returns: None */
void parseEnvVars(char** parsed) {
    for (int i = 1; parsed[i] != NULL; i++) {
        if (strcmp(parsed[i], "=") == 0) {
            // Separate name and value
            parsed[i] = NULL;
            char* name = parsed[i - 1];
            char* value = parsed[i + 1];
            insertIntoHashTable(name, value);
        }
    }
}


/*
  Function: setEnvVar

  Description:
  Sets environment variables by parsing and extracting name-value pairs from the given command arguments.
  The function ensures correct syntax and removes quotes around the value if present.

  Parameters:
  - parsed: An array of strings representing the command and its arguments, where parsed[1] contains the name-value pair.

  Returns:
  Void. Sets the environment variable if the syntax is valid.

  Notes:
  - Parses the name-value pair from the command arguments.
  - Checks for valid syntax; if not, prints an error message and returns.
  - Separates the name and value, and removes quotes from the value if present.
  - Calls insertIntoHashTable() function to set the environment variable.
*/
void setEnvVar(char** parsed) {
    if (parsed[1] == NULL) {
        fprintf(stderr, "Invalid syntax for set command\n");
        return;
    }

    // Separate name and value
    char* assignment = parsed[1];
    char* equal_sign = strchr(assignment, '=');
    if (equal_sign == NULL) {
        fprintf(stderr, "Invalid syntax for set command\n");
        return;
    }

    *equal_sign = '\0'; // Separate name and value
    char* name = assignment;
    char* value = equal_sign + 1;

    // Remove quotes if present
    if (value[0] == '"' && value[strlen(value) - 1] == '"') {
        value[strlen(value) - 1] = '\0';
        value++;
    }

    insertIntoHashTable(name, value);
}

/*
  Function: getEnvVar

  Description:
  Retrieves the value of an environment variable specified by the given name.
  If the name is '?', prints the last exit status.
  If the variable is found, prints its value; otherwise, prints a message indicating the variable was not found.

  Parameters:
  - parsed: An array of strings representing the command and its arguments, where parsed[1] contains the variable name.

  Returns:
  Void. Prints the value of the variable or an appropriate message.

  Notes:
  - Retrieves the name of the environment variable from the command arguments.
  - If the name is '?', prints the last exit status.
  - Searches for the variable in the hash table using searchInHashTable() function.
  - If the variable is found, prints its value; otherwise, prints a message indicating the variable was not found.
*/
void getEnvVar(char** parsed) {
    char* name = parsed[1];
    char* value;
    if (strcmp(name, "?") == 0) {
        printf("%d\n", last_exit_status); // Print last exit status
    } else {
        value = searchInHashTable(name);
        if (value != NULL) {
            printf("%s\n", value);
        } else {
            printf("Variable not found\n"); // Print message if variable not found
        }
    }
}

/*
  Function: execArgsPiped

  Description:
  Executes two commands in a piped manner. The output of the first command becomes the input of the second command.

  Parameters:
  - parsed: An array of strings representing the first command and its arguments.
  - parsedpipe: An array of strings representing the second command and its arguments.

  Returns:
  Void. Executes the commands in a piped manner.

  Notes:
  - Uses fork() and execvp() to execute the commands.
  - Creates a pipe using pipe() system call.
  - Redirects stdout of the first command to the write end of the pipe and stdin of the second command to the read end of the pipe.
  - Waits for both child processes to complete before returning.
*/
void execArgsPiped(char** parsed, char** parsedpipe) {
    int pipefd[2];
    pid_t p1, p2;

    // Create the pipe
    if (pipe(pipefd) < 0) { 
        printf("Pipe could not be initialized\n");
        return;
    }

    // Fork the first child process
    p1 = fork();
    if (p1 < 0) {
        printf("Could not fork\n");
        return;
    }

    if (p1 == 0) {
        close(pipefd[0]); // Close the read end of the pipe
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
        close(pipefd[1]); // Close the write end of the pipe

        // Execute the first command
        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute command 1\n");
            exit(0);
        }
    } else {
      // Parent process
        p2 = fork(); // Fork the second child process

        if (p2 < 0) {
            printf("Could not fork\n");
            return;
        }

        if (p2 == 0) {
            close(pipefd[1]); // Close the write end of the pipe
            dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the read end of the pipe
            close(pipefd[0]); // Close the read end of the pipe

            // Execute the second command
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf("Could not execute command 2\n");
                exit(0);
            }
        } else {
          // Parent process
            close(pipefd[0]); // Close the read end of the pipe
            close(pipefd[1]); // Close the write end of the pipe
            wait(NULL); // Wait for the first child process to complete
            wait(NULL); // Wait for the second child process to complete
        }
    }
}

/*
  Function: changeDirectory

  Description:
  Changes the current working directory to the directory specified in the command arguments.

  Parameters:
  - parsed: An array of strings representing the command and its arguments, where parsed[1] contains the directory to change to.

  Returns:
  Void. Changes the current working directory.

  Notes:
  - Checks if the expected argument for "cd" command is provided.
  - Uses chdir() system call to change the current working directory.
  - Prints an error message if chdir() fails.
*/
void changeDirectory(char** parsed) {
    // `parsed[1]` should be the directory to change to
    if (parsed[1] == NULL) {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    } else {
        if (chdir(parsed[1]) != 0) {
            perror("chdir failed");
        }
    }
}


/*
  Function: execCommandSeq

  Description:
  Executes a sequence of commands provided as a single input string. 
  Supports commands separated by ',' and handles piped commands.

  Parameters:
  - inputString: The input string containing the sequence of commands to execute.

  Returns:
  Void. Executes the sequence of commands.

  Notes:
  - Parses the input string into individual commands separated by ','.
  - Processes each command using processString() function.
  - Executes each command or piped commands accordingly.
*/
void execCommandSeq(char* inputString) {
    char* command;
    char* rest = inputString;
    char* parsedArgs[MAXLIST] = {NULL}; // Initialize parsedArgs array with NULL pointers
    char* parsedArgsPiped[MAXLIST] = {NULL}; // Initialize parsedArgsPiped array with NULL pointers
    int background = 0;

    while ((command = strsep(&rest, ",")) != NULL) {
        int execFlag = processString(command, parsedArgs, parsedArgsPiped, &background);

        if (execFlag == 1) { // No pipe
            if (strcmp(parsedArgs[0], "cd") == 0) {
                changeDirectory(parsedArgs);
            } else {
                execArgs(parsedArgs, background);
            }
        } else if (execFlag == 2) { // Piped commands
            parseSpace(command, parsedArgs);
            execArgsPiped(parsedArgs, parsedArgsPiped);
        }
    }
}

/*
Function: parseSpace

Description:
Processes a given input string, separating it into individual tokens based on spaces or commas.
Handles quoted strings, allowing spaces within quotes to be considered as part of the token.
Allocates memory for each token and copies it into the parsed array.

Parameters:
- str: The input string to be processed.
- parsed: An array of strings to store the parsed tokens.

Returns: None

Notes:
- Handles quoted strings enclosed within double quotes.
- Supports escaping within quoted strings using backslashes.
- Each token is copied into the parsed array after memory allocation.
- The last element of the parsed array is set to NULL to indicate the end.
*/
void parseSpace(char* str, char** parsed) {
    int i = 0;
    int in_quote = 0;

    while (*str != '\0') {
        // Skip leading spaces
        while (*str == ' ')
            str++;

        // Break if end of string is reached
        if (*str == '\0')
            break;

        // Find the end of the current token
        char* end = str;
        if (*end == '"') {
            // Handle quoted string
            end++; // Move past the opening quote
            in_quote = 1;
        }
        while (*end != '\0' && ((*end != ' ' && *end != ',') || in_quote)) {
            if (*end == '"' && *(end - 1) != '\\') {
                in_quote = 0; // End of quoted string
            }
            end++;
        }

        // Allocate memory for the token and copy it
        parsed[i] = (char*)malloc((end - str + 1) * sizeof(char));
        if (parsed[i] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        // Copy token without quotes
        if (*str == '"') {
            str++; // Move past the opening quote
            strncpy(parsed[i], str, end - str - 1); // Exclude the closing quote
            parsed[i][end - str - 1] = '\0';
        } else {
            strncpy(parsed[i], str, end - str);
            parsed[i][end - str] = '\0';
        }
        i++;

        // Move to the next token
        str = end;
    }

    // Set the last element of the parsed array to NULL to indicate the end
    parsed[i] = NULL;
}

/*
  Function: processString

  Description:
  Processes a given input string, separating it into individual commands and parsing their arguments.
  Handles piped commands and checks for special commands like 'exit', 'set', and 'get'.

  Parameters:
  - str: The input string to be processed.
  - parsed: An array of strings to store the arguments of the command.
  - parsedpipe: An array of strings to store the arguments of the piped command (if present).
  - background: A pointer to an integer indicating whether the command should be executed in the background.

  Returns:
  An integer indicating the type of command:
  - 0 if no command to execute (empty input or 'exit' command).
  - 1 if a single command to execute.
  - 2 if piped commands to execute.

  Notes:
  - Checks for special commands like 'exit', 'set', and 'get'.
  - Handles background execution indicated by '#' at the end of the command.
  - Splits piped commands and parses their arguments.
*/
int processString(char* str, char** parsed, char** parsedpipe, int* background) {
    if (str == NULL || *str == '\0') {
        // No input provided, return 0 to indicate no command to execute
        return 0;
    }

    char* strpiped[2];
    int piped = 0;

    // Check for background execution
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '#') {
        *background = 1;
        str[len - 1] = '\0'; // Remove '#' from command
    }

    // Check if the command is 'exit'
    if (strcmp(str, "exit") == 0) {
        exit(0); // Exit the shell
    }

    // Split command into piped parts
    char* pipeSeparator = strstr(str, "|");
    if (pipeSeparator != NULL) {
        piped = 1;
        *pipeSeparator = '\0'; // Replace underscore with null terminator
        strpiped[0] = str;
        strpiped[1] = pipeSeparator + 1; // Move to the next character after underscore
        parseSpace(strpiped[0], parsed);
        parseSpace(strpiped[1], parsedpipe);
    } else {
        parseSpace(str, parsed);
    }

    if (strncmp(parsed[0], "set", 3) == 0) {
        setEnvVar(parsed);
        return 0;
    } else if (strncmp(parsed[0], "get", 3) == 0) {
        getEnvVar(parsed);
        return 0;
    }

    return piped ? 2 : 1;
}

// Main function
int main() {
    char inputString[MAXCOM];
    while (1) {
        if (takeInput(inputString)) continue;
        execCommandSeq(inputString);
    }
    return 0;
}