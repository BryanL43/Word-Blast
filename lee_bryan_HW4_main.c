/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Bryan Lee
* Student ID:: 922649673
* GitHub-Name:: BryanL43
* Project:: Assignment 4 – Word Blast
*
* File:: lee_bryan_HW4_main.c
*
* Description:: This program reads the book War and Peace and acquires the top ten
* most frequent words with six letters or more. It also supports multi-threading,
* where the user can specify the number of threads in the run options to speed up
* the frequency counting.
*
**************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

// You may find this Useful
char* delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

// Declare the mutex lock
pthread_mutex_t lock;

char** threadBuffer;

typedef struct WordFreq {
    char* word;
    int freq;
} WordFreq;

int arraySize = 2000;
struct WordFreq* counterArray;
int counterArraySize = 0;

void* counterThread(void* args) {
    char* buffer = (char*)args;

    char* saveptr;
    char* token = strtok_r(buffer, delim, &saveptr);
    while (token != NULL) {
        if (strlen(token) >= 6) {
            int found = 0;

            //If word is already in array then increment frequency
            for (int i = 0; i < counterArraySize; i++) {
                if (strcasecmp(counterArray[i].word, token) == 0) {
                    pthread_mutex_lock(&lock);
                    counterArray[i].freq++;
                    found = 1;
                    pthread_mutex_unlock(&lock);
                    break;
                }
            }

            //If word isn't in array then add it to array with frequency of 1
            if (!found) {
                pthread_mutex_lock(&lock);
                if (counterArraySize == arraySize - 1) {
                    arraySize += 500;
                    counterArray = realloc(counterArray, arraySize * sizeof(WordFreq));
                    if (counterArray == NULL) {
                        perror("Failed to resize counter array");
                        exit(EXIT_FAILURE);
                    }
                }

                counterArray[counterArraySize].word = strdup(token);
                if (counterArray[counterArraySize].word == NULL) {
                    perror("Error copying word to array!\n");
                    exit(EXIT_FAILURE);
                }
                counterArray[counterArraySize].freq = 1;
                counterArraySize++;

                pthread_mutex_unlock(&lock);
            }
        }
        token = strtok_r(NULL, delim, &saveptr);
    }
}

void quickSort(WordFreq* arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high].freq;
        int i = low - 1;

        for (int j = low; j < high; j++) {
            if (arr[j].freq >= pivot) {
                i++;
                WordFreq temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }

        WordFreq temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;

        int pi = i + 1;

        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

int main (int argc, char *argv[]) {
    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************

    // ***TO DO*** Look at arguments, open file, divide by threads
    // Allocate and Initialize any storage structures

    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Error initializing mutex!\n");
        exit(EXIT_FAILURE);
    }

    char* fileName = argv[1];
    int threadCount = atoi(argv[2]);

    printf("\n\nWord Frequency Count on %s with %d threads\n", fileName, threadCount);
    
    //Open the file under read only permission
    int file = open(argv[1], O_RDONLY);
    if (file < 0) {
        perror("File failed to open!\n");
        exit(EXIT_FAILURE);
    }

    //Obtain the file size for calculating buffer size
    int fileSize = lseek(file, 0, SEEK_END);
    if (fileSize < 0) {
        perror("Error getting file size!\n");
        exit(EXIT_FAILURE);
    }

    //Calculate the buffer size for the fragmented file
    int bufferSize = fileSize / threadCount;
    
    threadBuffer = malloc(threadCount * sizeof(char*));
    if (threadBuffer == NULL) {
        perror("Failed to allocate thread buffer array!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < threadCount; i++) {
        threadBuffer[i] = malloc(bufferSize);
        if (threadBuffer[i] == NULL) {
            perror("Failed to instantiate thread buffer!\n");
            exit(EXIT_FAILURE);
        }

        if (pread(file, threadBuffer[i], bufferSize, bufferSize * i) == -1) {
            perror("Unable to read file!\n");
            exit(EXIT_FAILURE);
        }
    }

    //Instantiate initial counterArray that will hold all the words and their frequencies
    counterArray = malloc(arraySize * sizeof(WordFreq));
    if (counterArray == NULL) {
        perror("Failed to allocate counter array");
        exit(EXIT_FAILURE);
    }

    // *** TO DO *** Start your thread processing
    // Wait for the threads to finish

    pthread_t threads[threadCount];

    for (int i = 0; i < threadCount; i++) {
        if (pthread_create(&threads[i], NULL, &counterThread, threadBuffer[i]) != 0) {
            perror("Failed to create a thread!\n");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < threadCount; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join threads!\n");
            exit(EXIT_FAILURE);
        }
    }

    // ***TO DO *** Process TOP 10 and display

    //Process the top 10 frequent words and display them
    quickSort(counterArray, 0, counterArraySize - 1);

    printf("Printing top 10 words 6 characters or more.\n");
    for (int i = 0; i < 10; i++) { 
        printf("Number %d is %s with a count of %d\n", i + 1, counterArray[i].word, counterArray[i].freq);
    }

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec) {
        --sec;
        n_sec = n_sec + 1000000000L;
    }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************

    // ***TO DO *** cleanup

    //Clean up code and free buffers
    close(file);

    if (pthread_mutex_destroy(&lock) != 0) {
        perror("Failed to destroy mutex!\n");
    }

    for (int i = 0; i < counterArraySize; i++) {
        free(counterArray[i].word);
    }

    free(counterArray);

    for (int i = 0; i < threadCount; i++) {
        free(threadBuffer[i]);
    }
    free(threadBuffer);

    return 0;
}