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
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include "zlib.h"

//TODO:
//Initialize zlib streams
//• Create a socket
//• socket(2)
//• Identify server
//• gethostbyname(3)
//• connect(2) to server
//• Wait for input
//• poll(2) on keyboard and socket
//• Compress and decompress if necessary
//• shutdown(2) socket
//• Restore terminal modes

//to store prior terminal modes
struct termios priorattr;


//socket stuff
struct sockaddr_in server_address;
int sockfd;
int serv_len = sizeof(server_address);
#define BUFSIZE 256
int port;

int logflag=0;
int logfd;
char* logname=NULL;
int compressflag=0;
z_stream out_stream;
z_stream in_stream;



void resetAttr (void)
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &priorattr) < 0)
    {
        fprintf(stderr, "Error setting attributes.\n");
        exit(1);
    }
}

void end_streams(void)
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
    
    //setup poll struct; poll keyboard and socket (server)
    struct pollfd polls[2];
    polls[0].fd = STDIN_FILENO; //keyboard
    polls[0].events = POLLIN | POLLHUP | POLLERR;
    polls[1].fd = sockfd; //from server
    polls[1].events = POLLIN | POLLHUP | POLLERR;
    
    
    int rval=0;  //for return value of poll
    //loop to call poll and read from fd only if it has pending input (revents)
    while(1)
    {
        if((rval = poll(polls, 2, 0)) < 0)
        {
            fprintf(stderr, "Error occured while polling.\n");
            exit(1);
        }
        
        //now check which fd has pending input
        
        //if keyboard has pending input, read it, then echo and fwd to server
        if(polls[0].revents & POLLIN)
        {
            char compressbuf[256]; //buffer to store for compression
            charCount = read(STDIN_FILENO, &buf, BUFSIZE);
            if (charCount == 0)
            {
                //EOF received from server.
                break;
            }
        
            for (int i=0; i<charCount; i++)
            {
                current = buf[i];
                switch(current)
                {
                    case '\r':
                    case '\n':
                        //echo <CR> or <LF> as CRLF to stdout, fwd as LF
                        if (write(STDOUT_FILENO, &crlf, 2*sizeof(char)) < 0)
                        {
                            fprintf(stderr,"Error while writing.\n");
                            exit(1);
                        }
                        
                        if (compressflag) {
                            compressbuf[i] = crlf[1];
                        }
                        else //no compress option
                        {
                            //map <cr> or <lf> to <lf> when forwarding to server
                            if (write(sockfd, &crlf[1], sizeof(char)) < 0)
                            {
                                fprintf(stderr, "Error while writing input to server socket.\n");
                                exit(1);
                            }
                            //if log opt is entered, write to the file
                            if (logflag)
                            {
                                //log sent bytes
                                if (dprintf(logfd, "SENT %d bytes: ", 1) < 0)
                                {
                                    fprintf(stderr, "Error writing to logfile.\n");
                                    exit(1);
                                }
                                if (write(logfd, &crlf[1], sizeof(char)) < 0)
                                {
                                    fprintf(stderr, "Error writing to logfile.\n");
                                    exit(1);
                                }
                                if (write(logfd, "\n", sizeof(char)) < 0)
                                {
                                    fprintf(stderr, "Error writing to logfile.\n");
                                    exit(1);
                                }
                            }
                        }
                        break;
                    default:
                        //echo to stdout, then fwd to server
                        if(write(STDOUT_FILENO, &current, sizeof(char)) < 0)
                        {
                            fprintf(stderr, "Error while writing to stdout.\n");
                            exit(1);
                        }
                        if (compressflag) {
                            compressbuf[i] = current;
                        }
                        else { //no compress, just fwd to server
                            if (write(sockfd, &current, sizeof(char)) < 0)
                            {
                                fprintf(stderr, "Error while writing input to server socket.\n");
                                exit(1);
                            }
                            
                            //if log opt is entered, write to the file
                            if (logflag)
                            {
                                //log sent bytes
                                if (dprintf(logfd, "SENT %d bytes: ", 1) < 0)
                                {
                                    fprintf(stderr, "Error writing to logfile.\n");
                                    exit(1);
                                }
                                if (write(logfd, &current, sizeof(char)) < 0)
                                {
                                    fprintf(stderr, "Error writing to logfile.\n");
                                    exit(1);
                                }
                                if (write(logfd, "\n", sizeof(char)) < 0)
                                {
                                    fprintf(stderr, "Error writing to logfile.\n");
                                    exit(1);
                                }
                            }
                        }
                        break;
                }
            }
            //if compress, then do compression and write to socket
            if (compressflag)
            {
                out_stream.next_in = (unsigned char *) buf;
                out_stream.avail_in = charCount;
                out_stream.next_out = (unsigned char *) compressbuf;
                out_stream.avail_out = 256;
                
                do{
                    if( deflate(&out_stream, Z_SYNC_FLUSH) != Z_OK)
                    {
                        fprintf(stderr, "Error while compressing outstream data.\n");
                        exit(1);
                    }
                } while ( out_stream.avail_in > 0);
                
                
                //write to socket
                if (write(sockfd, &compressbuf, 256 - out_stream.avail_out) < 0)
                {
                    fprintf(stderr,"Error writing to socket.\n");
                    exit(1);
                }
                
                //if log option, then write compressed data to logfile
                if (logflag)
                {
                    //log sent bytes
                    if (dprintf(logfd, "SENT %d bytes: ", 256-out_stream.avail_out) < 0)
                    {
                        fprintf(stderr, "Error writing to logfile.\n");
                        exit(1);
                    }
                    if (write(logfd, &compressbuf, 256-out_stream.avail_out) < 0)
                    {
                        fprintf(stderr, "Error writing to logfile.\n");
                        exit(1);
                    }
                    if (write(logfd, "\n", sizeof(char)) < 0)
                    {
                        fprintf(stderr, "Error writing to logfile.\n");
                        exit(1);
                    }
                }
            }
        }
        
        //check if server socket has pending input, then read, and output to stdout
        if (polls[1].revents & POLLIN)
        {
            charCount = read(sockfd, &buf, BUFSIZE);
            char decompressbuf[1024];
            
            if(charCount < 0)
            {
                fprintf(stderr, "Error reading from socket.\n");
                exit(1);
            }
            if (charCount == 0)
            {
                //EOF received from server.
                break;
            }
            
            //if log option, then write received data to logfile, before decompress
            if (logflag)
            {
                if (dprintf(logfd, "RECEIVED %zd bytes: ", charCount) < 0)
                {
                    fprintf(stderr, "Error writing to logfile.\n");
                    exit(1);
                }
                if(write(logfd, &buf, charCount) < 0)
                {
                    fprintf(stderr, "Error writing to logfile.\n");
                    exit(1);
                }
                if (write(logfd, "\n", sizeof(char)) < 0)
                {
                    fprintf(stderr, "Error writing to logfile.\n");
                    exit(1);
                }
            }
            
            //decompress if necessary, then can write to screen
            if (compressflag)
            {
                
                in_stream.next_in = (unsigned char *) buf;
                in_stream.avail_in = charCount;
                in_stream.next_out = (unsigned char *) decompressbuf;
                in_stream.avail_out = 1024;
                
                do{
                    if( inflate(&in_stream, Z_SYNC_FLUSH) != Z_OK)
                    {
                        fprintf(stderr, "Error while decompressing instream data.\n");
                        exit(1);
                    }
                } while (in_stream.avail_in > 0 );
                
                charCount = 1024 - in_stream.avail_out; //update the number of characters in the buffer to be processed
            }
            
            for(int i=0; i<charCount; i++)
            {
                if (compressflag)
                {
                    current = decompressbuf[i]; //if compress, then use the decompressed buffer
                }
                else{
                    current = buf[i];
                }
                switch(current)
                {
                    case '\r':
                    case '\n':
                        if ( write(STDOUT_FILENO, &crlf, 2*sizeof(char)) < 0 )
                        {
                            fprintf(stderr, "Error writing to output.\n");
                            exit(1);
                        }
                        break;
                    default:
                        if ( write(STDOUT_FILENO, &current, sizeof(char)) < 0 )
                        {
                            fprintf(stderr, "Error writing to output.\n");
                            exit(1);
                        }
                        break;
                }
            }
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
        
        //check if there is polling error from server, if there is, then no more data from shell
        //shutdown condition!
        if (polls[1].revents & (POLLHUP | POLLERR))
        {
            break;
        }
    }
}


