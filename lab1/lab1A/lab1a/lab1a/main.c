//NAME: William Chong
//EMAIL: williamchong256@gmail.com
//ID: 205114665

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>

//to store prior terminal modes
struct termios priorattr;

//for the process; pipes and pid
int from_child[2];
int to_child[2];
pid_t childPID;

void signal_handler(int signum)
{
    if (signum == SIGPIPE)
    {
        fprintf(stderr, "SIGPIPE signal received. Exiting...\n");
        exit(0);
    }
}

void resetAttr (void)
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &priorattr) < 0)
    {
        fprintf(stderr, "Error setting attributes.\n");
        exit(1);
    }
}


void reg_read_write(void)
{
    //for read/write
    char buf[256];  //create a buf with size 100 bytes
    ssize_t charCount;
    char current;
    char crlf[2] = {'\r', '\n'};

    //read to buffer
    while(1)
    {
        charCount = read(STDIN_FILENO, buf, 256*sizeof(char));
        for (int i=0; i<charCount; i++)
        {
            current = buf[i];
            switch(current) {
                case '\n':
                case '\r':
                    //map <CR> or <LF> to <CR><LF>
                    if(write(1, &crlf, 2*sizeof(char))<0)
                    {
                        fprintf(stderr,"Error writing to output.\n");
                        exit(1);
                    }
                    break;
                case '\4':
                    //received ^D signal, so terminate normally
                    exit(0);
                default:
                    //write single char to output
                    if (write(1, &current, sizeof(char)) < 0)
                    {
                        fprintf(stderr, "Error writing to output.\n");
                        exit(1);
                    }
                    break;
            }
        }
    }
}


void term_read_write(void)
{
    //for read/write
    char buf[256];  //create a buf with size 256 bytes
    ssize_t charCount;
    char current;
    char crlf[2] = {'\r', '\n'};
    char temp[2];
    
    //      keyboard: read (ASCII) input from the keyboard, echo it to stdout, and forward it to the shell. <cr>                or <lf> should echo as <cr><lf> but go to shell as <lf>
    //  When your program reads a ^C (0x03) from the keyboard, it should use kill(2) to send a SIGINT to the shell process. Note that the shell will not necessarily die as a result of receiving this signal. echo as "^C" or "^D" instead of directly back, easier.
    
    
    charCount = read(STDIN_FILENO, buf, 256*sizeof(char));
    for (int i =0; i<charCount; i++)
    {
        current = buf[i];
        switch(current) {
            case '\4':
                //echo as ^D
                temp[0] = '^';
                temp[1] = 'D';
                if(write(STDOUT_FILENO, &temp, 2*sizeof(char)) < 0)
                {
                    fprintf(stderr, "Error while writing to output.\n");
                    exit(1);
                }
                //close the pipe to child, but don't exit yet, keep processing input from child
                if (close(to_child[1]) < 0)
                {
                    fprintf(stderr, "Error closing pipe.\n");
                    exit(1);
                }
                break;
            case '\3':
                //echo as ^C
                temp [0] = '^';
                temp [1] = 'C';
                if (write(STDOUT_FILENO, &temp, 2*sizeof(char)) < 0)
                {
                    fprintf(stderr, "Error while writing to output.\n");
                    exit(1);
                }
                //kill child process by sending sig interrupt signal
                if ( kill(childPID, SIGINT) < 0 )
                {
                    fprintf(stderr, "Error with kill().\n");
                    exit(1);
                }
                break;
            case '\n':
            case '\r':
                //echo <cr><lf> to stdout
                if (write(STDOUT_FILENO, &crlf, 2*sizeof(char)) < 0)
                {
                    fprintf(stderr,"Error while writing.\n");
                    exit(1);
                }
                //map <cr> or <lf> to <lf> when passing to shell
                if (write(to_child[1], &crlf[1], sizeof(char)) < 0)
                {
                    fprintf(stderr, "Error while writing input to shell (1).\n");
                    exit(1);
                }
                break;
            default:
                //echo to stdout
                if ( write(STDOUT_FILENO, &current, sizeof(char)) < 0)
                {
                    fprintf(stderr, "Error writing to output.\n");
                    exit(1);
                }
                //write to child/shell
                if ( write(to_child[1], &current, sizeof(char)) < 0 )
                {
                    fprintf(stderr, "Error writing to output.\n");
                    exit(1);
                }
                break;
        }
    }

}


