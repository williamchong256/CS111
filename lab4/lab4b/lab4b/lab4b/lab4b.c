//NAME: William Chong
//EMAIL: williamchong256@gmail.com
//ID: 205114665

#define _POSIX_C_SOURCE  200809L

#include <stdlib.h>
#include <stdio.h>
#include <mraa.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <ctype.h>
#include <dos.h>


int scaleflag = 0;
char temp_scale = 'F';  //default to Fahrenheit
int periodflag = 0;
int period = 1;         //default to sample rate of 1 sec
int run_flag = 1;
int logflag = 0;
char logfile[256];
int logfd;
int reportwrite = 1;

const int B_val = 4275;

//sensors
mraa_aio_context temp_sensor;
mraa_gpio_context button;

void interrupt_handler()
{
    run_flag = 0;  //set runflag to 0, exits out of loop
}

double getTemp()
{
    int read = mraa_aio_read(temp_sensor);
    int B_val = 4275;
    double temp = 1023.0 / ((double)read) - 1.0;
    temp *= 100000.0;
    double C = 1.0 / (log(temp/100000.0)/B_val + 1/298.15) - 273.15;
    switch (temp_scale){
        case 'C':
            return C;
        default:  //default F
            return C * 9.0/5.0 + 32;
    }
}

void commandHandler(const char* command)
{
    if (strcmp(command, "SCALE=F") == 0) {
        if (logflag)
        {
            dprintf(logfd, "%s\n", command);
        }
        temp_scale = 'F';
    }
    else if (strcmp(command, "SCALE=C") == 0) {
        if (logflag)
        {
            dprintf(logfd, "%s\n", command);
        }
        temp_scale = 'C';
    }
    else if (strncmp(command, "PERIOD=", 7*sizeof(char)) == 0) {
        if (logflag)
        {
            dprintf(logfd, "%s\n", command);
        }
        int i=7;
        int len=strlen(command);
        if (command[7] != '\0') //if not empty command
        {
            while (i < len)
            {
                if (!isdigit(command[i]))
                {
                    return;
                }
                i++;
            }
            period =  atoi(&command[7]);
            if (period == 0)
            {
                //invalid period
                period = 1; //reset to default
            }
        }
    }
    else if (strcmp(command, "STOP") == 0) {
        if (logflag)
        {
            dprintf(logfd, "%s\n", command);
        }
        //stop generating reports
        reportwrite = 0;
    }
    else if (strcmp(command, "START") == 0) {
        if (logflag)
        {
            dprintf(logfd, "%s\n", command);
        }
        reportwrite = 1;
    }
    else if (strncmp(command, "LOG ", 4*sizeof(char)) == 0) {
        if (logflag)
        {
            dprintf(logfd, "%s\n", command);
        }
    }
    else if (strcmp(command, "OFF") == 0) {
        if (logflag)
        {
            dprintf(logfd, "%s\n", command);
        }
        interrupt_handler(); //same as pressing button
    }
}

void generateReport(const double tempval)
{
    time_t t;
    struct tm *current;
    //get the raw current time
    time(&t);
    current = localtime(&t);
    
    if (reportwrite)
    {
        fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n", current->tm_hour, current->tm_min, current->tm_sec, tempval);
        
        if (logflag)
        {
            dprintf(logfd,"%.2d:%.2d:%.2d %.1f\n", current->tm_hour, current->tm_min, current->tm_sec, tempval);
        }
    }
}

void shutdown_handler()
{
    time_t t;
    struct tm *current;
    //get the raw current time
    time(&t);
    current = localtime(&t);

    fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", current->tm_hour, current->tm_min, current->tm_sec);
    
    if (logflag)
    {
        dprintf(logfd,"%.2d:%.2d:%.2d SHUTDOWN\n", current->tm_hour, current->tm_min, current->tm_sec);
    }

    
    //close file descriptors and sensors
    close(logfd);
    mraa_aio_close(temp_sensor);
    mraa_gpio_close(button);
}

int main(int argc, char * argv[]) {
    //setup signal handler for segmentation fault
    signal(SIGINT, interrupt_handler);
    
    //setup option struct
    static struct option options[] = {
        {"log", required_argument, 0, 'l'},
        {"scale", required_argument, 0, 's'},
        {"period", required_argument, 0, 'p'},
        {0,0,0,0}
    };
    
    int opt = getopt_long(argc, argv, "", options, NULL);
    
    while (opt != -1)
    {
        switch(opt) {
            case 'l':
                logflag = 1;
                if (strcpy(logfile, optarg) == NULL)  //store inputted filename into logfile
                {
                    fprintf(stderr, "Error getting filename.\n");
                    exit(1);
                }
                logfd = open(logfile, O_RDWR|O_CREAT|O_SYNC|O_TRUNC, 0666);
                if (logfd < 0)
                {
                    fprintf(stderr, "Error creating logfile.\n");
                    exit(1);
                }
                break;
            case 's':
                scaleflag = 1;
                temp_scale = optarg[0]; //get the character of the inputted argument
                break;
            case 'p':
                periodflag = 1;
                period = atoi(optarg);
                if (period==0)
                {
                    fprintf(stderr, "Invalid period.\n");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Invalid argument. Correct usage: ./lab4b --log=filename --scale=[F,C] --period=seconds\n");
                exit(1);
        }
        opt = getopt_long(argc, argv, "", options, NULL);
    }
    
    //declare pins
    temp_sensor = mraa_aio_init(1);
    button = mraa_gpio_init(60);
    
    mraa_gpio_dir(button, MRAA_GPIO_IN);
    //setup what to do on button press
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &interrupt_handler, NULL);
    
    //setup pollfd structure to poll keyboard for input
    struct pollfd polls[1];
    polls[0].fd = STDIN_FILENO; //keyboard
    polls[0].events = POLLIN | POLLHUP | POLLERR;
    
    char keyboard_buff[128];
    char copy_buff[128];
    ssize_t num=0;
    time_t begin;
    time_t current;
    double temp=0;
    int copyindex = 0;
    int firsttime = 1; //only true for first run
    
    while (run_flag)
    {
        if (firsttime) //only delay for first time to allow piped input to be read
        {
            sleep(3); //delay 3 seconds
        }
        firsttime=0; //never repeat delay after first run
        
        
        time(&begin);
        time(&current);
        
        //generate and convert temperature value
        temp = getTemp();
        //generate report
        generateReport(temp);
        
        
        //loop continues to delay until period is reached,
        //then breaks and generates temp output
        while (difftime(current, begin) < period)
        {
            if( poll(polls, 1, 0) < 0)
            {
                //error polling
                fprintf(stderr, "Error occured while polling.\n");
                exit(1);
            }
            
            if(polls[0].revents & POLLIN) //if keyboard has pending input, then read
            {
                num = read(STDIN_FILENO, keyboard_buff, 128);
                //read from keyboard to a buffer to store command
                if (num < 0)
                {
                    fprintf(stderr, "Error reading keyboard input.\n");
                    exit(1);
                }
                
                
                for (int i=0; i<num && copyindex < 128; i++)
                {
                    if (keyboard_buff[i] == '\n')
                    {
                        //send to command handler
                        commandHandler(copy_buff);
                        //reset copyindex and copy_buff
                        memset(copy_buff, 0, 128);
                        copyindex=0;
                    }
                    else {
                        //copy character to copy buff
                        copy_buff[copyindex] = keyboard_buff[i];
                        copyindex++;
                    }
                }
            }
            
            time(&current); //update the time
        }
        
        
    }
    
    //shutdown process
    shutdown_handler();
    
    exit(0);
}