int main(int argc, char * argv[])
{
    int optionindex=0;
    int portnum = -1;
    
    struct hostent* server;  //for server name/address
    
    //for saving/setting terminal attributes
    struct termios attr;
    
    static struct option inputOptions[] = {
        {"port", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {"compress", no_argument, 0, 'c'},
        {0,0,0,0}
    };
    
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
            case 'l':
                //set log flag, and store filename
                logname = optarg;
                logflag = 1;
                logfd = creat(logname, 0666);
                if (logfd < 0)
                {
                    fprintf(stderr,"Error creating log file.\n");
                    exit(1);
                }
                break;
            case'c':
                //set compress flag
                compressflag = 1;
                
                //initialize compression streams
                //outstream is the stream to server
                out_stream.zalloc = Z_NULL;
                out_stream.zfree = Z_NULL;
                out_stream.opaque = Z_NULL;
                if(deflateInit(&out_stream, Z_DEFAULT_COMPRESSION) != Z_OK)
                {
                    fprintf(stderr, "Error creating compression stream.\n");
                    exit(1);
                }
                
                //instream is stream from server
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
                fprintf(stderr, "Invalid argument. Correct usage: ./lab1b-client --port=portnum --log=filename [--compress]\n");
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
    
    
    //    SERVER SETUP AND CONNECT
    
    //setup socket and connect to server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr,"Error creating socket.\n");
        exit(1);
    }
    
    memset((char*) &server_address, 0, serv_len);
    
    //setup sockaddr_in struct
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portnum);
    
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "Error finding localhost.\n");
        exit(1);
    }
    //assign address to server
    memcpy((char*)&server_address.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
    
    //• connect(2) to server
    if (connect(sockfd, (struct sockaddr *) &server_address, serv_len) < 0)
    {
        fprintf(stderr, "Error connecting to server.\n");
        exit(1);
    }
    
    //do regular read and write
    reg_read_write();
    exit(0);

}

