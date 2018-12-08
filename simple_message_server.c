/* ================================================================ */
/**
 * @file simple_message_server.c
 * TCP/IP Server for bulletin board.
 *
 * A simple TCP/IP server to demonstrate how sockets works.
 *
 * This source file contains the spawning server.
 *
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

#define SERVER_LOGIC "/usr/local/bin/simple_message_server_logic"

/*
 * -------------------------------------------------------------- typedefs --
 */

/*
 * --------------------------------------------------------------- globals --
 */

/*
 * ------------------------------------------------- function declarations --
 */

static int get_parameters(int argc, char *argv[], char *port[]);
static int create_socket(char *port);
static int fork_server(int socket_file_descriptor);
static void child_signal(int sig);

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
    char *port_number = NULL;
    int socket = -1;

    // Parse the passed parameters or throw an error if this is not possible.
    if (get_parameters(argc, argv, &port_number) == -1) {
        fprintf(stderr, "Usage: %s -p port [-h]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Create a socket or throw an error if its not possible.
    if ((socket = create_socket(port_number)) == -1) {
        return EXIT_FAILURE;
    }

    // Accept connections and fork the server or throw error if not possible.
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
 * \return Information about succes or failure in the execution
 * \retval 0 successful execution
 * \retval -1 failed execution.
 */
static int get_parameters(int argc, char *argv[], char *port[]) {
    int options;
    long port_number;
    char *check_convert;

    // Reset errors
    errno = 0;

    if (argc < 2) {
        warnx("Not enough arguments!");
        return -1;
    }

    while ((options = getopt(argc, argv, "p:h")) != -1) {
        switch (options) {
            case 'h':
                return -1;
            case 'p':
                port_number = strtol(optarg, &check_convert, 10);
                //Check if error happen and if converting port number went wrong
                if (errno != 0 || *check_convert != '\0' ||
                    // Check port number rage
                    port_number < 1 || port_number > 65535) {
                    warnx("Something wrong with the port!");
                    return -1;
                }
                *port = optarg;
                break;
            default:
                return -1;
        }
    }

    if (optind < argc) {
        warnx("Wrong arguments present!");
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
 * \retval Any integer (except -1) is socket file descriptor
 */
static int create_socket(char *port) {
    int socket_file_descriptor = -1;
    struct addrinfo base_address;
    struct addrinfo *base_info, *addr_it;
    const int address_reuse = 1;

    // Get basic address information.
    memset(&base_address, 0, sizeof(base_address));
    base_address.ai_family = AF_INET;
    base_address.ai_socktype = SOCK_STREAM;
    base_address.ai_flags = AI_PASSIVE;

    // Get all available addresses
    if (getaddrinfo(NULL, port, &base_address, &base_info) != 0) {
        return -1;
    }

    // Iterate through all available addresses and save socket file descriptor
    for (addr_it = base_info; addr_it != NULL; addr_it = addr_it->ai_next) {
        // Get new socket or continue if failed
        if ((socket_file_descriptor = socket(addr_it->ai_family, addr_it->ai_socktype, addr_it->ai_protocol)) == -1)
            continue;

        // If connection abort reuse address
        if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &address_reuse, sizeof(int)) == -1) {
            close(socket_file_descriptor);
            continue;
        }

        // Finish connection
        if (bind(socket_file_descriptor, addr_it->ai_addr, addr_it->ai_addrlen) == -1) {
            close(socket_file_descriptor);
            continue;
        } else
            break;
    }

    if (addr_it == NULL) {
        warnx("Bind socket to address failed!");
        freeaddrinfo(base_info);
        return -1;
    }

    freeaddrinfo(base_info);

    return socket_file_descriptor;
}

/**
 * @brief a forking server
 *
 * @param sock the server socket
 *
 * @returns 0 if everything went well or -1 in case of error
 */
static int fork_server(int socket_file_descriptor) {
    int open_socket;
    struct sockaddr_in socket_address;
    socklen_t address_size = sizeof(struct sockaddr_in);

    struct sigaction signal_action;
    signal_action.sa_handler = child_signal;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &signal_action, NULL) == -1) {
        close(socket_file_descriptor);
        return -1;
    }

    if (listen(socket_file_descriptor, SOMAXCONN) == -1) {
        close(socket_file_descriptor);
        return -1;
    }

    while (1) {
        if ((open_socket = accept(socket_file_descriptor, (struct sockaddr *)&socket_address, &address_size)) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                close(socket_file_descriptor);
                return -1;
            }
        }

        switch (fork()) {

            case -1:
                close(open_socket);
                break;

            case 0:
                close(socket_file_descriptor);
                if (dup2(open_socket, STDIN_FILENO) == -1) {
                    _exit(EXIT_FAILURE);
                }
                if (dup2(open_socket, STDOUT_FILENO) == -1) {
                    _exit(EXIT_FAILURE);
                }
                close(open_socket);
                execl(SERVER_LOGIC, "", NULL);
                _exit(EXIT_FAILURE);

            default:
                close(open_socket);
                break;
        }
    }
}

/**
 * @brief handles SIGCHLD by waiting for dead processes
 *
 * @param sig the signal number (ignored)
 */
static void child_signal(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
