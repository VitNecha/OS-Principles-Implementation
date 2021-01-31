# OS Memory Management Implementation

OS memory management implemented in C using input.txt file (for reading commands) and output.txt file (for prints).

**Available commands:** 

>- write [memory cell integer] [character] : writes the chosen character into chosen memory cell (number of characters can be attached 
>to the same cell, result -> string).
>
>- read [memory cell integer] : reads from chosen memory cell
>
>- print : prints the current memory status (all cells with their values)

**Available methods:**

>- 1 : LRU - Least Recently Used
>
>- Other integer ( > 0 ) : FIFO SC - First In First Out Second Chance

**Run:**
>1. Compile the memoryManagement.c file with gcc.
>
>2. Using Linux terminal run the following command:
>
>./memoryManagement [method] [input file] [output file] [main memory size] [secondary memory size]

*** Input file example is in GitHub repository.
