# A-SHELL-in-C-FLASH

FLAme-SHell (flash) is a shell program implemented in C, capable of accepting commands from the user and executing them. This README provides an overview of FLASH and includes instructions for building and using the shell, as well as details about its features and design considerations.


### Collaborator: Kalash Shah

## Features
- Read-Eval-Print Loop (REPL) functionality.
- Support for different types of command lines:
  - Simple command line
  - Background command line
  - Sequence of command lines
  - Pipe command lines
- Redirection of standard input and output.
- Handling of command line arguments and return values.
- Environment variable support with a maximum of 15 variables.
- Special environment variable `?` to store the return value of the most recently executed command.
- `set` and `get` commands for managing environment variables.

### How to use the code:

#### Compiling and Execution:

```Bash
make #For compiling 
./flash
``` 

```
```
###### Command Line Sequence
```Bash
# Execute a sequence of commands separated by commas
FLASH$ ls -l, pwd, date
 
```

###### The code will always run in foreground unless asked to run in the background

###### Running in background
```Bash
# Execute a command in the background
FLASH$ <command> "#"
```

###### Execute commands via piping
```Bash
#A pipe is a sequence of two simple command lines separated by a “|” character. The simple command line before the “|” is the “(data) source”, and the simple command line after the “|” is the “(data) sink”. The pipe connects the standard output of the data source to the standard input of the data sink.
FLASH$ ls -l | grep .txt
```
**Explanation: Here, because it was difficult to implement "-" or using "--", because many of the codes demand flags which start with "-", hence we have traditionally use "|" for piping.

###### Redirection
```Bash
# The shell must support the redirection of a simple command line. By default, the standard input and output of the shell is the terminal. The “> <path/to/utfile>” and “< <path/to/infile>” may be used to redirect the standard output to the file with pathname “<path/to/outfile” and standard input redirected from the file with pathname path/to/infile.
FLASH$ echo "Hello world" > out.txt, cat < out.txt
```
**Explanation: 

###### Environment variables 
```Bash
# Environment variables are in UPPERCASE and can be at most 16 characters long. The values that can be associated with these variables can be at most 240 characters long. A special 16th environment variable, “?” is used to remember the return value, i.e. the exit code, of the most recently executed command line.

#Set environment variable
FLASH$ set FOO=bar

# Get the value of an environment variable
FLASH$ get FOO
```

###### Exit 
```Bash
# Exit the shell. Unless given the exit command, the shell is in what is called the read-eval-print-loop (REPL)
FLASH$ exit
```

###### Change Directory 
```Bash
    # Change the current working directory
    FLASH$ cd /path/to/directory
```

## Explanations:

### Environment Variables

- Maximum of 15 environment variables, each with a maximum length of 16 characters for the name and 240 characters for the value.
- Special environment variable `?` stores the return value of the most recently executed command.
- Use `set` command to set up environment variables: `set FOO="bar"`.
- Use `get` command to retrieve the value of an environment variable: `get FOO`.

### Design Considerations
- Implemented in C for portability and efficiency.
- Utilizes hash table data structure for efficient storage and retrieval of environment variables.
- String manipulation techniques are used for parsing user input.
- Error handling mechanisms are incorporated throughout the code for diagnosing and reporting errors during execution.

Certainly! Let's delve deeper into the design considerations of the FLAme-SHell (flash) program:

1. **Efficient Data Structures**:
   - Hash Table for Environment Variables: Flash utilizes a hash table data structure to store environment variables efficiently. This allows for fast lookup and insertion of variables, ensuring optimal performance even with a large number of variables.
   - Linked Lists for Command Line Parsing: Linked lists are used to parse and store command line arguments, providing flexibility and scalability in handling variable-length input.

2. **Memory Management**:
   - Dynamic Memory Allocation: Flash dynamically allocates memory for storing command line arguments, environment variables, and other data structures. This helps optimize memory usage and prevents wastage of resources.
   - Proper Deallocation: Memory allocated during runtime is carefully deallocated to prevent memory leaks and ensure efficient resource utilization. This includes freeing memory for parsed command arguments, hash table nodes, and other dynamically allocated objects.

3. **Error Handling**:
   - System Call Errors: Flash incorporates error handling mechanisms for system calls such as fork, execvp, open, etc. Error messages are displayed using perror or fprintf(stderr, ...) to provide detailed information about the cause of failure, aiding in troubleshooting and resolution.
   - Input Validation: User input is thoroughly validated to prevent unexpected behaviour, buffer overflow vulnerabilities, and security breaches. Input strings are checked for length, format, and correctness to ensure safe and reliable operation of the shell.

