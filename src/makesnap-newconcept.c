#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h> /* gentenv(), abort() */
#include <string.h> /* strlen() */
#include <ctype.h>  /* isdigit() */
#include <unistd.h> /* uid, euid */
#include <getopt.h>
#include <linux/limits.h>

#include <linux/types.h>  /* for list directory */
#include <sys/stat.h>  /* for check directory */
#include <sys/statfs.h>  /* for check btrfs? */
//#include <linux/magic.h> /* for check btrfs */
#include <dirent.h>
#include <errno.h>

#include <time.h>
#include <fcntl.h>

#include <signal.h>

#define PROGRAM     "MakeSnap"
#define EXECUTABLE  "makesnap"
#define DESCRIPTION "Make and manage snapshots in a Btrfs filesystem."
#define VERSION     "0.2a"
#define URL         "https://github.com/mdomlop/makesnap"
#define LICENSE     "GPLv3+"
#define AUTHOR      "Manuel Domínguez López"
#define NICK        "mdomlop"
#define MAIL        "zqbzybc@tznvy.pbz"

#define CONFIGFILE "/etc/makesnaprc"
#define PIDFILE "/tmp/makesnap.pid"
#define FIFOFILE "/tmp/makesnap.fifo"

//#define TIMESTAMP "%Y-%m-%d_%H.%M.%S"
#define TIMESTAMP "%Y%m%d-%H%M%S"
#define STORE "/.snapshots/"  // Fixed location
#define SNAPLISTSIZE 64000  /* Btrfs has not this limit, but I think this value is very safe. */

#define DEFSUBV "/"  // root directory
#define DEFQUOTA "30"  // 30 snapshots
#define DEFFREQ "1h"  // every hour

int getopt(int argc, char *const argv[], const char *optstring);
extern int optind, optopt;
void tstohuman(char *str);
int writefifo(const char *str);

char configtab[SNAPLISTSIZE][PATH_MAX];  // List of entries for automatic snapshotting
int configtab_c = 0;  // Number of elements in configtab

char *frequency = NULL;  // Frequency
char *quota = NULL;

char *subv_path = NULL;  // Path to subvolume
char store_path[PATH_MAX];  // Path to subvolume/.snapshots
char pool_path[PATH_MAX];  // Path to subvolume/.snapshots/pool. Pool is automatically freq@quota
char snap_path[PATH_MAX];  // Path to subvolume/.snapshots/pool/timestamp

char snaplist[SNAPLISTSIZE][PATH_MAX];  // List of snapshots in pool_path
int snapls_c = 0;  // Number of elements in snaplist

char timestamp[80];
char tshuman[80];  // Human readable timestamp, for list_snapshots()

int pid;
char fifocmd[PATH_MAX];


struct Pool
{
	char subvolume[PATH_MAX];
	int frequency;
	int quota;
};

struct Pool pooltab[SNAPLISTSIZE]; // This contents all pools and its configs
int pooltab_c = 0;

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
	printf ("%s Version: %s\n", PROGRAM, VERSION);
}

void help (int error)
{
	char text[] = "\nUsage:\n\n"

	"\t-s                   Start daemon.\n"
	"\t-a sub freq quota    Add entry to config.\n"
	"\t-p                   Print numbered config entries.\n"
	"\t-w                   Write config (Saving) to " CONFIGFILE ".\n"
	"\t-d delete            Delete the selected config entry.\n\n"

	"\t-l [sub] [pool]      Show a numbered list of snapshots in subvolume.\n"
	"\t-c sub [pool]        Cleans (deletes) all snapshots in subvolume/pool.\n"
	"\t-r snapshotn         Remove the selected snapshotn.\n\n"
	"\t-f sub pool          Force creation of snapshot of sub y  pool.\n"

	"\t-h                   Show this help and exit.\n"
	"\t-v                   Show program version and exit.\n";

	if (error)
		fprintf (stderr, "%s\n", text);
	else
	{
		version();
		printf ("%s\n", text);
	}
}

int loadconfig(void)
{
	//puts("Cargando configuración...");
	int c;
	char buffer[PATH_MAX];
	int buf_index = 0;
	int tab_index = 0;

	FILE *fd = fopen(CONFIGFILE, "r");
	if (fd)
	{
		while ((c =getc(fd)) != EOF)
		{
			if (c == '\n')
			{
				buffer[buf_index] = '\0';

				//printf("El caracter es %c %c\n", buffer[0], '#');

				if (buffer[0] == 'm')  // All lines must start with makesnap [...]
				{

					strcpy(configtab[tab_index], buffer);
					tab_index++;
				}
				strcpy(buffer,"");
				buf_index = 0;
				continue;
			}

			buffer[buf_index] = c;
			buf_index++;
			//putchar(c);
		}
		fclose(fd);
	}
	// No fail here.

	configtab_c = tab_index;

	return tab_index;  // Number of elements in config. configtab_c = loadconfig();
}

