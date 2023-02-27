#define _POSIX_SOURCE

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h> /* gentenv(), abort() */
#include <unistd.h> /* geteuid() */
#include <string.h> /* strlen() */
#include <time.h>
#include <linux/limits.h>  /* for PATH_MAX */
#include <sys/types.h>
#include <sys/stat.h>  /* for check directory */
#include <ctype.h>  /* isdigit() */
#include <dirent.h>
#include <btrfsutil.h>



#define PROGRAM	 "MakeSnap"
#define EXECUTABLE  "mksnp"
#define DESCRIPTION "Make snapshots in a Btrfs filesystem."
#define VERSION	 "0.1a"
#define URL		 "https://github.com/mdomlop/mksnp"
#define LICENSE	 "GPLv3+"
#define AUTHOR	  "Manuel Domínguez López"
#define NICK		"mdomlop"
#define MAIL		"zqbzybc@tznvy.pbz"


#define PATH_MAX_STRING_SIZE 256

#define TIMESTAMP "%Y-%m-%d_%H-%M-%S"
#define EPOCHSECS "%s"
#define SNAPLISTSIZE 64000  /* Btrfs has not this limit, but I think this value is very safe. */

FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

char lastsnap[PATH_MAX];
char snaplist[SNAPLISTSIZE][PATH_MAX];  // List of snapshots in pool_path
int snapls_c = 0;  // Number of elements in snaplist

size_t strnlen(const char s[], size_t maxlen);

int check_root(void)
{
	int uid = getuid();
	int euid = geteuid();
	if (uid != 0 || uid != euid)
	{
		fprintf(stderr, "You must to be root for do this.\n");
		return 1;
	}
	return 0;
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

int is_older(char *last, int now, int fq)
{
	/* Check if last snapshot in pool is old enough from right now. */

	/*printf("El último snapshot es: %d\n", last);
	printf("Segundos desde Epoch: %d\n", now);
	printf("La diferencia es: %d\n", now - last);
	printf("Debe ser mayor de %d\n", fq); */

	time_t epoch;
	struct tm my_tm = {0};
	//char buffer[80];

	memset(&my_tm, 0, sizeof(my_tm));

	if (sscanf(last, "%d-%d-%d_%d-%d-%d",
		&my_tm.tm_year, &my_tm.tm_mon, &my_tm.tm_mday, &my_tm.tm_hour, &my_tm.tm_min, &my_tm.tm_sec) != 6)
	{
		fprintf(stderr, "Bad TIMESTAMP: sscanf failed\n");
		return -1;
	}

	my_tm.tm_isdst = -1;
	my_tm.tm_year -= 1900;
	my_tm.tm_mon -= 1;

	epoch = mktime(&my_tm);

	if (epoch == -1)
		fprintf(stderr, "Error: unable to make time using mktime\n");
	/*
	else {
		strftime(buffer, sizeof(buffer), "%c", &my_tm);
		printf("%s  (epoch=%ld)\n", buffer, (long)epoch);
	}
*/
	//return (long)epoch;
	if ((now - (long)epoch) > fq)
		return 1;
	return 0;
}


int check_is_subvol(char *subvol)
{
    enum btrfs_util_error err;
    err = btrfs_util_is_subvolume(subvol);

    if (!err)
        return 1;
    else if (err == BTRFS_UTIL_ERROR_NOT_BTRFS || err == BTRFS_UTIL_ERROR_NOT_SUBVOLUME)
        fprintf(stderr,
				"%s: Is not a btrfs subvolume: %s\n",
				EXECUTABLE, subvol);
    else
        fprintf(stderr,
				"%s: Is not a subvolume: %s\n",
				EXECUTABLE, subvol);

    return 0;
}

int has_changed(char *last, char *src, char *pool)
{
	/* Check last snapshot generation. If original subvolume and last
	 * snapshot is greater than one line, means that original is different
	 * from last subvolume. */

    long unsigned int srcgen = 0;
    long unsigned int dstgen = 0;

	char dst[256];  // Last snapshot

	strcpy(dst, pool);
	strcat(dst, "/");
	strcat(dst, last);

    if (check_is_subvol(src) && check_is_subvol(dst))
    {
        struct btrfs_util_subvolume_info info;

        btrfs_util_subvolume_info(src, 0, &info);
        srcgen = info.generation;

        btrfs_util_subvolume_info(dst, 0, &info);
        dstgen = info.generation;
    }

    /* If diff is more than 0, subvolumes are different */
	return srcgen - dstgen;
}

int is_dir(char *dir)
{ /* Check if directory alredy exists. */
	struct stat sb;

	if (stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode))
		return 1;  // Directory alredy exists
	return 0;
}