void shell_read_write(void)
{
    //for read/write
    char buf[256];  //create a buf with size 256 bytes
    ssize_t charCount;
    char current;
    char crlf[2] = {'\r', '\n'};

    //      shell: read input from the shell pipe and write it to stdout. If it receives an <lf> from the shell,                it should print it to the screen as <cr><lf>. Print only as many characters as are read.
    
    charCount = read(from_child[0], buf, 256*sizeof(char));  //read from child process
    for (int i =0; i<charCount; i++)
    {
        current = buf[i];
        switch(current) {
            case '\n':
            case '\r':
                if ( write(STDOUT_FILENO, &crlf, 2*sizeof(char)) < 0 )
                {
                    fprintf(stderr, "Error writing to output.\n");
                    exit(1);
                }
                break;
            default:
                if (write(STDOUT_FILENO, &current, sizeof(char)) < 0 )
                {
                    fprintf(stderr, "Error writing to output.\n");
                    exit(1);
                }
                break;
                
        }
    }
}


void create_process(char* program)
{
    //create the pipes
    if (pipe(to_child) == -1)
    {
        //error creating pipe to child process
        fprintf(stderr, "Error creating pipe to child.\n");
        exit(1);
    }
    
    if (pipe(from_child) == -1)
    {
        //error creating pipe from child to parent
        fprintf(stderr, "Error creating pipe from child to parent.\n");
        exit(1);
    }
    
    //fork
    if ((childPID = fork()) < 0)
    {
        fprintf(stderr, "Error while forking\n");
        exit(1);
    }
    
    if (childPID == 0)
    {
        //setup child process by closing unused ends
        //close write end of pipe to_child[2]
        if (close(to_child[1]) < 0)
        {
            fprintf(stderr, "Error closing pipe.\n");
            exit(1);
        }
        //close read end of pipe from_child[2]
        if (close(from_child[0]) < 0)
        {
            fprintf(stderr, "Error closing pipe.\n");
            exit(1);
        }
        
        //redirect stdin, stdout, and stderr to the appropriate pipes
        
        //duplicate the read end of pipe from parent to child as input
        dup2(to_child[0], STDIN_FILENO);
        //dup the write end of pipe from child to parent as output
        dup2(from_child[1], STDOUT_FILENO);
        //redirect stderr of child to from_child[1]
        dup2(from_child[1], STDERR_FILENO);
        //close the original (duplicate) pipes
        if (close(to_child[0]) < 0)
        {
            fprintf(stderr, "Error closing pipe.\n");
            exit(1);
        }
        if (close(from_child[1]) < 0)
        {
            fprintf(stderr, "Error closing pipe.\n");
            exit(1);
        }
        
        //execvp the specified --shell=program program argument
        char* args[2];
        args[0] = program; //pass in the specified program argument
        args[1] = NULL;
        
        if (execvp(program, args) < 0)
        //if (execl(program, program, (char*) NULL)  < 0)
        {
            fprintf(stderr, "Error execvp()\n");
            exit(1);
        }
        
    }
    else {
        //setup parent process with waitpid
        //this is the parent process
        if (close(to_child[0]) < 0)  //close read end of pipe to child
        {
            fprintf(stderr, "Error closing pipe.\n");
            exit(1);
        }
        if (close(from_child[1]) < 0) //close write end of pipe from child to parent
        {
            fprintf(stderr, "Error closing pipe.\n");
            exit(1);
        }
        
        //setup pollfd structures, one to poll the keyboard, and one for the pipe from child to parent
        struct pollfd polls[2];
        polls[0].fd = STDIN_FILENO; //keyboard
        polls[0].events = POLLIN | POLLHUP | POLLERR;
        polls[1].fd = from_child[0]; //from child process
        polls[1].events = POLLIN | POLLHUP | POLLERR;
        
        
        int rval=0;  //for return value of poll
        //loop to call poll and read from fd only if it has pending input (revents)
        while(1)
        {
            if((rval = poll(polls, 2, 0)) < 0)
            {
                //error polling
                fprintf(stderr, "Error occured while polling.\n");
                exit(1);
            }
            
            //now check which fd has pending input
            
            if(polls[0].revents & POLLIN) //if keyboard has pending input, then read
            {
                //read from terminal, echo to stdout, then forward to shell
                term_read_write();
            }
            //check if shell has pending input, then read
            if (polls[1].revents & POLLIN)
            {
                //read from shell and then output to stdout
                shell_read_write();
            }
            
            //after checking if there is still output from shell (in case of final poll situation)
            //so as to make sure all input is processed, and all output outputted.
                        
            //check for error with keyboard
            if(polls[0].revents & (POLLHUP | POLLERR))
            {
                //SOME ERROR, HANDLE IT
                fprintf(stderr, "Error with keyboard polling.\n");
                break;
            }
            
            //check if there is polling error from shell, if there is, then no more data from shell
            //shutdown condition!
            if (polls[1].revents & (POLLHUP | POLLERR))
            {
//                close(from_child[0]);
                int stat;
                if (waitpid(childPID, &stat, 0) < 0)
                {
                    fprintf(stderr, "Error with waitpid().\n");
                    exit(1);
                }
                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(stat), WEXITSTATUS(stat) );
                if (close(from_child[0]) < 0)
                {
                    fprintf(stderr, "Error closing pipe.\n");
                    exit(1);
                }
                break;
            }
        }
    }
}


