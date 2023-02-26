#define _POSIX_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int countlines(char *cmd, char *buffer)
{
    int lines;

    FILE *fp;
    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to run command: %s\n", cmd);
        exit(1);
    }

    /* Read the output a line at a time - output it. */
    lines = 0;
    while (fgets(buffer, 256, fp) != NULL)
        lines++;

    /* close */
    pclose(fp);

    if (lines == 1)  // Returns true if generation is captured or no changes happened.
        return 1;

    return 0;
}

int has_changed(char *last, char *orig, char *pool)
{
	/* Check last snapshot generation. If original subvolume and last
	 * snapshot is greater than one line, means that original is different
	 * from last subvolume. */

	char cmd[256];
	char buffer[1024];

	strcpy(cmd, "btrfs subvolume show '");
	strcat(cmd, pool);
	strcat(cmd, "/");
	strcat(cmd, last);
	strcat(cmd, "' ");
    strcat(cmd, "| grep Generation | cut -d: -f2| tr -d '[:space:]'");

    //printf("%s\n", cmd);

    if (countlines(cmd, buffer))  // Generation number is in buffer.
    {

        strcpy(cmd, "btrfs subvolume find-new '");
        strcat(cmd, orig);
        strcat(cmd, "' ");
        strcat(cmd, buffer);

        //printf("%s\n", cmd);

        if (!countlines(cmd, buffer))  // If more than one line in cmd output
            return 1;  // Has changed
    }

	return 0;  // Not has changed
}

int main(int argc, char **argv)
{
    char last[80];
    char orig[80];
    char pool[80];

    strcpy(last, argv[1]);
    strcpy(orig, argv[2]);
    strcpy(pool, argv[3]);

    int ret = has_changed(last, orig, pool);
    //if (has_changed(last, orig, pool))
    if (ret)
        printf("Ha cambiado: %d\n", ret);
    else
        printf("No ha cambiado: %d\n", ret);

    return 0;
}
