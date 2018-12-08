/* ================================================================ */
/**
 * @file simple_message_server_logic.c
 * Business logic for bulletin board programming exercise.
 *
 * In the course "Verteilte Computer Systeme" the students shall
 * implement a bulletin board. It shall consist of a spawning TCP/IP
 * server which executes the business logic provided by the lector,
 * and a suitable TCP/IP client.
 *
 * This program is the implementation of the business logic.
 *
 * @author franz.hollerer@technikum-wien.at
 * @author thomas.galla@technikum-wien.at
 * @date 2016/12/29
 */
/*
 * $Id:$
 */

/*
 * -------------------------------------------------------------- includes --
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/times.h>
#include <ctype.h>
#include <sys/file.h>

/*
 * include embedded PNGs and HTML pages.
 */
#include "vcs_tcpip_bulletin_board.php.h"
#include "vcs_tcpip_bulletin_board_response_ok.thtml.h"
#include "vcs_tcpip_bulletin_board_response_error.thtml.h"
#include "ok.png.h"
#include "error.png.h"
#include "content_entry_with_img.thtml.h"
#include "content_entry_without_img.thtml.h"

/*
 * --------------------------------------------------------------- defines --
 */

#define MAXERRORMSG 256
#define MAXTAGLEN 16
#define MAXMESSAGELEN 1024
#define MAXPATHLEN _POSIX_PATH_MAX
#define MAXFILESIZEDIGITS 20
#define MAXSTATUSDIGITS 10
#define MAXURLLEN 4096

#define BULLETIN_BOARD_MAIN_FILE "vcs_tcpip_bulletin_board.php"
#define BULLETIN_BOARD_CONTENT_FILE "bulletin_board_content.dat"

#define SMSL_E_OK      0
#define SMSL_E_FAILED -1  /* a general problem occured */
#define SMSL_E_INVAL   1  /* invalid input */
#define SMSL_E_OVERLOW 2  /* given input too long */

#define CHUNKSIZE 1024U
#define ADDITIONAL_BLANK_CHUNKS (1024U * 1024U)

/*
 * The program supports different tests which can be activated
 * via the environment variable SMSL_TESTCASE. Constants for
 * the testcases are given below, for a description see testcase_info[]
 * or use the --help option of the program.
 */
#define TESTCASE_NONE                0
#define TESTCASE_CHECK_ARGV          1
#define TESTCASE_CHECK_FD            2
#define TESTCASE_POSTPONE_COMPLETION 3
#define TESTCASE_PREMATURE_CLOSE     4
#define TESTCASE_SMALLER_LENGTH      5
#define TESTCASE_WRITE_DELAY         6
#define TESTCASE_HTML_ONLY_REPLY     7
#define TESTCASE_HUGE_FILE           8
#define TESTCASE_MAX TESTCASE_HUGE_FILE
#define TESTCASE_MIN TESTCASE_NONE

