/* ================================================================ */
/**
 * @file simple_message_client.c
 * TCP/IP Client for bulletin board.
 *
 * A simple TCP/IP client to demonstrate how sockets work.
 *
 * @author ic18b081@technikum-wien.at
 * @author ic17b503@technikum-wien.at
 * @date 2018/12/08
 */
/*
 * $Id:$
 */

/*
 * -------------------------------------------------------------- includes --
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <simple_message_client_commandline_handling.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
//#include <arpa/inet.h>    //Can be used for printf(IP);

/*
 * --------------------------------------------------------------- defines --
 */

#define BUFFER_SIZE 255
#define print_v(fmt, ...)                                   \
  if (verbose)                                              \
    fprintf(stderr, "%s(): " fmt, __func__, __VA_ARGS__);

/*
* -------------------------------------------------------------- typedefs --
*/

/*
 * --------------------------------------------------------------- globals --
 */

static int verbose;

/*
 * ------------------------------------------------- function declarations --
 */

static void usage(FILE *stream, const char *cmd, int exitcode);
static int connect_to_server(const char *server, const char *port);
static int send_req(FILE *write_fd, const char *user, const char *message, const char *img_url);
static int read_resp(FILE *read_fd);

/*
 * ------------------------------------------------------------- functions --
 */

/**
 * \brief Main entry point of program.
 *
 * This function is the main entry point of the program.
 *
 * \param argc - number of command line arguments.
 * \param argv - array of command line arguments.
 *
 * \return Information about success or failure in the execution
 * \retval EXIT_FAILURE failed execution.
 * \retval EXIT_SUCCESS successful execution
 */
int main(const int argc, const char *const argv[]) {
    const char *server;
    const char *port;
    const char *user;
    const char *message;
    const char *img_url;

    int sfd;
    FILE *write_fd = NULL;
    FILE *read_fd = NULL;

    smc_parsecommandline(argc, argv, usage, &server, &port, &user, &message, &img_url, &verbose);
    print_v("Using the following options: server=%s port=%s, user=%s, img_url=%s, message=%s\n", server, port, user, img_url, message);

    if ((sfd = connect_to_server(server, port)) == -1){
        fprintf(stderr, "Could not connect to server\n");
        close(sfd);
        return EXIT_FAILURE;
    }

    write_fd = fdopen(sfd, "w");
    if (write_fd == NULL) {
        fprintf(stderr, "Could not open write fd\n");
        fclose(write_fd);
        close(sfd);
        return EXIT_FAILURE;
    }

    if (send_req(write_fd, user, message, img_url)){
        fprintf(stderr, "Error at sending request\n");
        fclose(write_fd);
        close(sfd);
        return EXIT_FAILURE;
    }
    if (shutdown(sfd, SHUT_WR) == -1){
        fprintf(stderr, "Error could not shutdown write part of socket\n");
        fclose(write_fd);
        close(sfd);
        return EXIT_FAILURE;
    }
    print_v("%s", "Closed write part of socket\n")

    read_fd = fdopen(sfd, "r");
    if (read_fd == NULL) {
        fprintf(stderr, "Could not open read fd\n");
        fclose(write_fd);
        fclose(read_fd);
        close(sfd);
        return EXIT_FAILURE;
    }

    if (read_resp(read_fd) != 0){
        fprintf(stderr, "Error at reading response\n");
        fclose(write_fd);
        fclose(read_fd);
        close(sfd);
        return EXIT_FAILURE;
    }

    fclose(write_fd);
    fclose(read_fd);
    close(sfd);

    return 0;
}

/**
 * \brief Create socket and connect to server
 *
 * \param server - number of command line arguments.
 * \param port - string with information of the port
 *
 * \return file descriptor
 * \retval On success, a file descriptor for the new socket is returned
 * \retval On error, -1 is returned
 */
static int connect_to_server(const char *server, const char *port) {
    int sfd = -1, s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */

    s = getaddrinfo(server, port, &hints, &result);
    if (s != 0) {
        warnx("getaddrinfo: %s\n", gai_strerror(s));
        return sfd;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        print_v("Connecting to IPv%d address.\n", rp->ai_family == PF_INET6 ? 6 : 4);

        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            print_v("%s\n","Failed");
            continue;
        }
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            print_v("%s\n","Success");
            break; /* Success */
        } else
        print_v("%s\n","Failed");
    }

    if (rp == NULL) { /* No address succeeded */
        warnx("Could not connect\n");
        freeaddrinfo(result);
        return -1;
    }

    freeaddrinfo(result); /* No longer needed */
    return sfd;
}

/**
 * \brief Prints the usage information of this program to a defined stream and exits the program
 *
 * \param stream - Stream to print the usage information
 * \param cmd - name of executable
 * \param exitcode - returned exit code of the program
 */
