# Custom-Unix-Shell

## Overview

This project contains the implementations of a Unix-like shell interface in C, allowing users to execute commands, manage processes, and utilize advanced features such as command history, input/output redirection, and inter-process communication via piping.

## Features

- **Command Execution**: Executes user-entered commands using `fork()`, `execvp()`, and `wait()` system calls.
  
- **Command History**: Supports command history management, allowing users to repeat previous commands using `hist`.

- **Input/Output Redirection**: Implements `<` and `>` operators for redirecting command input from and output to files using `dup2()`.

- **Piping**: Enables command chaining by passing the output of one command as the input to another using the `pipe()` system call.

## Project Structure

- **`shell.c`**: Contains the main implementation of the Unix-like shell, including command parsing, execution, and feature implementations.
