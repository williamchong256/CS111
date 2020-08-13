//NAME: William Chong
//EMAIL: williamchong256@gmail.com
//ID: 205114665

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/in.h>
#include <string.h>
#include "zlib.h"


//TODO:
//• Initialize zlib streams
//• Create a socket
//• socket(2)
//• Fill server’s sockaddr_in struct INADDR_ANY
//• bind(2) socket to name
//• Establish connection with client
//• listen(2), accept(2)
//• Input/output forwarding
//• poll(2) on socket and shell, decompress if necessary

//for the process; pipes and pid
int from_child[2];
int to_child[2];
pid_t childPID;

//socket stuff
struct sockaddr_in server_address;
struct sockaddr_in client_address;
int sockfd, new_sockfd;
int serv_len = sizeof(server_address);
int client_len = sizeof(client_address);
#define BUFSIZE 1024
int port;

//compression, zlib stuff
int compressflag = 0;
z_stream out_stream;  //stream to client
z_stream in_stream;   //stream from client


void signal_handler(int signum)
{
    if (signum == SIGPIPE)
    {
        fprintf(stderr, "SIGPIPE signal received. Exiting...\n");
        exit(0);
    }
    
    if (signum == SIGINT)
    {
        int val = kill(childPID, SIGINT);
        if(val < 0){
            fprintf(stderr, "Error killing process.\n");
            exit(1);
        }
        fprintf(stderr,"SIGINT received. Exiting...\n");
        exit(0);
    }
    
    if (signum == SIGTERM)
    {
        fprintf(stderr, "SIGTERM signal received. Exiting...\n");
        exit(0);
    }
}

void close_socket()
{
    close(sockfd);
    close(new_sockfd);
}

void end_streams()
{
    deflateEnd(&out_stream);
    inflateEnd(&in_stream);
}

