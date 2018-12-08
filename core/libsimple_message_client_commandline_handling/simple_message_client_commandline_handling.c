/* ================================================================ */
/**
 * @file simple_message_client_commandline_handling.c
 * TCP/IP Client for bulletin board.
 *
 * In the course "Verteilte Computer Systeme" the students shall
 * implement a bulletin board. It shall consist of a spawning TCP/IP
 * server which executes the business logic provided by the lector,
 * and a suitable TCP/IP client.
 *
 * This source file contains the commandline handling for the client.
 *
 * @author thomas.galla@technikum-wien.at
 * @date 2011/10/23
 */
/*
 * $Id:$
 */

/*
 * -------------------------------------------------------------- includes --
 */

#include <stdlib.h>
#include <getopt.h>

#include "simple_message_client_commandline_handling.h"

/*
 * --------------------------------------------------------------- defines --
 */

/*
 * -------------------------------------------------------------- typedefs --
 */

/*
 * --------------------------------------------------------------- globals --
 */

/*
 * ------------------------------------------------- function declarations --
 */

/*
 * ------------------------------------------------------------- functions --
 */

/**
 *
 * \brief Parse the command line
 *
 * This function parses the command line and extracts the arguments
 *
 * \param argc [IN] - number of command line arguments.
 * \param argv [IN] - array of command line arguments.
 * \param usagefunc [IN] - pointer to a function called for diplaying usage information.
 * \param server [OUT] - string containing the IP address or the FQDN of the server
 * \param port [OUT] - string containing the port number or the service name
 * \param user [OUT] - string containing the name of the user
 * \param message [OUT] - string containing the message
 * \param img_url [OUT] - string containing the image URL
 * \param verbose [OUT] - int containing info whether output shall be verbose or not
 *
 * \return Upon successful execution, the function returns and the output parameters
 *         \a port, \a server, \a message, and  \a img_url are filled properly (Note that
 *         img_url might be NULL, since it's optional on the commandline.). - Upon
 *         failure the function prints usage information and terminates the program by
 *         calling \a usagefunc.
 *
 */
void smc_parsecommandline(
    int argc,
    const char * const argv[],
    smc_usagefunc_t usagefunc, 
    const char **server,
    const char **port,
    const char **user,
    const char **message,
    const char **img_url,
    int *verbose
    )
{
    int c;

    *server = NULL;
    *port = NULL;
    *user = NULL;
    *message = NULL;
    *img_url = NULL;
    *verbose = FALSE;

    struct option long_options[] =
    {
        {"server", 1, NULL, 's'},
        {"port", 1, NULL, 'p'},
        {"user", 1, NULL, 'u'},
        {"image", 1, NULL, 'i'},
        {"message", 1, NULL, 'm'},
        {"verbose", 0, NULL, 'v'},
        {"help", 0, NULL, 'h'},
        {0, 0, 0, 0}
    };

    while (
        (c = getopt_long(
             argc,
             (char ** const) argv,
             "s:p:u:i:m:hv",
             long_options,
             NULL
             )
            ) != -1
        )
    {
        switch (c)
        {
            case 's':
                *server = optarg;
                break;

            case 'p':
                *port = optarg;
                break;

            case 'u':
                *user = optarg;
                break;

            case 'i':
                *img_url = optarg;
                break;

            case 'm':
                *message = optarg;
                break;

            case 'v':
                *verbose = TRUE;
                break;

            case 'h':
	      usagefunc(stdout, argv[0], EXIT_SUCCESS);
                break;

            case '?':
            default:
                usagefunc(stderr, argv[0], EXIT_FAILURE);
                break;
        }
    }

    if (
        (optind != argc) ||
        (*port == NULL) ||
        (*server == NULL) ||
        (*user == NULL) ||
        (*message == NULL)
        )
    {
        usagefunc(stderr, argv[0], EXIT_FAILURE);
    }
}

/*
 * =================================================================== eof ==
 */
