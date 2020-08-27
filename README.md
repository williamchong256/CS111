# CS111 Operating Systems Repository

This is a repository of my projects for UCLA's CS111 Operating System Principles course.

Here is a brief summary of the labs:

## Lab0: 

  - An introductory project that copies from an input file or stdin, to a specified output file or stdout. Applies option parsing using getopt_long().

## Lab1: 

  - In part A of this lab, I change terminal IO mode to character-at-a-time, full duplex. Optionally, the program can fork a child process (to run a shell) and polls the keyboard for input. Input/output is appropriately passed from parent process to child process with pipe communication.

  - In part B, I setup a server and client that communicate via a TCP socket. The server polls for input from the client over the socket, and pipes any input to a child process (a shell) to run the commands. Optionally, the data sent over the socket between client and server can be compressed/decompressed.