#define ERROR(format, ...) \
  error_at_line(EXIT_SUCCESS, errno, __FILE__, __LINE__, format, ## __VA_ARGS__)

#define ERROR_EXIT(format, ...)						\
  error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, format, ## __VA_ARGS__)

/*
 * -------------------------------------------------------------- typedefs --
 */

typedef struct
{
    int testcase;
    const char *description;
} testcase_info_t;

/*
 * --------------------------------------------------------------- globals --
 */

const testcase_info_t testcase_info[] =
{
    {
        TESTCASE_NONE,
        "no test"
    },
    {
        TESTCASE_CHECK_ARGV,
        "check if argv[0] is set correctly"
    },
    {
        TESTCASE_CHECK_FD,
        "check if unused file descriptors are closed"
    },
    {
        TESTCASE_POSTPONE_COMPLETION,
        "delayed close to check multiple connections"
    },
    {
        TESTCASE_PREMATURE_CLOSE,
        "simulate network problems (connection closed by peer)"
    },
    {
        TESTCASE_SMALLER_LENGTH,
        "give a smaller file length than transmitted"
    },
    {
        TESTCASE_WRITE_DELAY,
        "add a delay between writes"
    },
    {
        TESTCASE_HTML_ONLY_REPLY,
        "send the html part of the response only"
    },
    {
        TESTCASE_HUGE_FILE,
        "send a really huge html file for the response"
    }
};

/*
 * selected test case
 */
static int testcase = TESTCASE_NONE;

/*
 * list of allowed html tags for the client message
 */
static const char * const allowed_tags[] = 
{
    "<strong>", "</strong>",
    "<em>", "</em>",
    "<br/>"
};

/*
 * error message that will be sent to client
 */
static char errormsg[MAXERRORMSG];

/*
 * chunks of blank bytes
 */
static char chunk_of_blanks[CHUNKSIZE];

/*
 * global storage for content of argv[0]
 */
static const char *cmd = "<not yet set>";

/*
 * ------------------------------------------------------------- functions --
 */

/**
 * \brief Print a usage message
 *
 * Prints a usage message to \a stream and terminate program with \a exit_code
 *
 * \param fp stdio stream to write usage message to [IN]
 * \param exit_code exit code returned to the environment [IN]
 */
static void usage(
    FILE *fp,
    int exit_code
    )
{
    size_t i;

    (void) fprintf(
        fp,
        "usage: %s option\n"
        "options:\n"
        "\t-h, --help\n\n"
        "This program can perform several tests, which can be choosen\n"
        "with the environment variable SMSL_TESTCASE:\n",
	cmd
        );

    for (i = 0; i < (sizeof(testcase_info)/sizeof(*testcase_info)); i++)
    {
        (void) fprintf(
            fp, "\t%d - %s\n",
            testcase_info[i].testcase, testcase_info[i].description
            );
    }

    (void) fprintf(
        fp,
        "\nTests which must be executed manually:\n"
        "\t* rename simple_message_server_logic to check if a failure\n"
        "\t  of exec() is handled correctly.\n"
        );

    exit(exit_code);
}

/**
 * \brief Set seed for random number generator
 *
 * Sets the seed for the random number generator based on "the
 * number of clock ticks that have elapsed since an arbitrary
 * point in time in the past".
 */
static void set_seed_for_random_number_generation(
    void
    )
{
    struct tms tmsbuf;

    srandom(times(&tmsbuf));
}

/**
 * \brief Get a random number from the interval [1 .. \a max]
 *
 * Obtain an integral random number within the interval [1 .. \a max]
 *
 * \param max upper bound for the random number to be generated
 * \return the generated random number
 */
static int get_random_max(
    int max
    )
{
    return ((random() % max) + 1);
}

/**
 * \brief Get the number of the testcase to be executed
 *
 * Retrieve the number of the testcase to be executed from the environment variable
 * SMSL_TESTCASE.
 *
 * \return the number of the testcase to be executed
 * \retval 0 no testcase shall be executed (possibly because SMSL_TESTCASE was not set)
 * \retval -1 conversion of SMSL_TESTCASE to a long failed
 * \retval otherwise the number of the testcase to be executed
 */
static int get_testcase(
    void
    )
{
    const char *s;
    char *eptr;
    long testcase_number;

    if ((s = getenv("SMSL_TESTCASE")) == NULL)
    {
        return 0;
    }

    testcase_number = strtol(s, &eptr, 10);

    if (
	(eptr == NULL) ||
	(*eptr != '\0') ||
	(testcase_number < TESTCASE_MIN) ||
	(testcase_number > TESTCASE_MAX)
	)
    {
	return -1;
    }
	
    return testcase_number;
}

/**
 * \brief Assert that all non-standard file descriptors are closed
 *
 * Assert that all files expect stdin, stdout and stderr are
 * closed (especially the connected socket form the forking
 * server).
 *
 * note: FD_SETSIZE (see select()) is used as maximum number of
 * open file descriptors even the system may support an higher number
 * of open files.
 */
static void assert_files_closed(
    void
    )
{
    int i;
    struct stat statbuf;

    for (i = STDERR_FILENO + 1; i < FD_SETSIZE; i++)
    {
        if (fstat(i, &statbuf) == 0)
	{
	    ERROR_EXIT(
		"%s: Some files above STDERR_FILENO "
                "are still open.",
		__func__
		);
        }
    }
}

/**
 * \brief Get the URL for the bulletin board web page and the home directory
 *
 * Retrieve the URL for the bulletin board web page and the home directory for
 * the calling user.
 *
 * \param url pointer to buffer to be filled with the URL [IN]
 * \param url_len size of the buffer pointed to by \a url [IN]
 * \param homedir pointer to buffer to be filled with the path to the user's home dir [IN]
 * \param homedir_len size of the buffer pointed to by \a homedir [IN]
 */
static void get_url_and_homedir(
    char *url,
    size_t url_len,
    char *homedir,
    size_t homedir_len
    )
{
    struct passwd *pw;
    char host[HOST_NAME_MAX];
    int cnt;

    errno = 0;
    if ((pw = getpwuid(getuid())) == NULL)
    {
        ERROR_EXIT(
	    "%s: getpwuid() failed.",
	    __func__
	    );
    }

    if (gethostname(host, sizeof(host) - 1) == -1)
    {
        ERROR_EXIT(
	    "%s: gethostname() failed.",
	    __func__
	    );
    }

    host[sizeof(host) - 1] = 0;

    cnt = snprintf(
	url,
	url_len,
	"http://%s/~%s/%s",
	host,
	pw->pw_name,
	BULLETIN_BOARD_MAIN_FILE
	);

    if (cnt < 0)
    {
        ERROR_EXIT(
	    "%s: snprintf() failed.",
	    __func__
	    );
    }
    else if ((size_t) cnt >= url_len)
    {
        ERROR_EXIT(
	    "%s: snprintf() failed - buffer too small.",
	    __func__
	    );
    }

    if (strlen(pw->pw_dir) >= homedir_len)
    {
        ERROR_EXIT(
	    "%s: buffer for homedir too small.",
	    __func__
	    );
    }

    strncpy(homedir, pw->pw_dir, homedir_len - 1);
    homedir[homedir_len - 1] = 0; /* force string termination */
}

/**
 * \brief Turn off the nagle algorithm on stdout
 *
 * Check whether stdout is actually a socket and if so, turn off the
 * nagle algorithm on that socket.
 */
static void turn_off_nagle_algorithm(
    void
    )
{
    struct stat statbuf;
    int yes = 1;

    /*
     * Check if stdout is a socket. If not we can omit the
     * setsockopt() call to turn off the Nagle algorithm.
     */
    if (fstat(STDOUT_FILENO, &statbuf) == -1)
    {
        ERROR_EXIT(
	    "%s: fstat() failed.",
	    __func__
	    );
    }

    if (!S_ISSOCK(statbuf.st_mode))
    {
        return;  /* no socket */
    }

    if (setsockopt(
            STDOUT_FILENO, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)
            ) == -1)
    {
        ERROR_EXIT(
	    "%s: setsockopt() failed.",
	    __func__
	    );
    }
}

