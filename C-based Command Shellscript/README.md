# C-based Command ShellScript

C-based shellscript interface that allows you to interact with the kernel of an operating system (limited comamands and flags).

Implemented in and for Linux.

Features:

- Indicates the current working directory using pwd.

- cd changes the working directory

- Creates a file by the command Cat > <filename> or nano <filename>.

- cat <filename> views the contents of the file.

- wc [options/flags] filename. The wc command supports the following flags wc -l <filename>, wc -c <filename>, wc -w <filename>.

- cp <file1> <file2> copies the file <file1> to file <file2>

- The shell supports pipe operator “|” between processes (single pipe).

- sort <filename>, sort -r <filename>. sort command sorts a file, and supports the -r flag. (printing the file after sort)

- grep Returns lines with a specific word/string/pattern in a file. grep [options/flags] [Pattern] <filename>. Supports in the 
following options(flags): grep -c [pattern] <filename>

- man [command name]. A man command have one argument (command name) and then it will display a manual only for the commands 
that implemented in the shell, and then it will display a manual for all the commands we requested to execute.

- exit terminates your shell process.

Run:

1. Compile the command_shellscript.c in Linux terminal with gcc.

2. Run ./command_shellscript to open the shellscript.
