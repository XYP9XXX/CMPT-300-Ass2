#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "list.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define MSG_MAX_LEN 257
#define REMOTE_FQDN_LENGTH 255

// A mutex for receive thread and printer thread
pthread_mutex_t status_lock = PTHREAD_MUTEX_INITIALIZER;

// A mutex for keyboard thread and sender thread
pthread_mutex_t status_lock1 = PTHREAD_MUTEX_INITIALIZER;

// Condition variable for sender means sender finish
pthread_cond_t sender_finish = PTHREAD_COND_INITIALIZER;

// Condition variable for keyboard enter means keyboard enter finish
pthread_cond_t keyboard_finish = PTHREAD_COND_INITIALIZER;

// Condition variable for receive means receive finish
pthread_cond_t receive_finish = PTHREAD_COND_INITIALIZER;

// Condition variable for print means print finish
pthread_cond_t print_finish = PTHREAD_COND_INITIALIZER;

// Create two socketaddr_in struct that one store local IPV4 address while the other store remote IPV4 address
struct sockaddr_in sinLocal;
struct sockaddr_in sinRemote;

// A global variable that store the size of remote IPV4 address length
unsigned int client_addr_len = sizeof(sinRemote);

// Create socket for connection
int socketDescriptor;
int socketDescriptor1;

// A global variable in order to store the message to send.
char messageToSend[MSG_MAX_LEN];

// Three global boolean variable to indicate the state
bool startSending = false;
bool startReceive = false;
bool exitStatusCode = false;

// Two global variables to store the local port and remote port number.
int remote_port;
int local_port;

// A global variable to store the remote host name.
char remote_host_name[REMOTE_FQDN_LENGTH];

// Two lists to store the message
List *output_list;
List *input_list;

// Create four threads.
pthread_t receiveThreadPID;
pthread_t sendThreadPID;
pthread_t keyBoardDetectPID;
pthread_t printScreenPID;

// Function for exiting the system.
void exitSystem(char *exitStatus, bool exitFromRecv) {
    // Close the socket
    close(socketDescriptor);
    close(socketDescriptor1);

    fputs(exitStatus, stdout);

    // cancel the thread
    if (exitFromRecv) {
        // Set start receive to true.
        startReceive = true;

        // unlock print to screen thread and close it
        pthread_cond_signal(&receive_finish);
        pthread_mutex_unlock(&status_lock);

        // Set start sending to true.
        startSending = true;

        // unlock sender thread and close it.
        pthread_cond_signal(&keyboard_finish);
        pthread_mutex_unlock(&status_lock1);
        pthread_cancel(keyBoardDetectPID);
    } else {
        startReceive = true;
        // unlock print to screen thread and close it
        pthread_cond_signal(&receive_finish);
        pthread_mutex_unlock(&status_lock);
        free(exitStatus);
    }
    // Destroy the mutex
    pthread_mutex_destroy(&status_lock);
    pthread_mutex_destroy(&status_lock1);
    pthread_cond_destroy(&keyboard_finish);
    pthread_cond_destroy(&sender_finish);
    pthread_cond_destroy(&receive_finish);
    pthread_cond_destroy(&print_finish);
}

void sendExitCodeToRemote(char *exitStatus) {
    // send terminal exit code to remote
    ssize_t s_ret = sendto(socketDescriptor1, exitStatus,
                           strlen(exitStatus),
                           0, (struct sockaddr *) &sinRemote, client_addr_len);
    if (s_ret < 0) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }
    startSending = true;
    // unlock sender thread and close it
    pthread_cond_signal(&keyboard_finish);
    pthread_mutex_unlock(&status_lock1);

    startReceive = false;
    // unlock receive thread to close it
    pthread_cond_signal(&print_finish);
    pthread_mutex_unlock(&status_lock);
    pthread_cancel(receiveThreadPID);
    exitSystem(exitStatus, false);
}