void reg_read_write(void)
{
    //for read/write
    char buf[BUFSIZE];  //create a buf with size BUFSIZE bytes
    ssize_t charCount;
    char current;
    char crlf[2] = {'\r', '\n'};

    //read to buffer
    while(1)
    {
        //read from client
        charCount = read(new_sockfd, &buf, BUFSIZE*sizeof(char));
        for (int i=0; i<charCount; i++)
        {
            current = buf[i];
            switch(current) {
                case '\n':
                case '\r':
                    //map <CR> or <LF> to <CR><LF>
                    if(write(new_sockfd, &crlf, 2*sizeof(char))<0)  //write output to the socket
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
                    if (write(new_sockfd, &current, sizeof(char)) < 0)
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
    char buf[BUFSIZE];  //create a buf with size 256 bytes
    ssize_t charCount;
    char current;
    char crlf[2] = {'\r', '\n'};
    char decompressbuf[1024];
    
    //read from client via socket
    charCount = read(new_sockfd, &buf, BUFSIZE*sizeof(char));

    if(compressflag)
    {
        //decompress data from socket, so it becomes usable
        in_stream.next_in = (unsigned char *) buf;
        in_stream.avail_in = charCount;
        in_stream.next_out = (unsigned char *) decompressbuf;
        in_stream.avail_out = 1024;
        
        do {
            if (inflate(&in_stream, Z_SYNC_FLUSH) != Z_OK)
            {
                fprintf(stderr, "Error while decompressing.\n");
                exit(1);
            }
        } while (in_stream.avail_in > 0);
        
        //update the character count in the buffer that we are processing
        charCount = 1024-in_stream.avail_out;
        
    }
    
    for (int i =0; i<charCount; i++)
    {
        current = buf[i];
        switch(current) {
            case '\4':
                //close the pipe to child, but don't exit yet, keep processing input from child
                if (close(to_child[1]) < 0)
                {
                    fprintf(stderr, "Error closing pipe.\n");
                    exit(1);
                }
                break;
            case '\3':
                //kill child process by sending sig interrupt signal
                if ( kill(childPID, SIGINT) < 0 )
                {
                    fprintf(stderr, "Error with kill().\n");
                    exit(1);
                }
                break;
            case '\n':
            case '\r':
                //map <cr> or <lf> to <lf> when passing to shell
                if (write(to_child[1], &crlf[1], sizeof(char)) < 0)
                {
                    fprintf(stderr, "Error while writing input to shell.\n");
                    exit(1);
                }
                break;
            default:
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
    char buf[BUFSIZE];  //create a buf with size BUFSIZE bytes
    ssize_t charCount;
    
    char compressbuf[256];
        
    //read from child process
    if ((charCount = read(from_child[0], &buf, BUFSIZE*sizeof(char))) < 0)
    {
        fprintf(stderr, "Error reading from process.\n");
        exit(1);
    }
    
    //compress the data to be sent to client
    if (compressflag)
    {
        out_stream.next_in = (unsigned char *) buf;
        out_stream.avail_in = charCount;
        out_stream.next_out = (unsigned char *) compressbuf;
        out_stream.avail_out = 256;
        
        do{
            if(deflate(&out_stream, Z_SYNC_FLUSH) != Z_OK)
            {
                fprintf(stderr, "Error while compressing data.\n");
                exit(1);
            }
        } while (out_stream.avail_in > 0);
        
        //write to client
        if (write(new_sockfd, &compressbuf, 256-out_stream.avail_out) < 0)
        {
            fprintf(stderr, "Error writing to socket.\n");
            exit(1);
        }
        
    }
    else { //no compress
        //send all of buf to client
        if (write(new_sockfd, &buf, charCount) < 0)
        {
            fprintf(stderr, "Error writing to socket.\n");
            exit(1);
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
        
        //setup pollfd structures, one to poll the client, and one for the pipe from child to parent
        struct pollfd polls[2];
        polls[0].fd = new_sockfd; //set poll[0] to monitor the socketfd from client
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
            
            if(polls[0].revents & POLLIN) //if client has pending input, then read
            {
                //read from terminal (client in this case) and then forward to shell
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
                        
            //check for error with keyboard (client)
            if(polls[0].revents & (POLLHUP | POLLERR))
            {
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
                
                //close pipes and sockets
                if (close(from_child[0]) < 0)
                {
                    fprintf(stderr, "Error closing pipe.\n");
                    exit(1);
                }
        
                break;
                //will lead to exit(0), successful termination!
            }
        }
    }
}


int main(int argc, char * argv[])
{
    int optionindex=0;
    
    //for shell option, default to /bin/bash/
    char* shellpgm = "/bin/bash";
    int sflag = 0;
    static struct option inputOptions[] = {
        {"port", required_argument, 0, 'p'},
        {"shell", required_argument, 0, 's'},
        {"compress", no_argument, 0, 'c'},
        {0,0,0,0}
    };
    
    int portnum = -1;
    
    //get the option if passed in
    int opt = getopt_long(argc, argv, "", inputOptions, &optionindex);
    
    //get the option and store the program argument and specify whether or not sflag
    //is set
    while (opt != -1) {
        switch(opt){
            case 'p':
                //get the passed in port number
                portnum = atoi(optarg);
                break;
            case 's':
                shellpgm = optarg; //store the shell program argument
                sflag = 1; //if shell option is passed in, then set flag
                break;
            case 'c':
                compressflag=1;
                
                //initialize compression streams
                //outstream is the stream to client
                out_stream.zalloc = Z_NULL;
                out_stream.zfree = Z_NULL;
                out_stream.opaque = Z_NULL;
                if(deflateInit(&out_stream, Z_DEFAULT_COMPRESSION) != Z_OK)
                {
                    fprintf(stderr, "Error creating compression stream.\n");
                    exit(1);
                }
                
                //instream is stream from client
                in_stream.zalloc = Z_NULL;
                in_stream.zfree = Z_NULL;
                in_stream.opaque = Z_NULL;
                if (inflateInit(&in_stream) != Z_OK)
                {
                    fprintf(stderr, "Error creating decompression stream.\n");
                    exit(1);
                }
                
                atexit(end_streams);
                
                break;
            default:
                //unrecognized option
                fprintf(stderr, "Invalid argument. Correct usage: ./lab1b-server --port=portnum [--shell=program] [--compress]\n");
                exit(1);
        }
        
        opt = getopt_long(argc, argv, "", inputOptions, &optionindex);
    }
    
    //in order to make --port a mandatory option, check after while loop to see if there is a val stored
    //if not, then error!
    if (portnum == -1 || portnum <= 1024)
    {
        fprintf(stderr, "Please specify a port number (greater than 1024): [--port=portnum].\n");
        exit(1);
    }
    
    if (atexit(close_socket) < 0)
    {
        fprintf(stderr, "Error with atexit().\n");
        exit(1);
    }
    
    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error creating socket.\n");
        exit(1);
    }
    
    //initialize buffer to 0
    memset((char*)&server_address, 0, serv_len);
    
    //Fill server’s sockaddr_in struct INADDR_ANY
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;  //assign the IP addr of the server machine to the server
    server_address.sin_port = htons(portnum);  //convert to network byte order
    
    //bind(2) socket to name
    if ( bind(sockfd, (struct sockaddr *) &server_address, serv_len) < 0)
    {
        fprintf(stderr,"Error binding socket.\n");
        exit(1);
    }
    
    //Establish connection with client
    //listen(2), accept(2)
    if ( listen(sockfd, 5) < 0 )
    {
        fprintf(stderr, "Error listening to socket.\n");
        exit(1);
    }
    
    //store new fd for the socket
    new_sockfd = accept(sockfd, (struct sockaddr *) &client_address, (socklen_t * ) &client_len);
    if (new_sockfd < 0)
    {
        fprintf(stderr, "Error accepting connection.\n");
        exit(1);
    }
    
    
    
    
    //determine what behavior to do, depending on if --shell is activated
    if(sflag)
    {
        signal(SIGPIPE, signal_handler);
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        //then fork and create process
        create_process(shellpgm);
        
        exit(0);
    }
    else {
        //do regular read and write
        reg_read_write();
    }
}