/**
 * \brief Create main bulletin board web page
 *
 * Create main bulletin board web page in users public_html directory
 * in case it does not exist.
 *
 * \param homedir zero-terminated string with the path of the user's home directory [IN]
 *
 * \retval 0 success
 * \retval -1 main page not created
 */
static int create_main_page(
    const char *homedir
    )
{
    char file[MAXPATHLEN];
    int cnt;
    int fd;

    cnt = snprintf(
	file,
	sizeof(file),
	"%s/public_html/%s",
	homedir,
	BULLETIN_BOARD_MAIN_FILE
	);

    if (cnt < 0)
    {
        ERROR_EXIT(
	    "%s: snprintf() failed.",
	    __func__
	    );
    }
    else if ((size_t) cnt >= sizeof(file))
    {
        ERROR_EXIT(
	    "%s: snprintf() failed - buffer too small.",
	    __func__
	    );
    }

    if ((fd = open(file, O_RDWR | O_CREAT | O_EXCL, 0644)) == -1)
    {
        if ((errno == EEXIST))
	{
            return 0;
                /* the file already exists. nothing to do. */
        }
        else {
            ERROR(
	        "%s: Creation of %s failed.",
		__func__,
		file
		);
            return -1;
        }
    }

    if (
	write(
            fd,
	    vcs_tcpip_bulletin_board_php,
            sizeof(vcs_tcpip_bulletin_board_php)
            ) != sizeof(vcs_tcpip_bulletin_board_php)
	)
    {
        ERROR(
	    "%s: Write to %s failed.",
	    __func__,
	    file
	    );
        (void) close(fd);
        (void) unlink(file);
        return -1;
    }

    if (close(fd) == -1)
    {
        ERROR(
	    "%s: Close of %s failed.",
	    __func__,
	    file
	    );
	return -1;
    }

    return 0;
}

/**
 * \brief Write provided buffer in chunks of random size
 *
 * Write the given buffer (\a buf) of size \a len in chunks of random
 * size to enforce short reads on the client side. In case
 * SMSL_TESTCASE is set to the numeric value of TESTCASE_WRITE_DELAY,
 * introduce 0.2 seconds delay between the writing of each chunk.
 *
 * \param buf pointer to the buffer to write [IN]
 * \param len length to the buffer pointed to by \a buf [IN]
 */