static void usage(FILE *stream, const char *cmd, int exitcode) {
    fprintf(stream, "usage: %s options\n", cmd);
    fprintf(stream, "options:\n");
    fprintf(stream, "        -s, --server <server>   full qualified domain name or IP address of the server\n");
    fprintf(stream, "        -p, --port <port>       well-known port of the server [0..65535]\n");
    fprintf(stream, "        -u, --user <name>       name of the posting user\n");
    fprintf(stream, "        -i, --image <URL>       URL pointing to an image of the posting user\n");
    fprintf(stream, "        -m, --message <message> message to be added to the bulletin board\n");
    fprintf(stream, "        -v, --verbose           verbose output\n");
    fprintf(stream, "        -h, --help\n");
    exit(exitcode);
}


/**
 * \brief Format the message and send a request to the Server
 *
 * \param write_fd - FILE pointer to write the request
 * \param user - username which get added to request
 * \param message - message which get added to request
 * \param img_url - img_url which get added to request
 *
 * @returns 0 if everything went well or -1 in case of error
 */
static int send_req(FILE *write_fd, const char *user, const char *message, const char *img_url) {
    const char *pre_user = "user=";
    const char *pre_message = "\n";
    const char *pre_img_url = "";

    if (img_url != NULL) {
        pre_img_url = "\nimg=";
    } else {
        img_url = "";
    }

    print_v("Going to send the following message:%s%s%s%s%s%s\n", pre_user, user, pre_img_url, img_url, pre_message, message)

    if (fprintf(write_fd, "%s%s%s%s%s%s", pre_user, user, pre_img_url, img_url, pre_message, message) < 0){
        warnx("Could not write to file descriptor");
        return -1;
    }
    if (fflush(write_fd) != 0){
        warnx("Could not flush output buffer");
        return -1;
    }

    return 0;

}

/**
 * \brief Fetch and well-form server response.
 * Writes obtained Files to disk.
 *
 * \param read_fd - FILE pointer where to read the responst
 *
 * @returns 0 if everything went well or -1 in case of error
 */
static int read_resp(FILE *read_fd) {
    char *line = NULL;
    char buffer[BUFFER_SIZE];
    char *file_name = NULL;
    char *cmp = NULL;
    long status;
    long file_len = 0;
    long counter = 0;
    long to_process = 0;
    size_t read;
    size_t len = 0;
    FILE *fp = NULL;

    if ((getline(&line, &len, read_fd)) != -1) {
        cmp = strtok(line, "=");
        if (strncmp(cmp,"status",7) != 0){
            warnx("Expected: status= from server\n Received: %s=",cmp);
            return -1;
        }
        errno = 0;
        status = strtol(strtok(NULL, "\n"), NULL, 10);

        if(errno != 0){
            warnx("Something went wrong parsing status: %i", errno);
            return -1;
        }
        print_v("Obtained and parsed Status from server\nStatus: %ld\n", status);
    } else {
        warnx("Could not well-form status - Received no line from Server\n");
        return -1;
    }

    while ((getline(&line, &len, read_fd)) != -1) {
        //get file_name
        cmp = strtok(line, "=");
        if (strncmp(cmp,"file",5) != 0){
            warnx("Expected: file= from server\n Received: %s=",cmp);
            return -1;
        }

        if ((file_name = strtok(NULL, "\n")) == NULL){
            warnx("Failed read file_name");
            return -1;
        }
        print_v("Obtained and parsed Filename from server\nFilename: %s\n",file_name);

        //open file
        if ((fp = fopen(file_name, "w+")) == NULL) {
            warnx("Could not open File: %s\n",file_name);
            return -1;
        }

        //get file_len
        if ((getline(&line, &len, read_fd)) != -1) {
            cmp = strtok(line, "=");
            if (strncmp(cmp,"len",4) != 0){
                warnx("Expected: len= from server\n Received: %s=",cmp);
                return -1;
            }
            errno = 0;
            if ((file_len = strtol(strtok(NULL, "\n"), NULL, 10)) == 0){
                warnx("Could not parse received file_len: %i", errno);
                return -1;
            }
            print_v("Obtained and parsed File length from server\nFile length: %ld\n",file_len);
        } else {
            warnx("Could not read Filename line\n");
            return -1;
        }

        counter = file_len;

        //read/write data
        while (counter != 0) {
            to_process = counter;
            if (to_process > BUFFER_SIZE) {
                to_process = BUFFER_SIZE;
            }

            // read [buffersize] from file descriptor
            read = fread(buffer, 1, (size_t) to_process, read_fd);
            if ((long)read < to_process){
                warnx("File Data read Error\n");
                return -1;
            }

            // write [buffersize] bytes to file
            if ((long)(fwrite(buffer, sizeof(char), (size_t) to_process, fp)) < to_process){
                warnx("File Data write Error\n");
                return -1;
            }

            counter -= to_process;
            print_v("Copied %ld Bytes to File - %ld Bytes left\n",to_process,counter);
        }

        if (fclose(fp) == EOF) {
            warnx("Error closing filestream\n");
            return -1;
        }
    }
    return 0;
}

/*
 * =================================================================== eof ==
 */
