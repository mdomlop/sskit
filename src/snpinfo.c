#include <stdio.h>
#include <time.h>
#include <btrfsutil.h>
#include <getopt.h>
#include <stdlib.h>  // atoi() abort()
#include <ctype.h>  /* isdigit() isprint() */
#include <linux/limits.h>  /* for PATH_MAX */
#include <dirent.h>  // opendir()
#include <string.h> /* strlen() */

#define PROGRAM      "SnapInfo"
#define EXECUTABLE   "snpinfo"
#define DESCRIPTION  "Show info from snapshots in a Btrfs filesystem."
#define PKGNAME      "makesnap"
#define VERSION      "0.1a"
#define URL          "https://github.com/mdomlop/makesnap"
#define LICENSE      "GPLv3+"
#define AUTHOR       "Manuel Domínguez López"
#define NICK         "mdomlop"
#define MAIL         "zqbzybc@tznvy.pbz"



void version (void)
{
	printf ("%s Version: %s\n", PROGRAM, VERSION);
}


void help (int error)
{
	char text[] = "\nUsage:\n\n"

	"\t-p dir			Set the output directory.\n\n"
	"\t-q quota			Set the quota.\n\n"

	"\t-h				Show this help and exit.\n"
	"\t-v				Show program version and exit.\n";

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
    else if (err == BTRFS_UTIL_ERROR_NOT_BTRFS || err == BTRFS_UTIL_ERROR_NOT_SUBVOLUME)
        printf("Is NOT a btrfs subvolume: %s\n", subvol);
    else
        printf("Is NOT a subvolume: %s\n", subvol);

    return 0;
}

int main(int argc, char **argv)
{
	int hflag, vflag;
	hflag = vflag = 0;

	char *subvol = NULL;  // pool path

    struct btrfs_util_subvolume_info info;


	int c;

	while ((c = getopt (argc, argv, "s:hv")) != -1)
		switch (c)
		{
			case 's':  // Suvolume
				subvol = optarg;
				break;
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

	if (argc - optind > 0)  // Max non-option argumets are 0
	{
		fprintf (stderr, "Sorry. Too much arguments.\n");
		return 1;
	}

	if(hflag)
	{
		help(0);
	}
	else if (vflag)
	{
		version();
	}

	if (subvol)
    {
        if (check_is_subvol(subvol))
        {
            btrfs_util_subvolume_info(subvol, 0, &info);
            printf("Generation: %ld, CTime: %ld\n", info.generation, info.ctime.tv_sec);
        }
        else
            return 1;
    }

    return 0;
}