static void write_in_chunks(
    const void *buf,
    size_t len
    )
{
    const char *b = (const char *) buf;
    ssize_t cnt;

    while (len)
    {
        if ((cnt = write(STDOUT_FILENO, b, get_random_max(len))) == -1)
        {
	    /* error response will fail too, thus just exit. */
            ERROR_EXIT(
   	        "%s: write() failed.",
		__func__
		);
        }

        len -= cnt;
        b += cnt;

        if (testcase == TESTCASE_WRITE_DELAY)
	{
            usleep(200000);
	}
    }
}

/**
 * \brief Write execution status of business logic
 *
 * Write the provided execution status (\a status) of the business
 * logic to stdout using \a write_in_chunks().
 *
 * \param status execution status of the business logic [IN]
 */
static void write_status(int status)
{
    static const char * const fmt_status = "status=%d\n";
    char s[MAXSTATUSDIGITS + sizeof(fmt_status)];
    int cnt;

    cnt = snprintf(s, sizeof(s), fmt_status, status);

    if (cnt < 0)
    {
        ERROR_EXIT(
	    "%s: snprintf() failed.",
	    __func__
	    );
    }
    else if ((size_t) cnt >= sizeof(s))
    {
        ERROR_EXIT(
	    "%s: snprintf() failed - buffer too small.",
	    __func__
	    );
    }

    write_in_chunks(s, strlen(s));
}

/**
 * \brief Write file header and file content
 *
 * Write the file header for the file \a filename of length \a len to
 * stdout using \a write_in_chunks(). The contents of the file are
 * provided in the buffer pointed to by \a buf.
 *
 * \param filename name of the file to be written [IN]
 * \param buf pointer to the buffer containing the file contents [IN]
 * \param len length of the file (and thus size of the buffer) [IN]
 * \param additional_blank_chunks additional chunks of blanks to be
 * added at then end of the HTML file [IN]
 */
static void download_file(
    const char *filename, const void *buf, size_t len,
    unsigned additional_blank_chunks
    )
{
    static const char * const fmt_file = "file=%s\nlen=%zu\n";
    char s[MAXFILESIZEDIGITS + MAXPATHLEN + sizeof(fmt_file)];
    int cnt;

    /*
     * create header containing keywords "file" and "len".
     */
    cnt = snprintf(
	s,
	sizeof(s),
	fmt_file,
	filename,
	(testcase == TESTCASE_SMALLER_LENGTH) ? (len - len/3) :
	len
	);

    if (cnt < 0)
    {
        ERROR_EXIT(
	    "%s: snprintf() failed.",
	    __func__
	    );
    }
    else if ((size_t) cnt >= sizeof(s))
    {
        ERROR_EXIT(
	    "%s: snprintf() failed - buffer too small.",
	    __func__
	    );
    }

    /*
     * write header with keywords "file" and "len"
     */
    write_in_chunks(s, strlen(s));

    /*
     * in case we want to simulate a "connection closed by peer"
     * we only sent half of the ok.png image. Afterwards the
     * program exits and closes the connection automatically.
     */
    if ((testcase == TESTCASE_PREMATURE_CLOSE) && (buf == ok_png))
    {
        len /= 2;
    }

    if (buf != NULL)
    {
	/*
	 * write content of file
	 */
	write_in_chunks(buf, len);
    }

    /*
     * in case we want to simulate a really *huge* file
     * we append a few spaces ....
     */
    if (testcase == TESTCASE_HUGE_FILE)
    {
	for (unsigned i = 0; i < additional_blank_chunks; ++i)
	{
	    (void) fprintf(stderr, "Writing chunk %u of %u ...\n", i, additional_blank_chunks); 
	    if (write(STDOUT_FILENO, chunk_of_blanks, sizeof(chunk_of_blanks)) == -1)
	    {
    	        /* error response will fail too, thus just exit. */
	        ERROR_EXIT(
		    "%s: write() failed.",
		    __func__
		    );
	    }
	}
    }
}

/**
 * \brief Write an error response
 *
 * Write an error response as answer to the client's request to
 * stdout using \a write_status() and \a download_file().
 *
 * \param status execution status of the business logic [IN]
 */
