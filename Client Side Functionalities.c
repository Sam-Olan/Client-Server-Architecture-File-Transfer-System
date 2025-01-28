#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// User defined strucutre to hold packet payload and number of bytes read
typedef struct sourceContent{
    int bytesRead;
    char payload[1024];
}sourceContent;

// Function declarations 
void checkFilename(int, char *[]);
void checkFifos();
void sendMessage(char *);
void serialize(sourceContent *, char *);
int recieveMessage();


int main(int argc, char* argv[]){
    printf("Client\n");

    checkFilename(argc, argv);

    checkFifos();
    
    struct sourceContent sourceData;

    sendMessage(argv[1]);

    serialize(&sourceData, argv[1]);

    // Call recieveMessage function will wait for a message from the server. 
    // Print appropriate message depending on message recieved and exit.
    if (recieveMessage() == 0){
        printf("OK. File Uploaded. Client exiting\n");
        return 0;
    }
    else {
        printf("Some Error. Server did not send ALL DONE after file upload.\nClient exiting\n");
        return 1;
    }
}

// Function Name:  checkFilename
// Purpose:        Handles user input (command line parameters). Ensures that the user enters a file name, and ensures that the file exists
//                 Ensures that the user does not access a FIFO file      
// Parameters:     argc: Contains the count of command line parameters. Used to ensure user has entered exactly one command line argument (the filename)
//                 argv[]: An array that contains the command line parameters. Used to check the filename provided by the user
void checkFilename(int argc, char *argv[]) {
    // If the user did not enter exactly one command line parameter. (2 command line arguments, since the first entry in argv is the name of the executable)
    if (argc != 2){ 
        printf("You must enter a filename to be uploaded as a command line argument\n");
        exit(1);
    }
    // IF the file provided by the user does not exist, print error and exit
    if (access(argv[1], F_OK) == -1) {
        printf("The file: \"%s\" does not exits.\nIf the file does not exist in the current directory, be sure to include the file path\n", argv[1]);
        exit(2);
    }
    // If the user entered a FIFO as a command line parameter, print error and exit
    else if (strcmp(argv[1], "fifoServerToClient") == 0 || strcmp(argv[1], "fifoClientToServer") == 0){
        printf("The file: \"%s\" is a FIFO. Please choose another file that is not a FIFO\n", argv[1]);
        exit(3);
    }
}

// Function Name:  checkFifos
// Purpose:        Checks if there are FIFOs in the current directory. Since they are required for communication with the server, exit if either do not exist           
void checkFifos(){
    // If fifo does't exist, create it
    if (access("fifoClientToServer", F_OK) == -1) {
        printf("The Client->Server FIFO did not exist. Client exiting\n");
        exit(1);
    }
    // If fifo does't exist, create it
    if (access("fifoServerToClient", F_OK) == -1) {
        printf("The Server->Client FIFO does not exist. Client exiting\n");
        exit(2);        
    }
}

// Function Name:  sendMessage
// Purpose:        Used to send messages from the client to server over a FIFO. Used here to send name of file to be uploaded to server
// Parameters:     message: Contains the file name, provided by the user through command line argument
void sendMessage(char *message) {
    // Open fifo Server to Client. If there is an error opening, print error message and exit. Write only, as messages are only sent from here
    int fd = open("fifoServerToClient", O_WRONLY);
    if (fd == -1) {
        printf("error opening fifoServerToClient\n");
        exit(3);
    }
    // Write the name of the file to fifo Server to Client. If there is an error writing, print error and exit
    if (write(fd, message, strlen(message) + 1) == -1) {
        printf("error writing to fifo Client To Server\n");
        exit(4);
    }
    close(fd);

}

// Function Name:  serialize
// Purpose:        Used to open file to be uploaded specified by user and send it to server through a FIFO. Uses a structure to hold and send data through the FIFO
//                 The structure is serialized (converted into a byte stream) and deserialized on the server side.
// Parameters:     sourceData: Type sourceContent struct, sourceData is a pointer to a structure that holds data related to the data read from source content, including the number of bytes read and the payload(characters read)
//                 fileName: A pointer to the file specified for upload by the user. This is used to open and read the file to be uploaded
void serialize(sourceContent *sourceData, char *fileName){
    // Open the file to be uploaded transfer. If there is an error opening file, print error and exit
    int fdTransferFile = open(fileName, O_RDONLY);
    if (fdTransferFile == -1){
        printf("error opening source file\n");
        exit(1);
    }

    // Open fifo Client to Server. If there is an error opening file, print error and exit
    int fdFifo1 = open("fifoClientToServer", O_WRONLY);
    if (fdFifo1 == -1) {
        printf("error opening fifoClientToServer\n");
        exit(2);
    }
    // Should hang here until the file is opened in read mode
    
    // While loop that reads and writes the entirety of any valid file provided by user to FIFO
    // While statement reads 1024 bytes from the file to be uploaded into a structure payload. Read returns the number of bytes that were read to the bytesRead member of the strucutre
    while ((sourceData->bytesRead = read(fdTransferFile, sourceData->payload, 1024)) > 0 ) {
        // writes the payload into the FIFO, number of bytes written is number of bytes read from before
        if (write(fdFifo1, sourceData->payload, sourceData->bytesRead) == -1) {
            printf("error writing to fifo Client To Server\n");
            close(fdFifo1);
            exit(3);
        }
    }
    // Close necessary opened files
    close(fdTransferFile);
    close(fdFifo1);
    
}

// Function Name:  recieveMessage
// Purpose:        Used to recieve messages from the server->client FIFO. Used here to recieve a message containing the final status of a file upload
//                 Returns appropriate value to main, bassed off what the server sends back. This return value is used to print the appropriate message before exiting
int recieveMessage(){
    char message[1024];
    // Open FIFO Server to Client. If there is an error opening, print error message and exit. Read only, as messages are only read here   
    int fdFifo2 = (open("fifoServerToClient", O_RDONLY));
    if (fdFifo2 == -1){
        printf("error opening fifoServerToClient\n");
        return 2;
    }

    // Read FIFO Server to Client for message from server. IF there is an error reading, print error message and exit
    if ((read(fdFifo2, message, 1024)) == -1) {
        printf("Error reading fifo Server -> Client in recieveMessage. Client exiting\n");
        close(fdFifo2);
        exit(2);
    }   
    close(fdFifo2);

    // If server sent ALL DONE, file was uploaded correctly. Otherwise, there was an error. Return appropriate value
    if (strstr(message, "ALL DONE") != NULL){
        return 0;
    }
    else {
        return 1;
    }
} 