int main(int argc, char * argv[])
{
    int optionindex=0;
    
    //for shell option
    char* shellpgm;
    int sflag = 0;
    struct termios attr;
    static struct option inputOptions[] = {
        {"shell", required_argument, 0, 's'},
        {0,0,0,0}
    };
    
    //get the option if passed in
    int opt = getopt_long(argc, argv, "", inputOptions, &optionindex);
    
    //get the option and store the program argument and specify whether or not sflag
    //is set
    while (opt != -1) {
        switch(opt){
            case 's':
                shellpgm = optarg; //store the shell program argument
                sflag = 1; //if shell option is passed in, then set flag
                break;
            default:
                //unrecognized option
                fprintf(stderr, "Invalid argument. Correct usage: ./lab1a [--shell=program]\n");
                exit(1);
        }
        
        opt = getopt_long(argc, argv, "", inputOptions, &optionindex);
    }
    
    
    //check if in terminal
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Error: standard input is not a terminal.\n");
        exit(1); //exit with 1 to indicate error.
    }
    
    //save the terminal's current modes
    if (tcgetattr(STDIN_FILENO, &priorattr)<0)
    {
        fprintf(stderr, "Error saving attributes.\n");
        exit(1);
    }
    atexit(resetAttr);
    
    //set the attributes we want
    //store into termios object attr
    if (tcgetattr(STDIN_FILENO, &attr) < 0)
    {
        fprintf(stderr, "Error getting attributes.\n");
        exit(1);
    }
    attr.c_iflag = ISTRIP;
    attr.c_oflag = 0;
    attr.c_lflag = 0;
    
    //set terminal immediately
    if (tcsetattr(STDIN_FILENO, TCSANOW, &attr) < 0)
    {
        fprintf(stderr, "Error setting attributes.\n");
        exit(1);
    }
    
    
    //determine what behavior to do, depending on if --shell is activated
    if(sflag)
    {
        signal(SIGPIPE, signal_handler);
        //then fork and create process
        create_process(shellpgm);
        
        exit(0);
    }
    else {
        //do regular read and write
        reg_read_write();
    }
}
