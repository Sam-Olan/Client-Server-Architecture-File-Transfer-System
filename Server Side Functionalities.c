#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// User defined strucutre to hold packet payload and number of bytes read
typedef struct uploadContent{
    int bytesRead;
    char payload[1024];
}uploadContent;

// Function declarations 
void checkFifos();
void recieveMessage(char *);
void deserialize(uploadContent *, char *);
void sendMessage(char *);

int main(int argc, char* argv[]){
    printf("Server\n");

    // If the user entered a command line argument, print error and exit
    if (argc != 1){
        printf("You must not enter a command line arguments\n");
        return 1;
    }
    
    char fileName[1024];

    struct uploadContent uploadData;

    // Infinite while loop. This keeps the server running incase a new client attempts access
    while (1){

        checkFifos();

        // Calls function that waits for a message from client. Message should contain name of file to be uploaded
        recieveMessage(fileName);

        deserialize(&uploadData, fileName);

        // Send message to client indicating final status of the file upload
        sendMessage(fileName);
    }

    return 0;
}

// Function Name:  checkFifos
// Purpose:        Checks if there are FIFOs in the current directory. If either of the FIFOs are missing, create them
void checkFifos(){
    // If FIFO does't exist, create it. If there is an error creating FIFO, print error and exit
    if (access("fifoClientToServer", F_OK) == -1) {
        if ((mkfifo("fifoClientToServer", 0777)) == -1){
            printf("Error creating fifoClientToServer");
            exit(1);
        }
    }
    // If FIFO does't exist, create it. If there is an error creating FIFO, print error and exit
    if (access("fifoServerToClient", F_OK) == -1) {
        if ((mkfifo("fifoServerToClient", 0777)) == -1) {
            printf("Error creating fifoServerToClient");
            exit(2);
        }
    }
}

// Function Name:  recieveMessage
// Purpose:        Used to recieve messages from the server->client FIFO. Used here to recieve a message containing the file name of a file to be uploaded
//                 If the user entered a filepath to their file, extract the filename from the path, so server can name the file without the path when creating the file
// Parameters:     fileName: Used to hold the name of the file to be read. If filenName contains a path to a file, the filepath is removed and just the name remains
void recieveMessage(char *fileName) {
    // Open FIFO Server to Client. If there is an error opening, print error message and exit. Read only, as messages are only read here   
    int fdFifo2 = (open("fifoServerToClient", O_RDONLY));
    if (fdFifo2 == -1){
        printf("error opening fifoServerToClient. Server exiting\n");
        exit(1);
    }

    // Read FIFO Server to Client. IF there is an error reading, print error message and exit
    if ((read(fdFifo2, fileName, 1024)) == -1) {
        printf("Error reading fifo Server -> Client. Server exiting\n");
        close(fdFifo2);
        exit(2);
    }    
    close(fdFifo2);

    // If the user entered a filepath to a file, extract the file name from the file path
    // The file name is used to name the file created/opened, don't want filepath in file name
    if (fileName[0] == '/') {
        char *slash = strrchr(fileName, '/');   // Looks for the last slash in the file path, takes anything to the right of it
        strcpy(fileName, slash + 1);
    }
}

// Function Name:  deserialize
// Purpose:        Used to open/create new file for uploaded content containing data sent through FIFO. Uses a structure to hold data sent through the FIFO
//                 Structure sent through FIFO is deserialized, putting the data back into a structure in an identical format
//                 The data from the structure is then written to the newly created file in chunks. Chunk size matches the size of the chunk passed through the FIFO
// Parameters:     uploadContent: Type sourceContent struct, source data is a strucutre used to hold the payload (character written through the fifo), 
//                      and the number of bytes written through the fifo.
//                 fileName: used to hold the name of the file to be opened, specified by the user
 void deserialize(uploadContent *uploadContent, char *fileName){
    int fdUploadFile;

    // Check if the file being upload already exists. If it does, create file with new name
    if (access(fileName, F_OK) == -1){
        fdUploadFile = open(fileName, O_CREAT | O_WRONLY, 0777);
    }
    else {
        fdUploadFile = open(strcat(fileName, ".copy"), O_CREAT | O_WRONLY, 0777);
    }
    
    // Open fifo Client to Server. If there is an error opening, print error message and exit 
    int fdFifo1 = open("fifoClientToServer", O_RDONLY);
    if (fdFifo1 == -1) {
        printf("error opening fifoClientToServer\n");
        exit(1);
    }

    // While loop that reads and writes entire file provided by client
    while ((uploadContent->bytesRead = read(fdFifo1, uploadContent->payload, 1024)) > 0 ) { 
        // writes the payload into the newly created file, number of bytes written is number of bytes read from line above
        if (write(fdUploadFile, uploadContent->payload, uploadContent->bytesRead) == -1) {
            printf("error writing to transferContents\n");
            close(fdUploadFile);
            exit(2);
        }       
    }
    close(fdFifo1);
    close(fdUploadFile);
 }

// Function Name:  sendMessage
// Purpose:        Used to send messages to the client. In this program, it's used to send the client a message indicating the final status of the file upload 
//                 If the file is uploaded successfully, print the name of the file that was uploaded 
// Parameters:     fileName: Holds the name of the file to be read. Used to print the name of the file uploaded if successful
void sendMessage(char *fileName){
// Open fifo Server to Client   
    int fdFifo2 = (open("fifoServerToClient", O_WRONLY));
    if (fdFifo2 == -1){
        printf("error opening fifoServerToClient. Server exiting\n");
        close(fdFifo2);
        exit(2);
    }
    write(fdFifo2, "ALL DONE", 9);
    close(fdFifo2);
    printf("File \"%s\" Uploaded\n", fileName);
}