void printconfig(void)
{
	for (int i = 0; i < configtab_c; i++)
		printf("%d: %s\n", i, configtab[i]);
}

void writeconfig(void)
{
	printf("%s\n", "# <where> <frecuency> <quota>");
	for (int i = 0; i < configtab_c; i++)
		printf("%s\n", configtab[i]);
}

void execconfig(void)
{
	for (int i = 0; i < configtab_c; i++)
		printf("%d: %s\n", i, configtab[i]);
		//system("%s", configtab[i]);
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
            exit (1);
        }
        else
            numberic_part[i] = s[i];
    }
    numberic_part[i] = '\0';

    //printf("Parte númerica: `%s'\nSímbolo: `%c'\n", numberic_part, symbol);

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
            fprintf(stderr, "Malformed string: %s\n", s);
            exit(1);
    }

    return number;
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

int is_dir(char *dir)
{ /* Check if directory alredy exists. */
	struct stat sb;

	if (stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode))
		return 1;  // Directory alredy exists
	return 0;
}

void sort_snapshots(void)
{
	char temp[4096];
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
		tstohuman(snaplist[i]);
		printf("[%d]\t%s\t%s%s\n", i, tshuman, pool_path, snaplist[i]);
	}
}

int delete_snapshot (int index)
{
	if (check_root()) return 1;

	char mysnap[PATH_MAX];
	char command[PATH_MAX];
	int ret = 0;

	if (index >= snapls_c)
	{
		fprintf(stderr, "The selected snapshot does not exist.\n");
		return 1;
	}

	strcpy(mysnap, pool_path);
	strcat(mysnap, snaplist[index]);

	strcpy(command, "btrfs subvolume delete '");
	strcat(command, mysnap);
	strcat(command, "' > /dev/null");
	//printf("Command: %s\n", command);

	ret = system(command);
	if(ret)
	{
		fprintf(stderr,
				"An error occurred while trying to delete the snapshot: "
				"'%s'\n", mysnap);
		perror("");
		return 1;
	}
		printf("Delete snapshot: [%d] %s\n", index, mysnap);

	return 0;
}

int clean_all_snapshots(void)
{
	int ret;
	errno = 0;

	for (int i = 0; i < snapls_c; i++)
	{
		if(delete_snapshot(i))
			return 1;
	}

	ret = rmdir(pool_path);
	if(ret == -1)
	{
		switch (errno)
		{
			case ENOENT:
				return 0;
			default:
				perror("rmdir pool");
				return 1;
		}
	}
	else
	{
		printf("Empty pool deleted: %s\n", pool_path);
	}


	ret = rmdir(store_path);
	if(ret == -1)
	{
		switch (errno)
		{
			case ENOTEMPTY:
				return 0;
			default:
				perror("rmdir");
				return 1;
		}
	}
	else
	{
		printf("Empty store deleted: %s\n", store_path);
	}

	return 0;
}

int get_snapshots(void)
{
	//printf("Reloading snapshots from %s\n", pool_path);

	DIR *dp;
	struct dirent *ep;
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
	return 0;
}