void version (void)
{
	printf ("%s Version: %s\n", PROGRAM, VERSION);
}


void help (int error)
{
	char text[] = "\nUsage:\n\n"

	"\t-i subv			  Set the subvolume.\n"
	"\t-o dir			   Set the output directory.\n\n"

	"\t-f freq			  Set the frequency.\n"

	"\t-h				   Show this help and exit.\n"
	"\t-v				   Show program version and exit.\n";

	if (error)
		fprintf (stderr, "%s\n", text);
	else
	{
		version();
		printf ("%s\n", text);
	}
}


int timetosecs(char *s)
{
	int i;
	int number;
	int size = strlen(s);
	char numberic_part[size+1];
	char symbol = s[size-1];

	for (i=0; i<size-1; i++)
	{
		if (! isdigit(s[i]))
		{
			fprintf(stderr, "Is not a valid number: `%s'\n", s);
			return -1;
		}
		else
			numberic_part[i] = s[i];
	}
	numberic_part[i] = '\0';

	// printf("Parte númerica: `%s'\nSímbolo: `%c'\n", numberic_part, symbol);

	switch (symbol)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 's':
			//puts("Es un número normal, o acaba en s");
			number = atoi(s);
			break;
		case 'm':
			number = atoi(numberic_part) * 60;
			break;
		case 'h':
			number = atoi(numberic_part) * 60 * 60;
			break;
		case 'd':
			number = atoi(numberic_part) * 60 * 60 * 24;
			break;
		case 'y':
			number = atoi(numberic_part) * 60 * 60 * 24 * 365;
			break;
		default:
			fprintf(stderr, "Malformed string: `%s'\n", s);
			return -1;
	}

	return number;
}



/* recursive mkdir */
int mkdir_p(const char *dir, const mode_t mode) {
	char tmp[PATH_MAX_STRING_SIZE];
	char *p = NULL;
	struct stat sb;
	size_t len;

	/* copy path */
	len = strnlen (dir, PATH_MAX_STRING_SIZE);
	if (len == 0 || len == PATH_MAX_STRING_SIZE) {
		return -1;
	}
	memcpy (tmp, dir, len);
	tmp[len] = '\0';

	/* remove trailing slash */
	if(tmp[len - 1] == '/') {
		tmp[len - 1] = '\0';
	}

	/* check if path exists and is a directory */
	if (stat (tmp, &sb) == 0) {
		if (S_ISDIR (sb.st_mode)) {
			return 0;
		}
	}

	/* recursive mkdir */
	for(p = tmp + 1; *p; p++) {
		if(*p == '/') {
			*p = 0;
			/* test path */
			if (stat(tmp, &sb) != 0) {
				/* path does not exist - create directory */
				if (mkdir(tmp, mode) < 0) {
					return -1;
				}
			} else if (!S_ISDIR(sb.st_mode)) {
				/* not a directory */
				return -1;
			}
			*p = '/';
		}
	}
	/* test path */
	if (stat(tmp, &sb) != 0) {
		/* path does not exist - create directory */
		if (mkdir(tmp, mode) < 0) {
			return -1;
		}
	} else if (!S_ISDIR(sb.st_mode)) {
		/* not a directory */
		return -1;
	}
	return 0;
}

