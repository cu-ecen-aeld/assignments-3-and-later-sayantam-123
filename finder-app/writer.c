#include<stdio.h>
#include<syslog.h>
#include<stdlib.h>

int main(int argc, char* argv[]){

    openlog(NULL, LOG_CONS, LOG_USER);

    if(argc != 3){
        syslog(LOG_ERR, "Invalid number of arguments");
        return 1;
    }

    FILE *fptr;
    fptr = fopen(argv[1], "w");

    if(fptr == NULL){
        syslog(LOG_ERR, "Failed to open file %s", argv[1]);
        return 1;
    }

    fprintf(fptr, "%s", argv[2]);
    fclose(fptr);

    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);

    return 0;
}