4. **Signal Handling**:
   - Graceful Termination: Flash implements signal handlers to respond to signals like SIGINT and SIGTSTP gracefully. This allows the shell and its child processes to terminate or suspend properly in response to user actions, enhancing user experience and system stability.

5. **Robust Command Execution**:
   - Command Parsing and Execution: Flash parses user commands and executes them based on their type, supporting various command line formats such as single commands, piped commands, background commands, and command sequences. Robust command execution logic ensures accurate interpretation and reliable execution of user input.
   - Redirection and Background Execution: Flash supports input and output redirection using < and > symbols, as well as background execution using # symbol at the end of the command. This allows users to customize command behaviour and execute commands asynchronously when needed.


### Error Handling:
- System Call Errors: Error handling is implemented for various system calls such as fork, execvp, open, getpwuid, gethostname, getcwd, etc. Error messages are printed using perror or fprintf(stderr, ...) to indicate the cause of failure, allowing users to diagnose and address issues effectively.

- Memory Allocation Errors: Checks are performed for memory allocation errors using malloc. If memory allocation fails, an error message is printed, and the program exits gracefully to prevent undefined behaviour due to insufficient memory.

- Input Validation: User input is validated to prevent unexpected behaviour or security vulnerabilities. Input strings are checked for length and format to avoid buffer overflow vulnerabilities and ensure proper parsing of commands.

- Command Execution Errors: Return values of execvp are checked after attempting to execute a command. If command execution fails, an error message is printed to inform the user about the issue, facilitating troubleshooting and resolution.

- Signal Handling: Signal handlers are implemented to handle signals such as SIGINT and SIGTSTP gracefully. This allows the shell and its child processes to terminate or suspend properly in response to user actions.

- Environment Variable Handling: Environment variable names and values are validated before insertion into the hash table. Checks are performed to ensure that variable names adhere to the specified format and that values do not exceed the maximum allowed length.

- Resource Cleanup: Dynamically allocated memory and file descriptors are freed and closed before exiting the shell. This prevents memory leaks and ensures that resources are released properly, contributing to efficient resource management.


### Making it look like Shell
- to make it look like a Linux shell, the current directory, hostname and username are being found and printed.

### Parsing and Handling User Input

The flash program parses and handles the string provided by the user to execute commands and perform various operations. 
Here's an overview of how user input is processed:

1. Command Parsing
When the user enters a command, the FLASH program parses it into individual tokens based on spaces and commas. It uses the parseSpace function to tokenize the input string.

```Bash
        #Example
        char inputString[MAXCOM];
        takeInput(inputString); #Function to read user input
        execCommandSeq(inputString); #Function to execute parsed commands
```


2. Command Execution
After parsing, the flash program executes the commands based on their type. 
It supports single commands, piped commands, and built-in commands like cd, set, and get.

```Bash
        #Example
        char* parsedArgs[MAXLIST]; #Array to store parsed arguments
        char* parsedArgsPiped[MAXLIST]; #Array to store parsed arguments for piped commands
        int background = 0; #Flag to indicate background execution
        processString(inputString, parsedArgs, parsedArgsPiped, &background); #Parse and process the input string
```

3. Built-in Commands

The flash program supports built-in commands like cd, set, and get for changing directories and managing environment variables.
```Bash
        #Example
        if (strncmp(parsed[0], "set", 3) == 0) {
                setEnvVar(parsed); // Function to set environment variables
        } else if (strncmp(parsed[0], "get", 3) == 0) {
                getEnvVar(parsed); // Function to get environment variables
        }

```

4. Redirection and Background Execution

The shell program also supports input and output redirection using < and > symbols, as well as background execution using # symbol at the end of the command.

```Bash
        #Example
        FLASH$ "ls -l > output.txt" #Redirect output of ls command to output.txt file
        FLASH$ "ls -l &" #Execute ls command in background

```

### External Commands

The shell program supports executing a wide range of commands, including both external commands and built-in commands. External commands are those that are not directly implemented within the shell program but are instead separate binary executable files located in directories listed in the PATH environment variable.

#### Behavior: 
The shell program utilizes the execvp() function to execute external commands. This function searches for the corresponding binary executable file for the entered command in the directories listed in the PATH environment variable. If the binary file is found, it is executed, and the output (if any) is displayed in the shell. If the command is not found or encounters an error during execution, an appropriate error message is displayed.

This explanation provides an overview of how the shell program interacts with external commands and helps users understand the behaviour of the shell when executing commands like date, echo, and man.