int create_store(void)
{
	if (is_dir(pool_path))
	{
		puts("Store and pool alredy exists. No problemo.");
		return 0;  // Directory alredy exists
	}
	else if (!is_dir(store_path))
	{
		int ret = 0;
		//ret = mkdir (store_path, 0755);
		ret = mkdir (store_path, 0777);
		if (ret)
		{
			fprintf(stderr, "Unable to create store directory %s\n", store_path);
			perror("create_store");
			return 1;
		}
		else
			printf("New store created: %s\n", store_path);
	}

	if (!is_dir(pool_path))
	{
		int ret = 0;
		//ret = mkdir (pool_path, 0755);
		ret = mkdir (pool_path, 0777);
		if (ret)
		{
			fprintf(stderr, "Unable to create pool directory %s\n", pool_path);
			perror("create_store");
			return 1;
		}
		else
			printf("New pool created: %s\n", pool_path);
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

	printf("Quota checking... (%d / %d) [%d]\n", snapls_c, quota, diff);
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
	printf("Trying to create snapshot of '%s' in '%s'\n", subv_path, snap_path);
	char command[PATH_MAX];
	int ret = 0;

	strcpy(command, "btrfs subvolume snapshot -r '");
	strcat(command, subv_path);
	strcat(command, "' '");
	strcat(command, snap_path);
	strcat(command, "' > /dev/null");
	//printf("Command: %s\n", command);


	if (is_dir(snap_path))
	{
		fprintf(stderr, "Sorry. Too fast '%s'\n", snap_path);
		return 0;  // Directory alredy exists
	}

	//ret = mkdir (snap_path, 0755);
	ret = system(command);

	if (ret)
	{
		fprintf(stderr, "Unable to create snapshot '%s'\n", snap_path);
		return 1;
	}

	printf("Create snapshot of '%s' in '%s'\n", subv_path, snap_path);
	return 0;
}

int create_snapshot(char * quota)
{
	if (check_root()) return 1;

	int quota_int;

	//printf("%s\n", subv_path);
	//printf("%s\n", pool_path);
	//printf("%s\n", snap_path);

	if (is_integer(quota))
		quota_int = atoi(quota);
	else
	{
		fprintf(stderr, "Invalid quota setted.\n");
		return 1;
	}

	if(create_store())
		return 1;

	check_quota(quota_int);
	make_snapshot();

	return 0;
}

void tstohuman(char *s)
{
	char temp4[5];
	char temp2[3];
	int aux;

    struct tm t;
    struct tm ts;

    time_t t_of_day;

	//char tshuman[80];

	memcpy(temp4, &s[0], 4);
	aux =atoi(temp4);
    t.tm_year = aux-1900;  // Year - 1900

	memcpy(temp2, &s[4], 2);
	aux =atoi(temp2);
    t.tm_mon = aux;           // Month, where 0 = jan

	memcpy(temp2, &s[6], 2);
	aux =atoi(temp2);
    t.tm_mday = aux;          // Day of the month

	memcpy(temp2, &s[9], 2);
	aux =atoi(temp2);
    t.tm_hour = aux;

	memcpy(temp2, &s[11], 2);
	aux =atoi(temp2);
    t.tm_min = aux;

	memcpy(temp2, &s[13], 2);
	aux =atoi(temp2);
    t.tm_sec = aux;

    //t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
    t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
    t_of_day = mktime(&t);

	ts = *localtime(&t_of_day);
	strftime(tshuman, sizeof(tshuman), "%c", &ts);

	//printf("tstohuman: %s -> %s\n", s, tshuman);
	//return &tshuman;
}

void update_ts(void)
{
	//puts("Updating timestamp...");
	time_t now;
	struct tm ts;

	time(&now); // Get current time

	ts = *localtime(&now); // Format time
	strftime(timestamp, sizeof(timestamp), TIMESTAMP, &ts);
}

int update_snap_path(void)
{
	//puts("Updating snap_path...");
	strcpy(snap_path, pool_path);  // The path of to the snapshot
	strcat(snap_path, timestamp);

	if (check_path_size(snap_path))
		return 1;
	return 0;
}

int has_changed(void)
{
	/* Checks if subv_path has change from older_snapshot */
	return 0;
}

int addpool(char *subv, char *freq, char *quota)
{
	strcpy(pooltab[pooltab_c].subvolume, subv);
	pooltab[pooltab_c].frequency = timetosecs(freq);
	if (is_integer(quota))
		pooltab[pooltab_c].quota = atoi(quota);
	else
	{
		fprintf(stderr, "Invalid quota: %s\n", quota);
		return 1;
	}

	pooltab_c++;
	return 0;
}


int writepid(void)
{
	char *pidfile = PIDFILE;
	FILE *fd = fopen(pidfile, "w");

	fprintf(fd, "%d", getpid());
	fclose(fd);
	return 0;
}

int readpid(void)
{
	char *pidfile = PIDFILE;
	FILE *fd = fopen(pidfile, "r");
	int c;
	char buffer[8];  // /proc/sys/kernel/pid_max has 7 digits
	int buf_index = 0;

	if (fd)
	{
		while ((c =getc(fd)) != EOF)
		{
			buffer[buf_index] = c;
			buf_index++;
		}

		buffer[buf_index] = '\0';
	}
	fclose(fd);

	printf("PID is: %s\n", buffer);
	if (is_integer(buffer))
		return atoi(buffer);
	return -1;
}

int readfifo(void)
{

	int n;
    int fd;
    char buf[PATH_MAX];
    char myfifo[PATH_MAX] = FIFOFILE;


	if ((access(myfifo, F_OK) == 0))
	{
		fd = open(myfifo, O_RDONLY);
		/* open, read, and display the message
			from the FIFO */
		if ((n = read(fd, buf, PATH_MAX)) >= 0)
				{
					//buf[n] = 0;
					buf[n] = '\0';
					printf("Received: %s\n", buf);
					strcpy(fifocmd, buf);
				}
		else
			perror("read");
		close(fd);
	}

	return 0;
}

int writefifo(const char *buf)
{
	int fd;
    char *myfifo = FIFOFILE;

	printf("EScribiré: %s\n", buf);
	printf("%s\n", buf);

    /* create the FIFO (named pipe) */
    mkfifo(myfifo, 0666);

    /* write to the FIFO */
    fd = open(myfifo, O_WRONLY);
    write(fd, buf, strlen(buf));
    close(fd);

    /* remove the FIFO */
    unlink(myfifo);

	return 0;
}

char cutcmd(void)
{
	int i;
	int count = strlen(fifocmd);
	char c = fifocmd[0];

	for (i=0; i < count; i++)
		fifocmd[i] = fifocmd[i+2];
	return c; 
}


void handle_sigint(int sig)
{
	printf("Caught signal %d\n", sig);
	exit(0);
}

void handle_sigusr1(int sig)  // Read fifo
{
	readfifo();
	char cmd = cutcmd();

	switch (cmd)
	{
		case 'a': // Add to pooltab
			printf("La señal es '%c', los valores: '%s'\n", cmd, fifocmd);
			break;
		case '?':
			exit(1);
		default:
			abort();
	}
}

void handle_sigusr2(int c, char **strs)  // Write fifo and forces server to read
{
	puts("KILL");
	//writefifo(**strs);
	//kill


		//if (addpool(argv[2], argv[3], argv[4]))
		//	return 1;
}

int main(int argc, char **argv)
{
	int hflag, vflag, sflag, pflag, wflag, aflag, lflag, cflag, fflag;
	hflag = vflag = sflag = pflag = wflag = aflag = lflag = cflag = fflag = 0;

	/*
	char *dvalue = NULL;
	char *rvalue = NULL;

	char *def_quota = DEFQUOTA;
	char *def_subvol = DEFSUBV;
	char *def_freq = DEFFREQ;

	char *env_subvol = getenv("MAKESNAPSUBV");
	char *env_freq = getenv("MAKESNAPFREQ");
	char *env_quota = getenv("MAKESNAPQUOTA");

	int index = 0; */
	int c;

	while ((c = getopt (argc, argv, "hvspwalcfd:r:")) != -1)
		switch (c)
		{
			case 'h':
				hflag = 1;
				break;
			case 'v':
				vflag = 1;
				break;
			case 's':
				sflag = 1;
				break;
			case 'p':
				pflag = 1;
				break;
			case 'w':
				wflag = 1;
				break;
			case 'a':
				aflag = 1;
				break;
			case 'l':
				lflag = 1;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'f':
				fflag = 1;
				break;
			/*case 'd':
				dvalue = optarg;
				break;
			case 'r':
				rvalue = optarg;
				break;*/
			case '?':
				/*if (optopt == 'd')
					fprintf(stderr, "Option -%c requires an argument (an entry number).\n", optopt);
				else if (optopt == 'r')
					fprintf(stderr, "Option -%c requires an argument (a snapshot number).\n", optopt);
				else*/ if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n",
							optopt);
                return 1;
			default:
				abort ();
		}

	if (argc - optind > 3)  // Max non-option argumets are 3
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
	else if (sflag)  // Start as a daemon
	{
		printf("Daemonize!\n");
		writepid();
		for (;;)
		{
			signal(SIGINT, handle_sigint);
			signal(SIGUSR1, handle_sigusr1);  // read fifo
			//update_ts();
			/*
			for (int i = 0; i < pooltab_c; i++)
			{
				printf("Subvolume: %s\nFrequency: %d\nQuota: %d\n",
					pooltab[i].subvolume,
					pooltab[i].frequency,
					pooltab[i].quota);
			}
			*/
			puts("Sleeping for 60 seconds...");
			sleep(60);
		}
	}
	else if (aflag) // Add to daemon
	{
		if (sflag || pflag || wflag || lflag || cflag || fflag)
		{
			fprintf(stderr, "Too many options\n");
			return 1;
		}
		if ((argc - optind) != 3)
		{
			fprintf(stderr, "Sorry. Option -a requires three arguments.\n");
			help(1);
			return 1;
		}

		pid = readpid();
		if (pid)
		{
			strcpy(fifocmd, "a"); 
			puts("If aflag");
			for (int i=1; i<4; i++)
			{
				strcat(fifocmd, " ");
				strcat(fifocmd, argv[i+1]);
			}
			
			printf("KILL %d %d\n", pid, SIGUSR1);
			kill(pid, SIGUSR1);

			puts("Write");

			writefifo(fifocmd);
		}
		else
		{
			fprintf(stderr, "Failed to getpid from %s\n", PIDFILE);
			return 1;
		}
/*		else

		//for (int index = optind; index < argc; index++)
		//	printf("Non-option argument %d: %s\n", index, argv[index]);
		printf("Subvolume: %s\nFrequency: %d\nQuota: %d\n",
			   pooltab[pooltab_c].subvolume,
				pooltab[pooltab_c].frequency,
				pooltab[pooltab_c].quota);
*/
	}

	// Insert retail

		return 0;
}
