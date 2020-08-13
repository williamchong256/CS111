//NAME: William Chong
//EMAIL: williamchong256@gmail.com
//ID: 205114665



#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

void seghandler() {
    //exit with exit code of 4
    fprintf(stderr, "Segmentation fault detected.\n");
    exit(4);
}

int main(int argc, char * argv[]) {
    // insert code here...
    
    //to store the index of the long option used
    int optionindex = 0;
    char* input=NULL;
    char* output=NULL;
    int segfault = 0;
    
    //long options
    static struct option inputOptions[] = {
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"segfault", no_argument, 0, 's'},
        {"catch", no_argument, 0, 'c'},
        {0,0,0,0}
    };
    
    int opt = getopt_long(argc, argv, "", inputOptions, &optionindex);
    
    while (opt != -1) {
        switch(opt)
        {
            case 'i':
                //store the argument
                input = optarg;
                break;
                
            case 'o':
                output = optarg;
                break;
                
            case 's':
                //set segfault to 1, raise flag to indicate force a segfault
                segfault = 1;
                break;
            case 'c':
                //setup signal handler to register sigsegv handler
                signal(SIGSEGV, seghandler);
                
                break;
            default:
                fprintf(stderr, "Invalid argument. Correct usage: ./lab0 --input filename --output filename [--segfault] [--catch]\n");
                exit(1);
        }
        opt = getopt_long(argc, argv, "", inputOptions, &optionindex);
    }
    
    //input redirection
    if (input) {
        int ifd = open(input, O_RDONLY);
        //if successfully open file
        if (ifd >= 0) {
            close(0);
            dup(ifd); //set fd 0 to this inputfile
            close(ifd); //close the duplicate
        }
        else {
            //error opening the file, print which file, and error message, exit with exit code 2
            fprintf(stderr, "[--input] option error. Could not open the input file: %s\n", input);
            fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
            exit(2);
        }
    }
    
    //output redirection
    if (output) {
        int ofd = creat(output, 0666);
        if (ofd >= 0) {
            close(1);
            dup(ofd);
            close(ofd);
        }
        else {
            //error creating the file, print which file, and error message, exit with exit code 3
            fprintf(stderr, "[--output] option error. Could not create the file: %s\n", output);
            fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
            exit(3);
        }
    }
    
    //cause segfault if option passed in
    if (segfault){
        //try to access a null pointer
        char* ptr = NULL;
        *ptr = 'a';
    }
    
    
    //copy from input file to output file
    char buf;
    while ((read(0, &buf, sizeof(char))) >0) {
        if (write(1, &buf, sizeof(char))<0) {
            fprintf(stderr,"Could not write to the output file: %s\n", strerror(errno));
            exit(3);
        }
    }
    
    exit(0);
}
