#include <stdio.h>
#include <time.h>
#include <btrfsutil.h>
#include <getopt.h>
#include <stdlib.h>  // atoi() abort()
#include <ctype.h>  /* isdigit() isprint() */
#include <linux/limits.h>  /* for PATH_MAX */
#include <dirent.h>  // opendir()
#include <string.h> /* strlen() */

#define PROGRAM      "SnapCT"
#define EXECUTABLE   "ssct"
#define DESCRIPTION  "Show creation time and last change date from snapshots in a Btrfs filesystem."
#define PKGNAME      "sstools"
#define VERSION      "0.3b"
#define URL          "https://github.com/mdomlop/sstools"
#define LICENSE      "GPLv3+"
#define AUTHOR       "Manuel Domínguez López"
#define NICK         "mdomlop"
#define MAIL         "zqbzybc@tznvy.pbz"

#define TIMESTAMP    "%F %T"

void version (void)
{
	printf ("%s Version: %s -- %s\n", PROGRAM, VERSION, DESCRIPTION);
}


void help (int error)
{
	char text[] = "\nUsage:\n\t"
	EXECUTABLE
	" [-h] [-v] path\n"
	"\nOptions:\n"
	"\t-h	Show this help and exit.\n"
	"\t-v	Show program version and exit.\n";

	if (error)
		fprintf (stderr, "%s\n", text);
	else
	{
		version();
		printf ("%s\n", text);
	}
}


int check_is_subvol(char *subvol)
{
    enum btrfs_util_error err;
    err = btrfs_util_is_subvolume(subvol);

    if (!err)
        return 1;
    else if (err == BTRFS_UTIL_ERROR_NOT_BTRFS)
        printf("Is not on a Btrfs filesystem: %s\n", subvol);
    else if (err == BTRFS_UTIL_ERROR_NOT_SUBVOLUME)
        printf("Is not a subvolume: %s\n", subvol);
	else
		printf("Does not exist: %s\n", subvol);

    return 0;
}

int main(int argc, char **argv)
{
	int hflag, vflag;
	hflag = vflag = 0;

	char *subvol = NULL;  // pool path

    struct btrfs_util_subvolume_info info;


	int c;

	while ((c = getopt (argc, argv, "hv")) != -1)
		switch (c)
		{
			case 'h':  // Show help
				hflag = 1;
				break;
			case 'v':  // Show program version
				vflag = 1;
				break;
			case '?':
				if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n"
					"See help.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n",
							optopt);
				return 1;
			default:
				abort ();
		}

	if (argc - optind > 1)  // Max non-option argumets are 1
	{
		fprintf (stderr, "Sorry. Too much arguments.\n");
		return 1;
	}

	if(hflag)
	{
		help(0);
		return 0;
	}
	else if (vflag)
	{
		version();
		return 0;
	}

	subvol = argv[optind];
	/*for (int index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);*/

	if (subvol)
    {
        if (check_is_subvol(subvol))
        {
            btrfs_util_subvolume_info(subvol, 0, &info);

			struct tm *mytm;
			char otime[64], ctime[64];
			time_t myotime, myctime;
			myotime = info.otime.tv_sec;
			myctime = info.ctime.tv_sec;

			mytm = localtime(&myotime);
			strftime(otime, sizeof otime, TIMESTAMP, mytm);

			mytm = localtime(&myctime);
			strftime(ctime, sizeof ctime, TIMESTAMP, mytm);

            printf("Created: %s\nLast changed: %s\n", otime, ctime);
        }
        else
            return 1;
    }

    return 0;
}