static void error_response(
    int status
    )
{
    int cnt;
    size_t len;

    len = sizeof(vcs_tcpip_bulletin_board_response_error_thtml)
            + strlen(errormsg);

    char html_response[len];

    /*
     * encode error message
     */
    cnt = snprintf(
        html_response,
	len,
        (const char *) vcs_tcpip_bulletin_board_response_error_thtml,
        errormsg
        );

    if (cnt < 0)
    {
        ERROR_EXIT(
	    "%s: snprintf() failed.",
	    __func__
	    );
    }

    /*
     * signal failure to client
     */
    write_status(status);

    /*
     * write html file
     */
    download_file(
        "vcs_tcpip_bulletin_board_response.html",
        html_response,
        strlen(html_response),
	0
        );

    errormsg[0] = 0; /* clear error message */

    if (testcase == TESTCASE_HTML_ONLY_REPLY)
    {
        return;  /* omit image */
    }

    if (testcase == TESTCASE_HUGE_FILE)
    {
	download_file(
	    "/dev/null",
	    NULL,
	    ADDITIONAL_BLANK_CHUNKS * sizeof(chunk_of_blanks),
	    ADDITIONAL_BLANK_CHUNKS
	    );
    }

    /*
     * write error.png
     */
    download_file("error.png", error_png, sizeof(error_png), 0);
}

/**
 * \brief Write an OK response
 *
 * Write an OK response as answer to the client's request to
 * stdout using \a write_status() and \a download_file().
 *
 * \param url zero-terminated string containing the URL to the bulletin board web page [IN]
 */
static void ok_response(
    const char *url
    )
{
    int cnt;
    size_t len;

    /*
     * in the prepared html response we have %s twice as placeholder
     * for the url.
     */
    len = sizeof(vcs_tcpip_bulletin_board_response_ok_thtml)
            + 2 * strlen(url);

    char html_response[len];

    cnt = snprintf(
            html_response,
	    len,
            (const char *) vcs_tcpip_bulletin_board_response_ok_thtml,
            url,
	    url
            );

    if (cnt < 0)
    {
        ERROR_EXIT(
	    "%s: snprintf() failed.",
	    __func__
	    );
    }

    /*
     * signal success to client
     */
    write_status(SMSL_E_OK);

    /*
     * write html file
     */
    download_file(
        "vcs_tcpip_bulletin_board_response.html",
        html_response,
	strlen(html_response),
	0
        );

    if (testcase == TESTCASE_HTML_ONLY_REPLY)
    {
        return;  /* omit image */
    }

    if (testcase == TESTCASE_HUGE_FILE)
    {
	download_file(
	    "/dev/null",
	    NULL,
	    ADDITIONAL_BLANK_CHUNKS * sizeof(chunk_of_blanks),
	    ADDITIONAL_BLANK_CHUNKS
	    );
    }
    /*
     * write ok.png
     */
    download_file("ok.png", ok_png, sizeof(ok_png), 0);
}

/**
 * \brief Search next tag in the given string
 *
 * Search the next HTML tag in the zero-terminated string \a s and return
 * a pointer to the start and the end of the tag via \a beginp and \a endp.
 * In case that no tag is found beginp and endp are not changed.
 *
 * \param s zero-terminated string which shall be searched trough [IN]
 * \param beginp pointer to the begin of the tag [OUT]
 * \param endp pointer to the end of the tag [OUT]
 *
 * \return Information whether or not a tag was found.
 * \retval 0 a tag was found
 * \retval -1 no tag found
 */
static int search_next_tag(
    const char *s,
    const char **beginp,
    const char **endp
    )
{
    const char *beg = NULL;

    /*
     * a tag starts with a '<' and ends with '>'.
     * first we have to find the start of a tag.
     */
    for (; *s != 0; s++)
    {
        if (*s == '<')
	{
            beg = s; /* start of a tag found */
            break;
        }
    }

    /*
     * now search the end of the tag.
     */
    for (; *s != 0; s++)
    {
        if (*s == '>')
	{
            *beginp = beg; /* end of a tag found */
            *endp = s;
            return 0;
        }
        
    }

    return -1;
}

/**
 * \brief Validate client input
 *
 * Checks if the data received from the client consists of
 * printable characters and does not contain unsupported HTML tags.
 *
 * \param buf pointer to the buffer conatining the client's input data [IN]
 * \param len size fo the buffer pointed to by \a buf.
 * 
 * \return Information whether or not the input is accepted.
 * \retval 0 input is valid
 * \retval 1 input rejected
 */
