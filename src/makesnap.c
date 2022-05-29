#include <stdio.h>
#include <stdlib.h> /* gentenv(), abort() */
#include <string.h> /* strlen() */
#include <ctype.h>  /* isdigit() */
#include <unistd.h>
#include <getopt.h>
#include <linux/limits.h>

#include <sys/types.h>  /* for list directory */
#include <sys/stat.h>  /* for check directory */
#include <dirent.h>

#include <time.h>

#define PROGRAM     "MakeSnap"
#define EXECUTABLE  "makesnap"
#define DESCRIPTION "Make and manage snapshots in a Btrfs filesystem."
#define VERSION     "0.1a"
#define URL         "https://github.com/mdomlop/makesnap"
#define LICENSE     "GPLv3+"
#define AUTHOR      "Manuel Domínguez López"
#define NICK        "mdomlop"
#define MAIL        "zqbzybc@tznvy.pbz"

//#define TIMESTAMP "%Y-%m-%d_%H.%M.%S"
#define TIMESTAMP "%Y%m%d-%H%M%S"
#define STORE "/.snapshots/"
#define SNAPLISTSIZE 64000  /* Btrfs has not this limit, but I think this value is very safe. */

#define DEFQUOTA "30"  // 30 snapshots

int getopt(int argc, char *const argv[], const char *optstring);
extern int optind, optopt;

char timestamp[80];
char *subv_path = NULL;  // Path to subvolume
char store_path[PATH_MAX];  // Path to subvolume/.snapshots
char snap_path[PATH_MAX];  // Path to subvolume/.snapshots/timestamp

char snaplist[SNAPLISTSIZE][PATH_MAX];  // List of snapshots in store_path
int snapls_c = 0;  // Number of elements in snaplist

int is_integer (char * s)
/* Determines if passed string is a positive integer */
{
    short c;
    short sc = strlen(s);
    for ( c = 0; c < sc; c ++ )
    {
        if (isdigit (s[c]))
            continue;
        else
			return 0;
    }
    return 1;
}

void version (void)
{
	printf ("Version: %s\n", VERSION);
}

void help (int error)
{
	char text[] = "\nUsage:\n"
	"\tmakesnap [-cflhv] [-q quota] [-r restore] [-d delete]  "
	"[subvolume]\n\n"
	"Options:\n\n"
	"\tsubvolume    Path to subvolume."
	" Defaults to current directory.\n"
	"\t-l           Show a numbered list of snapshots in subvolume.\n"
	"\t-c           Cleans (deletes) all snapshots in subvolume.\n"
	"\t-h           Show this help and exit.\n"
	"\t-v           Show program version and exit.\n"
	"\t-q quota     Maximum quota of snapshots for subvolume. Defaults to `30'.\n"
	"\t-r restore   Restores the selected snapshot.\n"
	"\t-d delete    Deletes the selected snapshot.\n";

	if (error)
		fprintf (stderr, "%s\n", text);
	else
		printf ("%s\n", text);
}


int check_path_size(char *path)
{
	if (path)
	{
		int sub_path_size = strlen(path);

		//if(sub_path_size > 10)  // Testing
		if(sub_path_size > PATH_MAX)
		{
			fprintf (stderr, "Sorry. Path length is longer (%d) than PATH_MAX (%d):\n"
					"%s\n", sub_path_size, PATH_MAX, path);
			return 1;
		}
	}
	return 0;
}

void sort_snapshots(void)
{
	char temp[4096];
	for (int i = 0; i < snapls_c; i++)
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
}

void list_snapshots(void)
{
	for (int i = 0; i < snapls_c; i++)
		printf("[%d]\t%s\n", i, snaplist[i]);
}

int delete_snapshot (int index)
{
	char mysnap[PATH_MAX];

	if (index >= snapls_c)
	{
		fprintf(stderr, "The selected snapshot does not exist.\n");
		return 1;
	}

	strcpy(mysnap, store_path);
	strcat(mysnap, snaplist[index]);

	if(rmdir(mysnap))
	{
		fprintf(stderr,
				"An error occurred while trying to delete the snapshot: "
				"%s\n", mysnap);
		perror("");
		return 1;
	}
	else
		printf("Se borra la copia %d: %s\n", index, mysnap);

	return 0;
}

int clean_all_snapshots(void)
{
	for (int i = 0; i < snapls_c; i++)
	{
		if(delete_snapshot(i))
			return 1;
	}

	if(rmdir(store_path))
	{
		fprintf(stderr, "Couldn't delete the empty store: %s\n", store_path);
		perror("");
		return 1;
	}
	else
	{
		printf("Se borra el almacén vacío: %s\n", store_path);
	}
	return 0;
}

int get_snapshots(void)
{
	DIR *dp;
	struct dirent *ep;
	int n = 0;

	dp = opendir (store_path);
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
	// No fail if store does not exists
	/*
	else
	{
		fprintf(stderr, "Couldn't open the directory: %s\n", store_path);
		perror ("");
	}
	*/

	snapls_c = n;
	return 0;
}

int restore_snapshot (char * index)
{
	char mysnap[PATH_MAX];
	int i;

	if (is_integer(index))
		i = atoi(index);
	else
	{
		fprintf(stderr, "The selected snapshot number is not valid: %s\n",
			   index);
		return 1;
	}

	if (i >= snapls_c)
	{
		fprintf(stderr, "The selected snapshot does not exist.\n");
		return 1;
	}

	strcpy(mysnap, store_path);
	strcat(mysnap, snaplist[i]);

	printf("Se restaurará la copia: %s desde %s en %s\n", index, mysnap, subv_path);
	return 0;
}

