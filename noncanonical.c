// Non-canonical input processing

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B9600
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FALSE 0
#define TRUE 1
#define F 0x7E
#define A 0x02
#define C 0x07
#define Bcc1 A^C
#define BUF_SIZE 256

volatile int STOP = FALSE;

int main(int argc, char** argv)
{
    // Program usage: Uses either COM1 or COM2
    if ((argc < 2))
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0], argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) {
        perror(argv[1]);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;  // Inter-character timer unused
    newtio.c_cc[VMIN]  = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred  to
    // by  fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Loop for input
    char buf[BUF_SIZE + 1]; // +1: Save space for the final '\0' char

    while (STOP == FALSE) {
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, BUF_SIZE);
        

        printf(":%x %x %x %x %x:%d\n", buf[0],buf[1],buf[2],buf[3],buf[4],bytes);

        if (bytes>0){
            int flag_counter=0;
            for(int i=0;i<bytes;i++){
                if(buf [i]==F){
                    flag_counter++;
                }
            }
            if(flag_counter==2){
                char response[5]={F,A,C,Bcc1,F};
                int UAbytes=write(fd,response,5);
                 printf(":%x %x %x %x %x:%d\n", response[0],response[1],response[2],response[3],response[4],UAbytes);
            }
            
        }
    }


    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