static int validate_input(
    const char *buf,
    size_t len
    )
{
    size_t i;
    const char *cp, *tag_begin, *tag_end;
    char tag[MAXTAGLEN];
    int tag_valid;

    for (i = 0; i < len; i++)
    {
        if (!isprint(buf[i]) && !isspace(buf[i]))
        {
            (void) snprintf(
                errormsg,
		sizeof(errormsg),
                "Request contains non printable character "
                "0x%.2X at position %zu\n",
                buf[i], i
                );
            return -1; /* input contains a non-printable character. */
        }
    }

    /*
     * check if input is null-terminated.
     * this case should never occur...
     */
    if (buf[len] != 0)
    {
        ERROR_EXIT(
	    "%s: fatal error. - Input is not null-terminated.",
	    __func__
	    );
    }

    /*
     * check for allowed html tags
     */
    for (
	cp = buf;
	search_next_tag(cp, &tag_begin, &tag_end) == 0;
         cp = tag_end + 1
	)
    {
        const size_t tag_len = tag_end - tag_begin + 1;

        tag_valid = 0;
        for (
	    i = 0;
	    i < (sizeof(allowed_tags) / sizeof(*allowed_tags));
	    i++
	    )
        {
            if (
		strncmp(
                    allowed_tags[i],
		    tag_begin,
		    tag_len
                    ) == 0
		)
            {
                tag_valid = 1;
                break;
            }
        }
        if (tag_len >= MAXTAGLEN - 1)
        {
            (void) strncpy(tag, tag_begin, MAXTAGLEN - 5);
            tag[MAXTAGLEN - 5] = '.';
            tag[MAXTAGLEN - 4] = '.';
            tag[MAXTAGLEN - 3] = '.';
            tag[MAXTAGLEN - 2] = '>';
            tag[MAXTAGLEN - 1] = '\0';
        }
        else
        {
            (void) strncpy(tag, tag_begin, tag_len);
            tag[tag_len] = '\0';
        }

        if (!tag_valid)
        {
            (void) snprintf(
                errormsg,
		sizeof(errormsg),
                "Contains unsupported HTML tag <pre>%s</pre>\n",
                tag
                );
            return -1;  /* found tag not supported / allowed */
        }
    }

    return 0;
}

/**
 * \brief Replace first '\n' with '\0'
 *
 * Search for the first '\n' and replace it with '\0'.
 *
 * \param s zero-terminated string containing at least one '\n'
 *
 * \return Information on whether or not the replacement was done
 * \retval 0 success - the first '\n' was replaced with '\0' 
 * \retval -1 no newline is found and thus no newline replaced
 */
static int terminate_string_at_newline(
    char *s
    )
{
    for (; *s; s++)
    {
        if (*s == '\n')
        {
            *s = 0;
            return 0;
        }
    }
    return -1;
}

/**
 * \brief Parse and split client input
 *
 * Parse and split client input into user, message and the optional
 * image. Note: this function modifies the passed string by replacing
 * some '\n' with null-terminators to split the passed string into the
 * three parts mentioned above. The pointer passed back point into the
 * buffer if the associated keyword is found or set to NULL otherwise.
 *
 * \param buf zero-terminated string containing the data from
 *            the client [IN]
 * \param userp set to the begin of the user name or
 *              to NULL if the keyword "user=" is not found. [OUT]
 * \param imgp set to the begin of the image url or
 *             to NULL if the keyword "img=" is not found. [OUT]
 * \param msgp set to the begin of the message or
 *             to NULL if this part of the input is empty. [OUT]
 *
 * \return Information on whether or not the client input was valid
 * \retval 0 the input is valid and contains at least a user name
 *           and a message
 * \retval -1 input is invalid
 */
static int split_input(
    char *buf,
    const char **userp,
    const char **imgp,
    const char **msgp
    )
{
    char *user, *img, *msg;
    const char *kw_user = "user=";
    const char *kw_img = "img=";

    /*
     * the input shall have the following format:
     *
     * user=<username>
     * img=<URL>
     * <message>
     * :
     * :
     * EOF
     *
     * img is optional and must follow user if present.
     */

    if (strncmp(buf, kw_user, strlen(kw_user)) != 0)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Keyword <code>user</code> is missing in first line of request\n"
            );
        return -1;
    }

    user = buf + strlen(kw_user);
    if (terminate_string_at_newline(user))
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Line with keyword <code>user</code> is not newline terminated\n"
            );
        return -1;  /* no newline replaced */
    }

    img = user + strlen(user) + 1;
    if (strncmp(img, kw_img, strlen(kw_img)) != 0)
    {
        img = NULL;
    }
    else
    {
        img += strlen(kw_img);

        if (terminate_string_at_newline(img))
        {
            (void) snprintf(
                errormsg,
		sizeof(errormsg),
                "Line with keyword <code>img</code> is not newline terminated\n"
                );
            return -1;  /* no newline replaced */
        }
    }

    /*
     * now find begin of message
     */
    msg = (img != NULL) ? img : user;
    msg += strlen(msg) + 1;

    /*
     * perform some checks
     */
    if (strlen(user) == 0)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Keyword <code>user</code> ist present but username is missing\n"
            );
        return -1;
    }

    if (strlen(msg) == 0)
    {
        (void) snprintf(
	    errormsg,
	    sizeof(errormsg),
	    "Message is empty\n");
        return -1;
    }

    if ((img != NULL) && (strlen(img) == 0))
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Keyword <code>img</code> ist present, but URL to image is missing\n");
        return -1;
    }

    *userp = user;
    *imgp = img;
    *msgp = msg;

    return 0;
}