int mkpool(char *pool_path)
{
	if (is_dir(pool_path))
	{
		//puts("Pool alredy exists. No problemo.");
		return 2;  // Directory alredy exists
	}
	else
	{
		//printf("Creating pool: %s\n", pool_path);
		int ret = 0;
		//ret = mkdir (pool_path, 0755);
		ret = mkdir_p(pool_path, 0777);
		if (ret)
		{
			fprintf(stderr, "Unable to create pool directory %s\n", pool_path);
			perror("mkpool");
			return 0;
		}
	}

	printf("New pool created: %s\n", pool_path);
	return 1;
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

void get_snapshots(char *pool_path)
{

//char snaplist[SNAPLISTSIZE][PATH_MAX];  // List of snapshots in pool_path
//int snapls_c = 0;  // Number of elements in snaplist

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
}

void list_snapshots(void)
{
	for (int i = 0; i < snapls_c; i++)  // No fail when snapls_c is 0
	{
		printf("[%d] %s\n", i, snaplist[i]);
	}
}


int make_snapshot(char *src, char *dst)
{
    if (check_is_subvol(src))
    {
        if(btrfs_util_create_snapshot(src, dst,
                                   BTRFS_UTIL_CREATE_SNAPSHOT_READ_ONLY,
                                   NULL, NULL) == BTRFS_UTIL_OK)
        return 1;
    }

	fprintf(stderr, "Unable to create snapshot '%s'\n", dst);

	return 0;
}


int main(int argc, char **argv)
{
	int hflag, vflag;
	hflag = vflag = 0;

	char *ivalue = NULL;  // subvolume path
	char *ovalue = NULL;  // directory path
	char *fvalue = NULL;  // frequency

	int freq = 0;

	int c;

	while ((c = getopt (argc, argv, "i:o:f:hv")) != -1)
		switch (c)
		{
			case 'i':  // Set the subvolume
				ivalue = optarg;
				break;
			case 'o':  // Set the output directory
				ovalue = optarg;
				break;
			case 'f':  // Set the frequency
				freq = timetosecs(optarg);
				if (freq < 0)
				{
					fprintf(stderr, "Unknown frequency: %s\n", optarg);
				}
				else
				{
					fvalue = optarg;
				}
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

	if (ivalue && ovalue && fvalue)
	{
		int epochsecs;
		char epochsecs_str[80];
		char timestamp[80];
		int poolstatus = 0;


		char snap_path[PATH_MAX];  // Path to subvolume/.snapshots/pool/timestamp
		char lastsnap[20];  // The last snapshot in pool YYYY-MM-DD_HH-MM-SS

		// Set timestamp
		time_t now;
		struct tm ts;

		time(&now); // Get current time

		ts = *localtime(&now); // Format time
		strftime(timestamp, sizeof(timestamp), TIMESTAMP, &ts);
		strftime(epochsecs_str, sizeof(epochsecs_str), EPOCHSECS, &ts);
		epochsecs = atoi(epochsecs_str);


		//puts("Updating snap_path...");
		strcpy(snap_path, ovalue);  // The path of to the snapshot
		strcat(snap_path, "/");
		strcat(snap_path, timestamp);

		if (check_path_size(snap_path))
			return 1;

		poolstatus = mkpool(ovalue);

		if (poolstatus == 2)  // Alredy exists
		{
			get_snapshots(ovalue);
			strcpy(lastsnap, snaplist[snapls_c - 1]);
			//printf("mksnp: Último snapshot: %s/%s\n", ovalue, lastsnap);

			if (!is_older(lastsnap, epochsecs, freq))
			{
				//printf("No es viejo: %s/%s\n", ovalue, lastsnap);
				return 1;
			}
			if (!has_changed(lastsnap, ivalue, ovalue))
			{
				//printf("No ha cambiado: %s/%s\n", ovalue, lastsnap);
				return 1;
			}
			printf("%s: New snapshot: %s/%s\n", PROGRAM, ovalue, lastsnap);
			make_snapshot(ivalue, snap_path);
			//else
			//	fprintf(stderr,"Muy pronto para un snapshot\n");
		}
		else if (poolstatus == 1)  // New pool
			make_snapshot(ivalue, snap_path);
		else
		{
			fprintf(stderr,"Fallo al crear pool_path: %s\n", ovalue);
			return 1;
		}
	}
	else
	{
		fprintf(stderr, "Faltan argumentos\n");
		return 1;
	}

	return 0;
}
