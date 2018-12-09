/* ================================================================ */
/**
 * @file simple_message_server.c
 * TCP/IP Server for bulletin board.
 *
 * A simple TCP/IP server to demonstrate how sockets works.
 *
 * This source file contains the spawning server.
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
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

/*
 * --------------------------------------------------------------- defines --
 */

#define SERVER_LOGIC "./simple_message_server_logic"

/*
 * -------------------------------------------------------------- typedefs --
 */

/*
 * --------------------------------------------------------------- globals --
 */

/*
 * ------------------------------------------------- function declarations --
 */

static int parse_parameters(int argc, char **argv, char **port);
static int create_socket(char *port);
static int fork_server(int socket_fd);
static void child_signal(int signal);

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
int main(int argc, char *argv[]) {
    char *port = NULL;
    int socket;

    // Parse the passed parameters or throw an error if this is not possible
    if (parse_parameters(argc, argv, &port) == -1) {
        fprintf(stderr, "Usage: %s -p port [-h]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Create a socket or throw an error if its not possible
    if ((socket = create_socket(port)) == -1) {
        return EXIT_FAILURE;
    }

    // Wait and accept connections and fork a new server or throw error if not possible
    if (fork_server(socket) == -1) {
        return EXIT_FAILURE;
    }

    close(socket);

    return 0;
}

/**
 * \brief Reads all parameters from command line
 *
 * Parameters are parsed from the command line and are also validated.
 *
 * \param argc - number of command line arguments.
 * \param argv - array of command line arguments.
 * \param port - string with information of the port
 *
 * \return Information about success or failure in the execution
 * \retval 0 successful execution
 * \retval -1 failed execution.
 */
static int parse_parameters(int argc, char **argv, char **port) {
    int options;
    long port_number;
    char *check_convert;

    if (argc < 2) {
        warnx("Not enough arguments!");
        return -1;
    }

    // Get passed options
    while ((options = getopt(argc, argv, "p:h")) != -1) {
        switch (options) {
            case 'h':
                return -1;
            case 'p':
                // Clear error info
                errno = 0;

                // Convert port to validate
                port_number = strtol(optarg, &check_convert, 10);

                if(errno != 0){
                    warnx("Something went wrong parsing port: %i", errno);
                    return -1;
                }

                if(*check_convert != '\0'){
                    warnx("Port need to be a number!");
                    return -1;
                }

                if(port_number < 1 || port_number > 65535){
                    warnx("Port not in rage (1-65535)!");
                    return -1;
                }

                *port = optarg;
                break;
            default:
                return -1;
        }
    }

    if (optind < argc) {
        return -1;
    }

    return 0;
}

/**
 * \brief Create socket and bind
 *
 * A socket is created and bind to an address.
 *
 * \param port - string with information of the port
 *
 * \return Socket file descriptor
 * \retval -1 failed execution.
 * \retval fd file descriptor for socket
 */
static int create_socket(char *port) {
    int socket_fd = -1;
    struct addrinfo base_addr;
    struct addrinfo *base_info, *addr_iterator;
    const int address_reuse = 1;

    // Set basic address information
    memset(&base_addr, 0, sizeof(base_addr));
    base_addr.ai_family = AF_INET;
    base_addr.ai_socktype = SOCK_STREAM;
    base_addr.ai_flags = AI_PASSIVE;

    // Get all available addresses
    if (getaddrinfo(NULL, port, &base_addr, &base_info) != 0) {
        return -1;
    }

    // Iterate through all available sockets and save socket file descriptor
    for (addr_iterator = base_info; addr_iterator != NULL; addr_iterator = addr_iterator->ai_next) {
        // Get new socket or continue if failed
        if ((socket_fd =
                socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol)) == -1)
            continue;

        // If connection abort reuse address
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &address_reuse, sizeof(int)) == -1) {
            close(socket_fd);
            continue;
        }

        // Finish connection
        if (bind(socket_fd, addr_iterator->ai_addr, addr_iterator->ai_addrlen) == -1) {
            close(socket_fd);
            continue;
        } else
            break;
    }

    freeaddrinfo(base_info);

    if (addr_iterator == NULL) {
        warnx("Bind socket to address failed!");
        return -1;
    }

    return socket_fd;
}

/**
 * \brief Fork the server
 *
 * Clones the calling process when new connection happens and passes the connection.
 *
 * \param socket_fd - integer value of the server socket
 *
 *
 * \return Information about success or failure in the execution
 * \retval -1 failed execution.
 */
static int fork_server(int socket_fd) {
    int active_connection;
    struct sigaction signal_action;
    struct sockaddr_in socket_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Configure signal handler
    signal_action.sa_handler = child_signal;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = SA_RESTART;

    // Set signal action
    if (sigaction(SIGCHLD, &signal_action, NULL) == -1) {
        close(socket_fd);
        return -1;
    }

    // Open socket to listen
    if (listen(socket_fd, SOMAXCONN) == -1) {
        close(socket_fd);
        return -1;
    }

    // Wait for connections
    while (1) {
        if ((active_connection = accept(socket_fd, (struct sockaddr *)&socket_addr, &addr_len)) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                close(socket_fd);
                return -1;
            }
        }

        // When connection occur fork new process
        switch (fork()) {
            case -1:
                close(active_connection);
                break;
            case 0:
                close(socket_fd);

                // Redirect connection, passing to STDIN and closing old one
                if (dup2(active_connection, STDIN_FILENO) == -1) {
                    _exit(EXIT_FAILURE);
                }

                // Redirect connection, passing to STDOUT and closing old one
                if (dup2(active_connection, STDOUT_FILENO) == -1) {
                    _exit(EXIT_FAILURE);
                }

                close(active_connection);
                execl(SERVER_LOGIC, "", NULL);

                // Will only be reached if starting logic failed
                _exit(EXIT_FAILURE);
            default:
                close(active_connection);
                break;
        }
    }
}


/**
 * \brief Child signal handler
 *
 * Waits for child-process to die.
 *
 * \param signal - integer value for signal
 */
static void child_signal(int signal) {
    (void)signal;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/*
 * =================================================================== eof ==
 */