/**
 * \brief Write client message into bulletin board content file
 *
 * Write the client message \a msg, sent by \a user together with the
 * URL to the optional image (\a img) into to the bulletin board content file
 * located in the public_html directory in the user's \a homedir.
 *
 * \param homedir zero-terminated string containing the path to the user's home directory [IN]
 * \param user zero-terminated string containing the user name [IN]
 * \param img zero-terminated string containing the URL of the image to be used [IN]
 * \param msg zero-terminated string containing the message to be added [IN]
 *
 * \return Information on whether or not the writing was successful
 * \retval 0 success
 * \retval -1 failed
 */
static int post_message(
    const char *homedir,
    const char *user,
    const char *img,
    const char *msg
    )
{
    char file[MAXPATHLEN];
    FILE *fp;
    int cnt;
    char content_entry[sizeof(content_entry_with_img_thtml)
                        + MAXMESSAGELEN];
    size_t content_wr_count;

    /*
     * in the content entry template we have some %s to fill in
     * user, image and message.
     */
    if (img != NULL)
    {
        cnt = snprintf(
	    content_entry,
	    sizeof(content_entry),
	    (const char *) content_entry_with_img_thtml,
	    img,
	    user,
	    user,
	    msg
            );
    }
    else
    {
        cnt = snprintf(
	    content_entry,
	    sizeof(content_entry),
	    (const char *) content_entry_without_img_thtml,
	    user,
	    msg
            );
    }

    if (cnt < 0)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "%s: snprintf() failed - <pre>%s</pre>\n",
	    cmd,
            strerror(errno)
            );
        return -1;
    }
    else if ((size_t) cnt >= sizeof(content_entry))
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Message too long - only a maximum of %d bytes (incl. username) "
            "are supported\n",
            MAXMESSAGELEN
            );
        return -1;
    }

    content_wr_count = (size_t) cnt;

    cnt = snprintf(
            file,
	    sizeof(file),
	    "%s/public_html/%s",
	    homedir,
            BULLETIN_BOARD_CONTENT_FILE
            );

    if (cnt < 0)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "%s: snprintf() failed - <pre>%s</pre>\n",
	    cmd,
            strerror(errno)
            );
        return -1;
    }
    else if ((size_t) cnt >= sizeof(file))
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
	    "Filename for bulleting board file (incl. path) is too long - "
	    "only a maximum of %d bytes are supported\n",
            MAXPATHLEN
            );
        return -1;
    }

    if ((fp = fopen(file, "a+")) == NULL)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Unable to open file <code>%s</code> - <pre>%s</pre>\n",
	    file,
	    strerror(errno)
            );
        return -1;
    }

    if (flock(fileno(fp), LOCK_EX) == -1)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Upable to lock file <code>%s</code> - <pre>%s</pre>\n",
	    file,
	    strerror(errno)
            );
        (void) fclose(fp);
        return -1;
    }

    if (
	fwrite(
	    content_entry,
	    sizeof(char),
	    content_wr_count,
	    fp
	    ) != content_wr_count)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Unbale to write to file <code>%s</code> - <pre>%s</pre>\n",
	    file,
	    strerror(errno)
            );
        (void) fclose(fp);
        return -1;
    }

    if (fclose(fp) == EOF)  /* unlock performed automatically with close */
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Unable to close (flush) file <code>%s</code> - <pre>%s</pre>\n",
	    file,
	    strerror(errno)
            );
        return -1;
    }

    return 0;
}

