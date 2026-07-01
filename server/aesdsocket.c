#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

bool caught_sigint = false;
bool caught_sigterm = false;


static void signal_handler(int signal_number){
    int saved_errno = errno;
    if ( signal_number == SIGINT){
        caught_sigint = true;
    }
    if ( signal_number == SIGTERM ){
        caught_sigterm = true;
    }
    errno = saved_errno;
}

int main(int argc, char *argv[]){
    int daemon_mode = false;

    if ( (argc > 1) && strcmp(argv[1], "-d") == 0){

        daemon_mode = true;
    }


    struct sigaction new_action;

    memset(&new_action,0,sizeof(struct sigaction));

    new_action.sa_handler = signal_handler;

    sigaction(SIGINT, &new_action, NULL);
    sigaction(SIGTERM, &new_action, NULL);

    remove("/var/tmp/aesdsocketdata");

    int sockfd;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        return -1;
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in sockaddr;

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(9000);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*) &sockaddr, sizeof(sockaddr))<0){
        return -1;
    }

    if (listen(sockfd, 3) < 0){
        return -1;
    }

    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) return -1;
        if (pid > 0) exit(0);
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (!caught_sigint && !caught_sigterm){

        int client_fd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_len);

        if ( client_fd < 0){
            if (caught_sigint || caught_sigterm) {
                break;
            }
            return -1;
        }

        openlog(NULL, LOG_CONS, LOG_USER);
        char* client_ip_string = inet_ntoa(client_addr.sin_addr);

        syslog(LOG_DEBUG, "Accepted connection from %s", client_ip_string);

        closelog();



        FILE *file = fopen("/var/tmp/aesdsocketdata", "a");
        if (file == NULL){
            return -1;
        }

        //the thing should write from here

        size_t buf_size = 1024;
        size_t total_read = 0;
        char* buf = malloc(buf_size);
        if (buf == NULL){
            return -1;
        }

        while (1) {

            if (total_read >= buf_size){
                buf_size *= 2;
                char* new_buf = realloc(buf, buf_size);
                if ( new_buf == NULL ){
                    free(buf);
                    return -1;
                }
                buf = new_buf;
            }

            int val_read = read(client_fd, buf + total_read, buf_size - total_read);
            if (val_read == -1){
                free(buf);
                return -1;
            }
            if (val_read == 0) {
                // client closed connection
                break;
            }
            total_read += val_read;


            int found_newline = 0;
            for (int i = 0; i < total_read; i++){
                if (buf[i] == '\n'){
                    found_newline = 1;
                    break;
                }
            }
            if (found_newline) {
                break;  // exit the WHILE loop now that we have a complete packet
            }


        }

        fwrite(buf, 1, total_read, file);

        fclose(file);

        FILE *read_file = fopen("/var/tmp/aesdsocketdata", "r");
        if (read_file == NULL) {
            return -1;
        }

        char send_buf[1024];
        size_t bytes_read;
        while ((bytes_read = fread(send_buf, 1, sizeof(send_buf), read_file)) > 0) {
            write(client_fd, send_buf, bytes_read);
        }

        fclose(read_file);

        openlog(NULL, LOG_CONS, LOG_USER);

        syslog(LOG_DEBUG, "Closed connection from %s", client_ip_string);

        closelog();

        free(buf);

        close(client_fd);

    }

    openlog(NULL, LOG_CONS, LOG_USER);
    syslog(LOG_DEBUG, "Caught signal, exiting");
    closelog();
    close(sockfd);
    remove("/var/tmp/aesdsocketdata");
    return 0;
}