int create_store(void)
{
	struct stat sb;

	if (stat(store_path, &sb) == 0 && S_ISDIR(sb.st_mode))
	{
		puts("Store alredy exists. No problemo.");
		return 0;  // Directory alredy exists
	}
	else
	{
		int ret = 0;
		ret = mkdir (store_path, 0755);
		if (ret)
		{
			fprintf(stderr, "Unable to create directory %s\n", store_path);
			perror("create_store");
			return 1;
		}
		else
			printf("Se crea el almacén: %s\n", store_path);
	}
	return 0;
}

int check_if_subvol(char *where)
{
	struct stat sb;
	size_t inode;

	if (stat(where, &sb) == -1)
	{
		perror("stat");
		exit(EXIT_FAILURE);
	}

	//printf("I-node number %s: %ld\n", where, (long) sb.st_ino);

	inode = (long) sb.st_ino;
	
	switch (inode)
	{
		case 2:
		case 256:
			return 0;
		default:
			return 1;
	}
}

int check_quota(int quota)
{
	int diff = quota - snapls_c;
	int n = 0;

	printf("Chequeo quota... (%d / %d) [%d]\n", snapls_c, quota, diff);
	//list_snapshots();
	//puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	//printf("Chequeo quota... (%d / %d) [%d]\n", snapls_c, quota, diff);
	for (int i = diff; i <= 0; i++)
	{
		if(delete_snapshot(n))
			return 1;
		n++;
	}
		
	//get_snapshots();
	//list_snapshots();

	return 0;
}

int make_snapshot(void)
{
	int ret = 0;
	ret = mkdir (snap_path, 0755);

	if (ret)
	{
		fprintf(stderr, "Unable to create snapshot %s\n", snap_path);
		return 1;
	}
	else
		printf("%s\n", snap_path);
	return 0;
}

int create_snapshot(char * quota)
{
	int quota_int;

	//printf("%s\n", subv_path);
	//printf("%s\n", store_path);
	//printf("%s\n", snap_path);

	if (is_integer(quota))
		quota_int = atoi(quota);
	else
	{
		fprintf(stderr, "Invalid quota.\n");
		return 1;
	}

	if(create_store())
		return 1;

	check_quota(quota_int);
	printf("Se crea del snapshot sin forzar en: ");
	make_snapshot();

	return 0;
}

int main(int argc, char **argv)
{
	char *defquota = DEFQUOTA;

	int hflag = 0;
	int vflag = 0;
	int cflag = 0;
	int lflag = 0;

	char *qvalue = NULL;
	char *rvalue = NULL;
	char *dvalue = NULL;
	//char *subvolume = NULL;

	char *env_quota = getenv("MAKESNAPQUOTA");

	char cwd[PATH_MAX];

	time_t now;
	struct tm ts;
	//char buf[80];

	// Get current time
	time(&now);

	// Format time
	ts = *localtime(&now);
	strftime(timestamp, sizeof(timestamp), TIMESTAMP, &ts);
	//printf("%s\n", timestamp);


	int index = 0;

	int c;

	while ((c = getopt (argc, argv, "clhvq:r:d:")) != -1)
		switch (c)
		{
			case 'h':
				hflag = 1;
				break;
			case 'v':
				vflag = 1;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'l':
				lflag = 1;
				break;
			case 'q':
				qvalue = optarg;
				break;
			case 'r':
				rvalue = optarg;
				break;
			case 'd':
				dvalue = optarg;
				break;
			case '?':
				if (optopt == 'a')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (optopt == 'q')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (optopt == 'r')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (optopt == 'd')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n",
							optopt);
                return 1;
			default:
				abort ();
		}

	if (argc - optind > 1)  // Only one is more easy for user.
	{
		fprintf (stderr, "Sorry. Only one subvolume is accepted.\n");
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

	if (argv[optind])  // Determining the subvolume
		subv_path = argv[optind];
	else
		if (getcwd(cwd, sizeof(cwd)) == NULL)  // cwd must be current directory
		{
			perror("Can not get the current working directory.\n");
			return 1;
		}
		else
			subv_path = cwd;

	if (check_if_subvol(subv_path))  // Quit on wrong subvolume selection
	{
		fprintf(stderr, "Selected directory is not a subvolume: %s\n",
				subv_path);
		return 1;
	}

	strcpy(store_path, subv_path);
	strcat(store_path, STORE);

	strcpy(snap_path, store_path);
	strcat(snap_path, timestamp);

	if (check_path_size(snap_path))
		return 1;

	if (!qvalue)
	{
		if (env_quota)
			qvalue = env_quota;
		else
			qvalue = defquota;
	}


	get_snapshots();
	sort_snapshots();

	//printf("cflag = %d, lflag = %d, qvalue = %s, rvalue = %s, dvalue = %s, subv_path = %s\n", cflag, lflag, qvalue, rvalue, dvalue, subv_path);


	if (rvalue && dvalue)
	{
		fprintf (stderr, "Sorry. Restore and delete simultaniously is not accepted.\n");
		return 2;
	}
	else if (rvalue)
	{
		if (restore_snapshot(rvalue))
			return 1;
	}
	else if (dvalue)
	{
		if (is_integer(dvalue))
			index = atoi(dvalue);
		else
		{
			fprintf(stderr, "The selected snapshot does not exist.\n");
			return 1;
		}
		if (delete_snapshot(index))
			return 1;
	}

	if (rvalue || dvalue)  // No execute other options
		return 0;

	if (lflag)
		list_snapshots();
	else if (cflag)
	{
		if(clean_all_snapshots())
			return 1;
	}
	else
		create_snapshot(qvalue);


	/*
	for (int index = optind; index < argc; index++)
		printf("Non-option argument: %s\n", argv[index]);
	return 0;
	*/

	return 0;
}