/**
 * \brief Read, validate and store the client request message.
 *
 * Read the client's input request from stdin, validate this input by
 * calling \a validate_input() and \a split_input(), and store the
 * client message into bulletin board content file by calling \a
 * post_message().
 *
 * \param homedir zero-terminated string containing the path to the user's
          home directory [IN]
 * \param mainpagecreated value indicating if main page has been
 *        created successfully (0 in that case; -1 upon failue) [OUT]
 *
 * \return Information on whether or not the processing was successful
 * \retval SMSL_E_OK success
 * \retval SMSL_E_FAILED a general error occured
 * \retval SMSL_E_INVAL input invalid / not accepted
 * \retval SMSL_E_OVERFLOW given input exceeds internal buffer size
 */
static int process_message(
    const char *homedir,
    int mainpagecreated
    )
{
    char buf[MAXMESSAGELEN];
    int cnt;
    const char *user, *img, *msg;

    memset(buf, 0, sizeof(buf));
    if ((cnt = fread(buf, sizeof(char), sizeof(buf) - 1, stdin)) <= 0)
    {
        return SMSL_E_INVAL;  /* nothing read at all */
    }

    /*
     * if we are at EOF the user input is finished. otherwise
     * there is more input pending which would overflow our internal
     * buffer.
     */
    if (!feof(stdin))
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
	    "Server input buffer overflow - "
	    "processing of input messages is limited to %u bytes\n",
	    MAXMESSAGELEN
            );
        return SMSL_E_OVERLOW;
    }

    /*
     * if we could not create the main page, we simply discard all the
     * input from the client and report the error to the client
     */
    if (mainpagecreated != 0)
    {
        (void) snprintf(
            errormsg,
	    sizeof(errormsg),
            "Server could not create main HTML page\n"
            );
	return SMSL_E_FAILED;
    }

    if (validate_input(buf, cnt) == -1)
    {
        return SMSL_E_INVAL;    /* input malformed */
    }

    if (split_input(buf, &user, &img, &msg) == -1)
    {
        return SMSL_E_INVAL;    /* input malformed */
    }

    if (post_message(homedir, user, img, msg))
    {
        return SMSL_E_INVAL;    /* write to content file failed */
    }

    return SMSL_E_OK;
}

/**
 *
 * \brief Main entry point of program.
 *
 * This function is the main entry point of the program.
 *
 * \param argc - number of command line arguments.
 * \param argv - array of command line arguments.
 *
 * \return Information about succes or failure in the execution
 * \retval EXIT_FAILURE failed execution.
 * \retval EXIT_SUCCESS successful execution
 */
int main(
    int argc,
    char **argv
    )
{
    int c, status;
    int mainpagecreated = -1;
    char url[MAXURLLEN], homedir[MAXPATHLEN];
    struct option long_options[] =
    {
        {"help", 0, NULL, 'h'},
        {0, 0, 0, 0}
    };

    cmd = argv[0];

    while (
	(c = getopt_long(
	    argc,
	    argv,
	    "h",
	    long_options,
	    NULL)
	    ) != -1
	)
    {
        switch (c)
        {
            case 'h':
                usage(stdout, EXIT_SUCCESS);
                break;
            case '?':
            default:
                usage(stderr, EXIT_FAILURE);
                break;
        }
    }

    if ((optind < argc))
    {
        usage(stderr, EXIT_FAILURE);
    }

    testcase = get_testcase();

    if (testcase == -1)
    {
	usage(stderr, EXIT_FAILURE);
    }

    memset(chunk_of_blanks, ' ', sizeof(chunk_of_blanks));

    if ((testcase == TESTCASE_CHECK_ARGV))
    {
        if ((argc == 0))
	{
            (void) fprintf(stderr, "Sorry, argv[0] not set\n");
            exit(EXIT_FAILURE);
        }
    }
    
    if (testcase == TESTCASE_CHECK_FD)
    {
        assert_files_closed();
    }

    set_seed_for_random_number_generation();
    turn_off_nagle_algorithm();

    get_url_and_homedir(url, sizeof(url), homedir, sizeof(homedir));

    mainpagecreated = create_main_page(homedir);

    if ((status = process_message(homedir, mainpagecreated)) == SMSL_E_OK)
    {
        ok_response(url);
    }
    else
    {
        error_response(status);
        return EXIT_FAILURE;
    }

    if (testcase == TESTCASE_POSTPONE_COMPLETION)
    {
        pause();
    }

    exit(EXIT_SUCCESS);
}

/*
 * =================================================================== eof ==
 */
