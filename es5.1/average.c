#define _POSIX_C_SOURCE	200809L
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "utils.h"

#define MAXCHAR 4096

/* Function that uses strtol to read the natural number (i.e., non-negative 
integer) at the beginning of a string. */

int get_natural(const char *str)
{
        long ret;
        char *endptr;	

        ret = strtol(str, &endptr, 10);

        if (ret == 0 && errno == EINVAL) {
                // no conversion performed
                return -1;
        }

        if (errno == ERANGE) {
                if (ret == LONG_MIN) {
                        // underflow
                        return -2;
                } else { // ret == LONG_MAX
                        // overflow
                        return -3;
                }
        }

        if (ret < 0 || ret > INT_MAX) {
                return -4;
        }

        return (int)ret;
}

/* Function that calculates the average and that for each read with fgets,
 * writes what it has read on the file descriptor fd_out.
 * As a final operation, it also writes the average on fd_out. */ 
void output_with_average(int fd_in, int fd_out)
{
        FILE *stream;
        char str[MAXCHAR], avrg_str[MAXCHAR]; 
        int tot = 0, ret;
        float avrg;
	int lines = 0;

        /* Encapsulate the file descriptor in a FILE* so I can use fgets */
        stream = fdopen(fd_in, "r");
        if (stream == NULL) {
                fprintf(stderr, "Could not open file %d", fd_in);
                exit(EXIT_FAILURE);
        }

        /* fgets reads until it encounters a \n or \0.
         * In this case, I use it to read line by line
         * the result of the exec  */
        while (fgets(str, MAXCHAR, stream) != NULL) {
                /* Write the line read with fgets to fd_out
                 * which in my case will be the socket descriptor ns */
                if (write_all(fd_out, str, strlen(str)) < 0) {
                        perror("write exec"); 
                        exit(EXIT_FAILURE);
                }

                /* Extrapolate the number at the beginning of the line with get_natural. I assume that
                 * the separator is a space, otherwise I would have to manipulate
                 * the string to transform, for example, a ',' separator into
                 * a ' ' separator. */
                if ((ret = get_natural(str)) >= 0) {
                        tot += ret;
                }

                /* Update count of lines read */
                lines += 1;
        }

        /* Close file */
        //fclose(stream);

        if (lines > 0) {
                printf("Calculating average from %d lines\n", lines);
                /* Calculate average and format it with snprintf */
                avrg = (float)tot / (float)lines;

                /* Prepare summary message */
                snprintf(avrg_str, sizeof(avrg_str), "\nThe average snow height on the requested facilities is: %f cm\n", avrg);

                /* Write the average to fd_out, which is the socket */
                if (write_all(fd_out, avrg_str, strlen(avrg_str)) < 0){
                        perror("write final average");
                        exit(EXIT_FAILURE);
                }
        }
}

