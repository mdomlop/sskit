#define PROGRAM      "SnapClean"
#define EXECUTABLE   "sscl"
#define DESCRIPTION  "Clean snapshots in a Btrfs filesystem."

#include <stdio.h>
#include <time.h>
#include <btrfsutil.h>
#include <getopt.h>
#include <stdlib.h>  // atoi() abort()
#include <ctype.h>  /* isdigit() isprint() */
#include <unistd.h> /* uid, euid */
#include <linux/limits.h>  /* for PATH_MAX */
#include <dirent.h>  // opendir()
#include <string.h> /* strlen() */
#include "sstools.h"

/* Btrfs has not this limit, but I think this value is very safe. */
#define SNAPLISTSIZE 64000

char snaplist[SNAPLISTSIZE][PATH_MAX];  // List of snapshots in pool_path
int snapls_c = 0;  // Number of elements in snaplist
int diff = 0;


void help (int error)
{
	char text[] = "\nUsage:\n\t"
	EXECUTABLE
	" [-h] [-v] -p dir -q quota\n"
	"\nOptions:\n"
	"\t-p dir       Set the output directory.\n"
	"\t-q quota     Set the quota.\n\n"

	"\t-h           Show this help and exit.\n"
	"\t-v           Show program version and exit.\n";

	if (error)
		fprintf (stderr, "%s\n", text);
	else
	{
		version();
		printf ("%s\n%s\n", DESCRIPTION, text);
	}
}

int get_snapshots(char *pool_path)
{
	//printf("Reloading snapshots from %s\n", pool_path);

	DIR *dp;
	struct dirent *ep;

	char temp[4096];
	int n = 0;

	dp = opendir (pool_path);
	if (dp != NULL)
	{
		while ((ep = readdir (dp)))
		{
			if (ep->d_name[0] != '.')
			{
				strcpy(snaplist[n], ep->d_name);
				//printf("[%d] %s\n", n, ep->d_name);
				n++;
			}
		}
		(void) closedir (dp);
	}
	// No fail if store does not exists, because snapls_c is alredy 0
	/*
	else
	{
		fprintf(stderr, "Couldn't open the directory: %s\n", pool_path);
		perror ("");
	}
	*/

	snapls_c = n;

	/* Sort snapshots */
	for (int i = 0; i < snapls_c; i++)  // No fail when snapls_c is 0
	{
		for (int j = 0; j < snapls_c -1 -i; j++)
		{
			if (strcmp(snaplist[j], snaplist[j+1]) > 0)
			{
				strcpy(temp, snaplist[j]);
				strcpy(snaplist[j], snaplist[j+1]);
				strcpy(snaplist[j+1], temp);
			}
		}
	}

	return 0;
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

int check_root(void)
{
	int uid = getuid();
	int euid = geteuid();
	if (uid != 0 || uid != euid)
		return 1;
	return 0;
}

int main(int argc, char **argv)
{

	int hflag, vflag;
	hflag = vflag = 0;

	char snap_path[PATH_MAX];

	char *pvalue = NULL;  // pool path
	char *qvalue = NULL;  // quota

	int quota = 0;

	int c;

	while ((c = getopt (argc, argv, "p:q:hv")) != -1)
		switch (c)
		{
			case 'p':  // Set the output directory
				pvalue = optarg;
				break;
			case 'q':  // Set the quota
				quota = atoi(optarg);
				if (quota > 0)
					qvalue = optarg;
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

	if (check_root())
	{
		fprintf(stderr, "You must to be root for do this.\n");
		return 1;
	}

	if (pvalue && qvalue)
	{
		get_snapshots(pvalue);

		diff = snapls_c - quota;

		if (diff > 0)
		{
			for (int i = 0; i<diff; i++)
			{
				strcpy(snap_path, pvalue);
				strcat(snap_path, "/");
				strcat(snap_path, snaplist[i]);

				if (check_is_subvol(snap_path))
				{
					btrfs_util_delete_subvolume(snap_path, 0);
					printf("Deleted snapshot: %s\n", snap_path);
				}
			}
		}
	}
	else
		return 1;

	return 0;
}
