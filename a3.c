#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define SHM_KEY 11242

void copy(const char *input, size_t length, char* output){
    while(length--){
        if(*input!='\0')
            *output++ = *input;
        input++;

    }
    //*output = '\0';
}

int main(){
    int fd_resp, fd_req;

    char buf[500];
    char sizeReq=0;
    char *success = "SUCCESS";
    char sizeSuccess = strlen(success);
    char* error = "ERROR";
    char sizeErr = strlen(error);
    unsigned int shmSize = 0;
    int shmId;
    char *sharedValue = NULL;
    char *data = NULL;
    int fdFile;
    int sizeF;

    unlink("RESP_PIPE_25096");

    if(mkfifo("RESP_PIPE_25096", 0600) < 0){
        printf("ERROR \ncannot create the response pipe\n");
        exit(1);
    }

   if((fd_req = open("REQ_PIPE_25096", O_RDWR)) < 0){
        printf("ERROR\ncannot open the request pipe\n");
        exit(1);
    }
    fd_resp = open("RESP_PIPE_25096", O_RDWR);
    int sizeConnect = strlen("CONNECT");
    write(fd_resp, &sizeConnect, 1); //1 the size of an int (1 byte)
    int n = write(fd_resp, "CONNECT", sizeConnect);

    if(n > 0){
        printf("SUCCESS\n");
    } else printf("error writing\n");

    while(1){
        read(fd_req, &sizeReq, 1);
        int m = read(fd_req, buf, sizeReq);
        buf[m] = 0;

        if(strcmp(buf, "PING") == 0){
            int sizepp = strlen("PING");
            unsigned int variant = 25096;
            write(fd_resp, &sizepp, 1);
            write(fd_resp, "PING", sizepp);
            write(fd_resp, &sizepp, 1);
            write(fd_resp, "PONG", sizepp);
            write(fd_resp, &variant, sizeof(unsigned int));
        } else {
            if(strcmp(buf, "CREATE_SHM") == 0){

                read(fd_req, &shmSize, sizeof(unsigned int));
                int create = strlen("CREATE_SHM");

                shmId = shmget(SHM_KEY, shmSize * sizeof(char), IPC_CREAT | 0664);
                if(shmId < 0) {
                    write(fd_resp, &create, 1);
                    write(fd_resp, "CREATE_SHM", create);
                    write(fd_resp, &sizeErr, 1);
                    write(fd_resp, error, sizeErr);
                } else {
                    write(fd_resp, &create, 1);
                    write(fd_resp, "CREATE_SHM", create);
                    write(fd_resp, &sizeSuccess, 1);
                    write(fd_resp, success, sizeSuccess);
                }

            }
            else{
                if(strcmp(buf, "WRITE_TO_SHM") == 0){
                    unsigned int offset = 0;
                    unsigned int value = 0;
                    read(fd_req, &offset, sizeof(unsigned int));
                    read(fd_req, &value, sizeof(unsigned int));
                    char writeToShmSize = strlen("WRITE_TO_SHM");

                    sharedValue = shmat(shmId, NULL, 0);
                    printf("%d %d %d\n", offset, value, shmSize);
                    if(sharedValue == (void*)-1){
                        perror("Could not attach to shm");
                        exit(1);
                    }
                    if((offset < shmSize - sizeof(unsigned int)) && (offset > 0)){

                        printf("found value %d\n", *sharedValue + offset);
                        memcpy(offset + sharedValue, &value, 4);
                        printf("sent value %d\n", *sharedValue + offset);

                        write(fd_resp, &writeToShmSize, 1);
                        write(fd_resp, buf, writeToShmSize);
                        write(fd_resp, &sizeSuccess, 1);
                        write(fd_resp, success, sizeSuccess);
                    }
                    else {
                        printf("end value %d\n", *sharedValue);
                        write(fd_resp, &writeToShmSize, 1);
                        write(fd_resp, buf, writeToShmSize);
                        write(fd_resp, &sizeErr, 1);
                        write(fd_resp, error, sizeErr);
                    }

                } else {
                    if(strcmp(buf, "MAP_FILE") == 0){
                    int fileSize = 0;
                    char fileName[100];
                    int sizeMapFile = strlen("MAP_FILE");

                    read(fd_req, &fileSize, 1);
                    int m = read(fd_req, fileName, fileSize);
                    fileName[m] = 0;

                    fdFile = open(fileName, O_RDONLY);
                    if(fdFile == -1){
                        perror("Could not open file\n");
                        write(fd_resp, &sizeMapFile, 1);
                        write(fd_resp, "MAP_FILE", sizeMapFile);
                        write(fd_resp, &sizeErr, 1);
                        write(fd_resp, error, sizeErr);
                        exit(1);
                    }
                    sizeF = lseek(fdFile, 0, SEEK_END);
                    lseek(fdFile, 0 ,SEEK_SET);
                    data = (char*)mmap(NULL, sizeF, PROT_READ, MAP_SHARED, fdFile, 0);

                    if(data == (void*)-1){
                        perror("Could not map file\n");
                        write(fd_resp, &sizeMapFile, 1);
                        write(fd_resp, "MAP_FILE", sizeMapFile);
                        write(fd_resp, &sizeErr, 1);
                        write(fd_resp, error, sizeErr);
                        close(fdFile);
                        exit(1);
                    } else{
                        write(fd_resp, &sizeMapFile, 1);
                        write(fd_resp, "MAP_FILE", sizeMapFile);
                        write(fd_resp, &sizeSuccess, 1);
                        write(fd_resp, success, sizeSuccess);
                    }

                    //munmap(data, sizeF);
                    //close(fdFile);

                    } else{
                        if(strcmp(buf, "READ_FROM_FILE_OFFSET") == 0){
                            unsigned int offset = 0;
                            unsigned int no_of_bytes = 0;

                            char readFromOffsetSize = strlen("READ_FROM_FILE_OFFSET");

                            read(fd_req, &offset, sizeof(unsigned int));
                            read(fd_req, &no_of_bytes, sizeof(unsigned int));
                            //char *readStuff = (char*)malloc((no_of_bytes + 2)*sizeof(char));
                            printf("%d %d\n", offset, no_of_bytes);
                            sharedValue = shmat(shmId, NULL, 0);
                            if((offset + no_of_bytes > sizeF) || (data == (void*)-1) || (sharedValue==(void*)-1)){
                                printf("true\n");
                                write(fd_resp, &readFromOffsetSize, 1);
                                write(fd_resp, "READ_FROM_FILE_OFFSET", readFromOffsetSize);
                                write(fd_resp, &sizeErr, 1);
                                write(fd_resp, error, sizeErr);

                            } else{
                                printf("true\n");
                                printf("SHARED VAL : %s\n", sharedValue);
                                //copy(data+offset, no_of_bytes, readStuff);

                                //printf("%s %ld\n", readStuff, strlen(readStuff));
                                memcpy(sharedValue, data+offset, no_of_bytes);

                                printf("shared %s\n", sharedValue);
                                write(fd_resp, &readFromOffsetSize, 1);
                                write(fd_resp, "READ_FROM_FILE_OFFSET", readFromOffsetSize);
                                write(fd_resp, &sizeSuccess, 1);
                                write(fd_resp, success, sizeSuccess);
                                //free(readStuff);
                            }

                        }else {
                            if(strcmp(buf, "READ_FROM_FILE_SECTION") == 0){
                                int section_no = 0;
                                unsigned int offset = 0;
                                unsigned int no_of_bytes = 0;
                                char readFromSecSize = strlen("READ_FROM_FILE_SECTION");
                                read(fd_req, &section_no, sizeof(int));
                                read(fd_req, &offset, sizeof(unsigned int));
                                read(fd_req, &no_of_bytes, sizeof(unsigned int));
                                printf("%d %d %d\n", section_no, offset, no_of_bytes);

                                int header_size = 0;
                                memcpy(&header_size, data+sizeF-3,2);
                                printf("head size %d\n", header_size);

                                int no_of_sections = 0;
                                memcpy(&no_of_sections, data + sizeF - header_size + 2, 1);
                                printf("nr sect %d\n", no_of_sections);
                                unsigned int sect_offset = 0;
                                memcpy(&sect_offset, data + sizeF - header_size + 3 + section_no*(18+2+4+4) - 8, 4);

                                printf(" |||section oofset: %d\n", sect_offset);
                                sharedValue = shmat(shmId, NULL, 0);
                                if(section_no > no_of_sections || section_no < 1|| (data == (void*)-1) || (sharedValue==(void*)-1)){
                                    printf("true\n");
                                    write(fd_resp, &readFromSecSize, 1);
                                    write(fd_resp, "READ_FROM_FILE_SECTION", readFromSecSize);
                                    write(fd_resp, &sizeErr, 1);
                                    write(fd_resp, error, sizeErr);

                                } else{
                                    memcpy(sharedValue, (data + sect_offset + offset), no_of_bytes);
                                    //printf("shared %s\n", sharedValue);
                                    write(fd_resp, &readFromSecSize, 1);
                                    write(fd_resp, "READ_FROM_FILE_SECTION", readFromSecSize);
                                    write(fd_resp, &sizeSuccess, 1);
                                    write(fd_resp, success, sizeSuccess);
                                }
                            } else {
                                break;
                            }
                        }

                    }
                }
            }
        }

    }

    close(fd_req);
    close(fd_resp);
    unlink("RESP_PIPE_25096");
    unlink("REQ_PIPE_25096");
    return 0;
}




