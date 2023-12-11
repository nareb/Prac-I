/*
* message.c / Practicum I
*
* Binita Shakya / CS5600 / Northeastern University
* Fall 2023 / November 14, 2023
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


// Define a structure for a message
typedef struct {
    int id;
    char sender[50];
    char receiver[50];
    char content[500];
    time_t time_sent;
    int delivered;
    int in_cache; // Added field to track whether the message is in the cache
} Message;


// Define the size of the main memory
#define MAIN_MEMORY_SIZE 100

// Define the size of a message in the cache
#define MESSAGE_SIZE 1024

// Define the number of messages to be cached
#define CACHE_SIZE 16

// Define an array to represent the main memory
Message mainMemory[MAIN_MEMORY_SIZE];

// Define a cache for messages
Message messageCache[CACHE_SIZE];

// Define an array to store the last access times for cache replacement
time_t lastAccessTimes[CACHE_SIZE];


int cacheOccupied[CACHE_SIZE] = {0}; // Initialized to 0, indicating empty slots


// Function to perform random page replacement
int randomReplacement() {
    return rand() % CACHE_SIZE;
}

// Function to perform LRU page replacement
int LRURreplacement(time_t* lastAccessTimes) {
    time_t minAccessTime = lastAccessTimes[0];
    int indexToReplace = 0;

    for (int i = 1; i < CACHE_SIZE; i++) {
        if (lastAccessTimes[i] < minAccessTime) {
            minAccessTime = lastAccessTimes[i];
            indexToReplace = i;
        }
    }

    return indexToReplace;
}


// Function to create a new message
Message* create_msg(int id, const char* sender, const char* receiver, const char* content) {
    Message* msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) {
        // Handle memory allocation error
        perror("Failed to allocate memory for a new message");
        return NULL;
    }

    msg->id = id;
    strncpy(msg->sender, sender, sizeof(msg->sender));
    strncpy(msg->receiver, receiver, sizeof(msg->receiver));
    strncpy(msg->content, content, sizeof(msg->content));
    msg->time_sent = time(NULL);
    msg->delivered = 0; // Initialize delivered flag to 0

    msg->in_cache = 0; // Initialize in_cache to 0

    return msg;
}


// Function to store a message on disk
int store_msg(Message* msg, const char* store_filename, int (*replacementAlgorithm)()) {
    FILE* file = fopen(store_filename, "ab"); // Open file for appending in binary mode
    if (file == NULL) {
        // Handle file open error
        perror("Failed to open message storage file for appending");
        return -1;
    }


    // Write the message to the file separately
    if (fwrite(&(msg->id), sizeof(int), 1, file) != 1 ||
fwrite(msg->sender, sizeof(char), sizeof(msg->sender), file) != sizeof(msg->sender) ||
        fwrite(msg->receiver, sizeof(char), sizeof(msg->receiver), file) != sizeof(msg->receiver) ||
        fwrite(msg->content, sizeof(char), sizeof(msg->content), file) != sizeof(msg->content) ||
        fwrite(&(msg->time_sent), sizeof(time_t), 1, file) != 1 ||
        fwrite(&(msg->delivered), sizeof(int), 1, file) != 1) {
        // Handle write error
        perror("Failed to write message to file");
        fclose(file);
        return -1;
    }

    fclose(file);

    // Add the message to the cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!cacheOccupied[i]) {
            messageCache[i] = *msg;
            cacheOccupied[i] = 1; // Mark the cache slot as occupied
            msg->in_cache = 1; // Update the in_cache field
            lastAccessTimes[i] = time(NULL); // Update the last access time
            break;
        }
    }

    return 0; // Success
}


// Function to determine if the cache is full
int cacheIsFull() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (messageCache[i].id == -1) {
            return 0; // Cache is not full
        }
    }
    return 1; // Cache is full
}


// Function to replace a message in the cache using a specified replacement algorithm
int replaceMessageInCache(int (*replacementAlgorithm)()) {
    int indexToReplace = replacementAlgorithm();
    messageCache[indexToReplace].in_cache = 0; // Mark the replaced message as not in the cache
    return indexToReplace;
}


// Function to retrieve a message by its identifier
Message* retrieve_msg(int id, const char* store_filename, int (*replacementAlgorithm)()) {

     // Check if the message is in the cache
for (int i = 0; i < CACHE_SIZE; i++) {
        if (messageCache[i].id == id) {
            // Update cache metadata
            messageCache[i].in_cache = 1; // Update the in_cache field
            lastAccessTimes[i] = time(NULL); // Update the last access time

            // Allocate memory for the retrieved message
            Message* retrieved_msg = (Message*)malloc(sizeof(Message));
            if (retrieved_msg == NULL) {
                // Handle memory allocation error
                perror("Failed to allocate memory for retrieved message");
                return NULL;
            }

            // Copy the contents from the cache to the retrieved message
            *retrieved_msg = messageCache[i];

            return retrieved_msg; // Return the allocated memory, not the address in cache
        }
     }

    // Check if the message is in main memory
    for (int i = 0; i < MAIN_MEMORY_SIZE; i++) {
        if (mainMemory[i].id == id) {
            // Allocate memory for the retrieved message
            Message* retrieved_msg = (Message*)malloc(sizeof(Message));
            if (retrieved_msg == NULL) {
                // Handle memory allocation error
                perror("Failed to allocate memory for retrieved message");
                return NULL;
            }

            // Copy the contents from main memory to the retrieved message
            *retrieved_msg = mainMemory[i];
            return retrieved_msg;
        }
    }


    // If the message is not in the cache or main memory, retrieve it from secondary storage (disk)
    FILE* file = fopen(store_filename, "rb");
    if (file == NULL) {
        // Handle file open error
        return NULL;
    }


    Message msg;
    while (fread(&msg, sizeof(int), 1, file) == 1) {

        if(fread(msg.sender, sizeof(char), sizeof(msg.sender), file)!= sizeof(msg.sender) ||
fread(msg.receiver, sizeof(char), sizeof(msg.receiver), file)!= sizeof(msg.receiver) ||
            fread(msg.content, sizeof(char), sizeof(msg.content), file) != sizeof(msg.content) ||
            fread(&(msg.time_sent), sizeof(time_t), 1, file) !=1 ||
            fread(&(msg.delivered), sizeof(int), 1, file) != 1){
            //Handle read error
            perror("Failed to read message from file");
            fclose(file);
            return NULL;
        }

        if (msg.id == id) {
            fclose(file);

            // Allocate memory for the retrieved message
            Message* retrieved_msg = (Message*)malloc(sizeof(Message));
            if (retrieved_msg == NULL) {
                // Handle memory allocation error
                perror("Failed to allocate memory for retrieved message");
                return NULL;
            }


            retrieved_msg->id = msg.id;
            strncpy(retrieved_msg->sender, msg.sender, sizeof(retrieved_msg->sender));
            strncpy(retrieved_msg->receiver, msg.receiver, sizeof(retrieved_msg->receiver));
            strncpy(retrieved_msg->content, msg.content, sizeof(retrieved_msg->content));
            retrieved_msg->time_sent = msg.time_sent;
            retrieved_msg->delivered = msg.delivered;


            // Store the retrieved message in the cache
            int indexToReplace = replaceMessageInCache(replacementAlgorithm);
            messageCache[indexToReplace] = *retrieved_msg;
            messageCache[indexToReplace].in_cache = 1; // Mark the new message as in the cache
            lastAccessTimes[indexToReplace] = time(NULL); // Update the last access time

            fclose(file);
            return retrieved_msg;

        }
    }

    fclose(file);
    return NULL; // Message not found
}


// Function to print the contents of the cache for testing purposes
void printCache() {
    printf("\nCache Contents:\n");
    for (int i = 0; i < CACHE_SIZE; i++) {
printf("Index %d: ID %d, In Cache: %d\n", i, messageCache[i].id, messageCache[i].in_cache);
    }
}


// Function to test the cache mechanism
void testCacheMechanism(int cacheSize, int messageSize, int numMessages) {

    // Initialize hit/miss counters
    int cacheHits = 0;
    int cacheMisses = 0;

    // Testing - Basic Functionality
    printf("Testing Basic Functionality...\n");
    for (int i = 1; i <= numMessages; i++) {
        Message* newMessage = create_msg(i, "Sender", "Receiver", "Test Content");
        store_msg(newMessage, "test_store.txt", LRURreplacement); // Using LRU for testing
        Message* retrievedMessage = retrieve_msg(i, "test_store.txt", LRURreplacement);
        printf("Message %d: Retrieved ID %d\n", i, retrievedMessage->id);
        free(retrievedMessage);
    }

    // Print cache contents after basic functionality testing
    printCache();


    // Testing - Cache Size Management
    printf("\nTesting Cache Size Management (Cache Size: %d)...\n", CACHE_SIZE);
    for (int i = numMessages + 1; i <= numMessages + CACHE_SIZE; i++) {
        Message* newMessage = create_msg(i, "Sender", "Receiver", "Test Content");
        store_msg(newMessage, "test_store.txt", LRURreplacement); // Using LRU for testing

        Message* retrievedMessage = retrieve_msg(i - CACHE_SIZE, "test_store.txt", LRURreplacement);
        if (retrievedMessage != NULL) {
            printf("Message %d: Retrieved ID %d\n", i, retrievedMessage->id);
            free(retrievedMessage);
        } else {
            printf("Failed to retrieve Message with ID %d\n", i);
        }
}

    // Print cache contents after cache size management testing
    printCache();


    // Testing - Page Replacement Algorithms (Random)
    printf("\nTesting Page Replacement Algorithms (Random)...\n");
    for (int i = 1; i <= CACHE_SIZE + 1; i++) {
        Message* newMessage = create_msg(i, "Sender", "Receiver", "Test Content");
        store_msg(newMessage, "test_store.txt", randomReplacement);
        Message* retrievedMessage = retrieve_msg(i, "test_store.txt", randomReplacement);
printf("Message %d: Retrieved ID %d\n", i, retrievedMessage->id);
        free(retrievedMessage);
    }

    // Print cache contents after random replacement testing
    printCache();

    // Testing - Page Replacement Algorithms (LRU)
    printf("\nTesting Page Replacement Algorithms (LRU)...\n");
    for (int i = 1; i <= CACHE_SIZE + 1; i++) {
        Message* newMessage = create_msg(i, "Sender", "Receiver", "Test Content");
        store_msg(newMessage, "test_store.txt", LRURreplacement);
        Message* retrievedMessage = retrieve_msg(i, "test_store.txt", LRURreplacement);
        printf("Message %d: Retrieved ID %d\n", i, retrievedMessage->id);
        free(retrievedMessage);
    }

    // Print cache contents after LRU replacement testing
    printCache();


    //Testing - Test Cache Overflow
    printf("\nTesting - Cache Overflow...\n");
    for (int i = 1; i <= CACHE_SIZE * 2; i++) {
        Message* newMessage = create_msg(i, "Sender", "Receiver", "Test Content");
        store_msg(newMessage, "test_store.txt", LRURreplacement); // Using LRU for testing
        Message* retrievedMessage = retrieve_msg(i, "test_store.txt", LRURreplacement);
        printf("Message %d: Retrieved ID %d\n", i, retrievedMessage->id);
        free(retrievedMessage);
    }


    // Print cache contents after cache overflow testing
    printCache();

    // Testing - Test Retrieving Non-Existent Message
    printf("\nTesting - Retrieve Non-Existent Message...\n");
    int nonExistentId = CACHE_SIZE * 3;
    Message* nonExistentMessage = retrieve_msg(nonExistentId, "test_store.txt", LRURreplacement);
    if (nonExistentMessage == NULL) {
        printf("Message with ID %d does not exist.\n", nonExistentId);
    } else {
        printf("Unexpected: Retrieved Message with ID %d\n", nonExistentMessage->id);
        free(nonExistentMessage);
    }


    // Testing - Test Cache Replacement with Disk Retrieval
    printf("\nTesting - Cache Replacement with Disk Retrieval...\n");
    int replacementTestId = CACHE_SIZE * 2 + 1;
    Message* replacementTestMessage = retrieve_msg(replacementTestId, "test_store.txt", LRURreplacement);
if (replacementTestMessage != NULL) {
        printf("Retrieved Message with ID %d\n", replacementTestMessage->id);
        free(replacementTestMessage);
    } else {
        printf("Failed to retrieve Message with ID %d\n", replacementTestId);
    }


    // Perform 1000 random accesses for testing
    for (int i = 0; i < 1000; i++) {
        int messageIdToRetrieve = rand() % numMessages + 1;
        Message* retrievedMessage = retrieve_msg(messageIdToRetrieve, "test_store.txt", LRURreplacement);

        if (retrievedMessage != NULL) {
            // Update cache hit count
            cacheHits++;
        } else {
            // Update cache miss count
            cacheMisses++;
        }

        // Free memory if necessary
        if (retrievedMessage != NULL && retrievedMessage->in_cache == 0) {
            free(retrievedMessage);
        }
    }

    // Calculate and print cache metrics
    printf("Number of cache hits: %d\n", cacheHits);
    printf("Number of cache misses: %d\n", cacheMisses);

    float hitRatio = (float)cacheHits / (cacheHits + cacheMisses) * 100.0;
    printf("Cache hit ratio: %.2f%%\n", hitRatio);


    // Free allocated memory
    for (int i = 1; i <= numMessages + CACHE_SIZE + 1; i++) {
        Message* msg = retrieve_msg(i, "test_store.txt", LRURreplacement);
        if (msg != NULL) {
            free(msg);
        }
    }

    printf("\nTesting completed.\n");
}


int main() {
    srand(time(NULL)); // Initialize random seed

    const char* store_filename = "message_store.txt";
// Initialize the cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        messageCache[i].id = -1; // Set id to -1 to indicate an empty slot
        messageCache[i].in_cache = 0; // Initialize in_cache flag to 0
     }


    //Clear the contents of main memory
    memset(mainMemory, 0, sizeof(mainMemory));


    //Delete the existing message storage file
    if (remove(store_filename) != 0){
        //Handle file deletion error
        perror("Failed to delete existing message storage file");
        return 1; // Exit with an error code
    }


    //Create a new message
    Message* newMessage = create_msg(1, "Sender1", "Receiver1", "New, Day!");

    if (newMessage == NULL) {
        // Handle memory allocation error
        printf("Failed to create a new message.\n");
        return 1; // Exit with an error code
    }



    // Choose the replacement algorithm (0 for Random, 1 for LRU)
    int replacementAlgorithmChoice = 1;


    // Store the message on disk using the selected replacement algorithm
    int storeResult = store_msg(newMessage, store_filename, (replacementAlgorithmChoice == 0) ? randomReplacement : LRURreplacement);


    if (storeResult == -1) {
        // Handle file write error
        printf("Failed to store the message on disk.\n");
        free(newMessage); // Free allocated memory
        return 1; // Exit with an error code
    }

    printf("Message stored successfully.\n");


    // Retrieve the message by its identifier
int messageIdToRetrieve = 1; //
    Message* retrievedMessage = retrieve_msg(messageIdToRetrieve, store_filename, LRURreplacement);

    if (retrievedMessage == NULL) {
        // Handle message retrieval error
        printf("Failed to retrieve the message.\n");
        free(newMessage); // Free allocated memory
        return 1; // Exit with an error code
    }


    // Print the retrieved message
    printf("Retrieved Message:\n");
    printf("ID: %d\n", retrievedMessage->id);
    printf("Sender: %s\n", retrievedMessage->sender);
    printf("Receiver: %s\n", retrievedMessage->receiver);
    printf("Content: %s\n", retrievedMessage->content);
    printf("Time Sent: %ld\n", (long)retrievedMessage->time_sent);
    printf("Delivered: %d\n", retrievedMessage->delivered);


    // Check if the retrieved message was created through malloc (not in the cache or main memory)
    if (retrievedMessage->in_cache == 0) {
        printf("Freeing retrievedMessage...\n");

        //Free memory only if it was created through malloc
        free(retrievedMessage);
    }

    // Set the pointers to NULL to avoid inadvertent use or freeing
    newMessage = NULL;
    retrievedMessage = NULL;


    // Set numMessages to a suitable value for testing
   // int numMessages = 1000;

    // Call the testing function
    testCacheMechanism(18, 1024, 10);

    return 0; // Exit successfully
}







          
  

      
