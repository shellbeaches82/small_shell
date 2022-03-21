Rachelle Bass

Portfolio assignment from Operating Systems class written in C.  

This program is an implimentation of a small shell with the following functionality
 -a prompt for running commands
 -ignores blank lines and comments (lines beginning with the # character)
 -executes commands "exit", "cd", and "status"
 -allows foreground vs background commands 
 -file I/O redirection
 -executes various commands by creating new processes using exec functions
 -monitors and prints status of child processes
 -checks for "$$" anywhere in command and expands variable with pid in place of "$$"

Instructions:
*program testing was done in VS Code with C extention using gcc compiler on os1 with remote ssh.

*In terminal, compile as:
    gcc --std=gnu99 -o smallsh smallsh.c

*To execute: 
    ./smallsh