// The receiving thread for receiving the message
void *receiveThread() {
    while (1) {
        // Lock the system cause in this thread will access the shared area(output list).
        pthread_mutex_lock(&status_lock);

        // When receive ends, we need to wait until print thread give the system signal to wake up this thread.
        while (startReceive != false) {
            pthread_cond_wait(&print_finish, &status_lock);
        }

        if (exitStatusCode) {
            break;
        }

        socklen_t sin_size = sizeof(sinLocal);

        char receiveBuffer[MSG_MAX_LEN];

        // Receive from call to receive the message and store it to the receive buffer.
        int recv_size = recvfrom(socketDescriptor,
                                 receiveBuffer, MSG_MAX_LEN, 0,
                                 (struct sockaddr *) &sinLocal, &sin_size);

        // End the receive buffer array
        receiveBuffer[recv_size] = '\0';

        // If the only character is !, we exit the system.
        if (receiveBuffer[0] == '!' && strlen(receiveBuffer) == 2) {
            exitStatusCode = true;
            exitSystem("!\n", true);
            break;
        }

        // Check if we successfully receive the message from the other port.
        if (recv_size < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        char *receive_message = (char *) malloc(MSG_MAX_LEN);
        strcpy(receive_message, receiveBuffer);
        // Add the receiving message to the output list.
        List_append(output_list, receive_message);

        // Set start receive to true.
        startReceive = true;

        // Give the signal to the printer thread that it can start print the message receive.
        pthread_cond_signal(&receive_finish);

        // Unlock the system so other thread can acccess the shareed area.
        pthread_mutex_unlock(&status_lock);
    }
    // Exit the system.
    pthread_exit(NULL);
}

// Printer thread to proint the message receive.
void *printToScreen() {
    while (1) {
        // Lock the system cause in this thread will access the shared area(output list).
        pthread_mutex_lock(&status_lock);

        // When print ends, we need to wait until receive thread receive the message again and
        // give the system signal to wake up this thread.
        while (startReceive == false) {
            pthread_cond_wait(&receive_finish, &status_lock);
        }

        // If exit status is true, we just break the loop.
        if (exitStatusCode) {
            break;
        }
        // While loop for print the receiving thread and remove it from the output list by calling "List_trim" function.
        while (List_count(output_list) > 0) {
            char *tmp = (char *) List_trim(output_list);
            if (strlen(tmp) > 0) {
                fputs(">>> ", stdout);
                int status = fputs(tmp, stdout);
                if (status == EOF) {
                    fputs("ERROR!", stdout);
                    exit(EXIT_FAILURE);
                }
            }
            free(tmp);
            fflush(stdout);
        }

        // After printin, just set start receive to true.
        startReceive = false;

        // Give the signal to the receiving thread that it can start receive the message again.
        pthread_cond_signal(&print_finish);

        // Unlock the system so other thread can access the shared area.
        pthread_mutex_unlock(&status_lock);
    }
    // Exit the system.
    startReceive = false;
    // Give the signal to the receiving thread that it can start receiv the message again.
    pthread_cond_signal(&print_finish);
    // Unlock the system so other thread can access the shared area.
    pthread_mutex_unlock(&status_lock);
    pthread_exit(NULL);
}


// The keyboard thread for typing the message user want to send.
void *responseFromKeyBoard() {
    while (1) {
        // Lock the system cause in this thread will access the shared area(input list).
        pthread_mutex_lock(&status_lock1);

        // When send ends, we need to wait until send thread give the system to wake up this thread
        while (startSending != false) {
            pthread_cond_wait(&sender_finish, &status_lock1);
        }

        // If exit status is true, we just break the loop.
        if (exitStatusCode) {
            break;
        }

        // Use fgets call for user type in message.
        fgets(messageToSend, MSG_MAX_LEN, stdin);

        // A new variable s for user message.
        char *s = (char *) malloc(strlen(messageToSend) + 1);
        strcpy(s, messageToSend);

        // Check if user message is "!". If so, exit the system.
        if (s[0] == '!' && strlen(s) == 2) {
            exitStatusCode = true;
            sendExitCodeToRemote(s);
            break;
        }

        // If user message is not "!", we add it to the input list.
        if (exitStatusCode != true) {
            List_append(input_list, s);
        }

        // Set start sending to true.
        startSending = true;

        // Give the signal to the send thread that it can start send the message.
        pthread_cond_signal(&keyboard_finish);

        // Unlock the system so other thread can acccess the shareed area.
        pthread_mutex_unlock(&status_lock1);
    }
    pthread_exit(NULL);

}

// Send thread to send the message user typed.
void *sendThread() {
    while (1) {
        // Lock the system cause in this thread will access the shared area(output list).
        pthread_mutex_lock(&status_lock1);

        // When keyboard thread ends, we need to wait until keyboard thread type the message again and
        // give the system signal to wake up this thread.
        while (startSending == false) {
            pthread_cond_wait(&keyboard_finish, &status_lock1);
        }

        // If exit status is true, we just break the loop.
        if (exitStatusCode) {
            break;
        }
        // Message variable for storing the message user type in.
        char *message = (char *) List_trim(input_list);

        // Sendto call to send the message.
        ssize_t s_ret = sendto(socketDescriptor1, message,
                                strlen(message),
                                0, (struct sockaddr *) &sinRemote, client_addr_len);

        // Check if we successfully send the message to the other port.
        if (s_ret < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }

        // Free the memory
        free(message);

        // Set start sending to false
        startSending = false;

        // Give the signal to the keyboard thread that it can start type in the message.
        pthread_cond_signal(&sender_finish);

        // Unlock the system so other thread can acccess the shareed area.
        pthread_mutex_unlock(&status_lock1);
    }
    pthread_exit(NULL);
}

int main(int argCount, char **argv) {

    // Set two ports according to user input
    char *tmp;
    local_port = strtol(argv[1], &tmp, 10);
    remote_port = strtol(argv[3], &tmp, 10);

    //get remote host FQDN
    strcpy(remote_host_name, argv[2]);

    // Create two lists for receive and send data frame storage.
    output_list = List_create();
    input_list = List_create();

    // welcome message
    fputs("s_talk starting online!!\n", stdout);

    // Get remote host information based on the remote host name.
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    int status = getaddrinfo(remote_host_name, NULL, &hints, &servinfo);
    if (status) {
        fputs("getaddrinfo failed: \n", stdout);
        return EXIT_FAILURE;
    }
    struct addrinfo *ptr = servinfo;
    char address[INET_ADDRSTRLEN];
    void *addr;
    // Get remote host ip address.
    while (ptr != NULL) {

        if (ptr->ai_addr->sa_family == AF_INET) {
            addr = &(((struct sockaddr_in *) ptr->ai_addr)->sin_addr);
        } else {
            addr = &(((struct sockaddr_in6 *) ptr->ai_addr)->sin6_addr);
        }

        // sign remote IP address to address buffer.
        inet_ntop(ptr->ai_family, addr, address, sizeof(address));
        ptr = ptr->ai_next;
    }

    // Setting of the local client
    sinLocal.sin_family = AF_INET;
    sinLocal.sin_addr.s_addr = htonl(INADDR_ANY);
    sinLocal.sin_port = htons(local_port);

    // Setting of the remote client
    sinRemote.sin_family = AF_INET;
    inet_pton(AF_INET, address, &sinRemote.sin_addr.s_addr);
    sinRemote.sin_port = htons(remote_port);

    // Setting of the socket
    socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    socketDescriptor1 = socket(AF_INET, SOCK_DGRAM, 0);

    // Bind the socket.
    int success = bind(socketDescriptor, (struct sockaddr *) &sinLocal, sizeof(sinLocal));

    // Check if bind successfully
    if (success == -1) {
        fputs("Failed to bind\n", stdout);
        exit(EXIT_FAILURE);
    } else {
        fputs("Remote IP address get successfully\n", stdout);
        fputs("Successfully bound\n", stdout);
    }

    // Start threads
    pthread_create(&sendThreadPID, NULL, sendThread, NULL);
    pthread_create(&receiveThreadPID, NULL, receiveThread, NULL);
    pthread_create(&keyBoardDetectPID, NULL, responseFromKeyBoard, NULL);
    pthread_create(&printScreenPID, NULL, printToScreen, NULL);

    // Join threads
    pthread_join(receiveThreadPID, NULL);
    pthread_join(sendThreadPID, NULL);
    pthread_join(keyBoardDetectPID, NULL);
    pthread_join(printScreenPID, NULL);

    // free addrinfo
    freeaddrinfo(servinfo);
    exit(EXIT_SUCCESS);
}
