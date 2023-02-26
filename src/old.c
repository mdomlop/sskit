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
#define SERVERFIFO "/tmp/makesnap.server.fifo"

//#define TIMESTAMP "%Y-%m-%d_%H.%M.%S"
#define TIMESTAMP "%Y%m%d-%H%M%S"
#define STORE "/.snapshots/"  // Fixed location
#define SNAPLISTSIZE 64000  /* Btrfs has not this limit, but I think this value is very safe. */

#define DEFSUBV "/"  // root directory
#define DEFQUOTA "30"  // 30 snapshots
#define DEFFREQ "1h"  // every hour

int dryrun = 0;  // Perform a trial run with no changes made.
int getopt(int argc, char *const argv[], const char *optstring);
extern int optind, optopt;
void tstohuman(char *str);
int writefifo(char *fifo, char *msg);
char *readfifo(char *fifopath, char *buffer);
void serverquit(int n);
void clientquit(int n);
void handle_sigterm(int sig);
void handle_sighup(int sig);
void handle_sigint(int sig);
void handle_sigusr1();
void handle_sigusr2();
struct Fifomsg splitmsg(char *msg);
void speakto_server(int serverpid, char *message);
void speakto_client(int clientpid, char *fifo, char *message);
int addpool(char *subv, char *store, char *freq, char *quota);
void lspool(char *clientpid, char *fifo);

char configtab[SNAPLISTSIZE][PATH_MAX];  // List of entries for automatic snapshotting
int configtab_c = 0;  // Number of elements in configtab

char clientfifo[PATH_MAX];
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
char fifoline[PATH_MAX];

struct Fifomsg
{
	char cmd[1024];  // Change for PATH_MAX
	char pid[1024];
	char fifo[1024];
	char args[1024][PATH_MAX];
};
//struct Fifomsg msg;


struct Pool
{
	char subvolume[20000];
	char store[1024];
	char sfrequency[1024];
	char squota[1024];
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
	"\t-a                   Add entry to config.\n"
	"\t-p                   Print numbered config entries.\n"
	"\t-w                   Write config (Saving) to " CONFIGFILE ".\n"
	"\t-d n                 Delete the selected config entry.\n\n"

	"\t-i subv              Set the subvolume.\n"
	"\t-o dir               Set the output directory.\n\n"

	"\t-f freq              Set the frequency.\n"
	"\t-q quota             Set the quota.\n\n"

	"\t-l                   Show a numbered list of all snapshots ordered by\n"
	"\t                     creation date.\n"
	"\t-r n [n ...]         Remove the selected n_snapshots.\n\n"
	//"\t-F                   Force a snapshot creation.\n"  // No necesario. Basta con no especificar la frecuencia o la cuota.

	"\t-n                   Perform a trial run with no changes made.\n"
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
	printf("%s\n", "# <from> <where> <frecuency> <quota>");
	for (int i = 0; i < pooltab_c; i++)
		printf("makesnap -a -i '%s' -o '%s' -f %s -q %s\n",
				pooltab[i].subvolume,
				pooltab[i].store,
				pooltab[i].sfrequency,
				pooltab[i].squota);
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
			return -1;
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
            fprintf(stderr, "Malformed string: `%s'\n", s);
			return -1;
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

	if(dryrun)
		strcpy(command, "echo [DRY RUN] btrfs subvolume delete '");
	else
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
		serverquit(EXIT_FAILURE);
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

	if(dryrun)
		strcpy(command, "echo [DRY RUN] btrfs subvolume snapshot -r '");
	else
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

int addpool(char *subv, char *store, char *freq, char *quota)
{
	strcpy(pooltab[pooltab_c].subvolume, subv);
	strcpy(pooltab[pooltab_c].store, store);
	strcpy(pooltab[pooltab_c].sfrequency, freq);
	pooltab[pooltab_c].frequency = timetosecs(freq);
	if (is_integer(quota))
	{
		strcpy(pooltab[pooltab_c].squota, quota);
		pooltab[pooltab_c].quota = atoi(quota);
	}
	else
	{
		fprintf(stderr, "Invalid quota: %s\n", quota);
		return 1;
	}

	pooltab_c++;
		printf("addpool: Añado a pooltab: '%s' '%s' '%s' '%s'\n", subv, store, freq, quota);
	return 0;
}

void lspool(char *clientpid, char *fifo)
{
	puts("lspool: INICIO");
	char aux[2077120055] = "";
	char message[2077120055] = "";
	for (int i = 0; i < pooltab_c; i++)
	{
		sprintf(aux, "%d: Subvolume '%s' in '%s' every %s until %s snapshots\n",
				i,
				pooltab[i].subvolume,
				pooltab[i].store,
				pooltab[i].sfrequency,
				pooltab[i].squota);

		strcat(message, aux);
	}

	//printf("lspool: ~~~~~~~~~~~~~~~~~~~~~~~~\n%s\n~~~~~~~~~~~~~~~~~~~~~\n", message);
	speakto_client(atoi(clientpid), fifo, message);
}

int rmpool(int n)
{
	printf("rmpool: Borro %d (%d): %s %s %s\n", n, pooltab_c -1, pooltab[n].subvolume, pooltab[n].sfrequency, pooltab[n].squota);

	if ((n > pooltab_c - 1) || (n < 0))
	{
		fprintf(stderr, "No pool has this number: %d (max: %d)\n",
			   	n, pooltab_c - 1);
		return 0;
	}

	for (int i = n; i < pooltab_c; i++)
	{
		if (pooltab_c == 0) continue;  // Skip 0
		pooltab[i] = pooltab[i+1];
	}

	pooltab_c--;
	printf("Tamaño: %d\n", pooltab_c);

	return 1;
}

int writepid(void)
{
	char *pidfile = PIDFILE;
	FILE *fd = fopen(pidfile, "w");

	fprintf(fd, "%d", getpid());
	fclose(fd);
	return 0;
}

void serverquit(int n)
{
	printf("Borro %s\n", SERVERFIFO);
	unlink(SERVERFIFO);
	unlink(PIDFILE);
	exit(n);
}
void clientquit(int n)
{
	if (unlink(clientfifo) == -1)
	{
		fprintf(stderr, "No he podido borrar el pipe del cliente: ");
		switch(errno)
		{
			case ENOENT:
				fprintf(stderr, "ENOENT faliure on %s: A component of path does not name an existing file or path is an empty string.\n", clientfifo);
				exit(1);
			case EACCES:
				fprintf(stderr, "EACCES faliure\n");
				exit(1);
			case EBUSY:
				fprintf(stderr, "BUSY faliure\n");
				exit(1);
			default:
				fprintf(stderr, "El fallo fue: %d (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", errno, EACCES, EBUSY, ELOOP, ENAMETOOLONG, ENOENT, ENOTDIR, EPERM, EROFS, EBADF, ENOTDIR, EEXIST, ENOTEMPTY, ETXTBSY, EINVAL);
				fprintf(stderr, "Pruebo otra vez.\n");
				if (unlink(clientfifo) == -1)
				{
					fprintf(stderr, "He vuelto a fallar.\n");
					exit(1);
				}
				else
					exit(n);
		}
	}
	exit(n);
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

	if (is_integer(buffer))
		return atoi(buffer);
	return -1;
}

char *OLDreadfifo(char *myexfifo, char *buffer)
{
	FILE *fd = fopen(myexfifo, "r");
	int c;
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

	return buffer;
}
char* readfifo(char *myfifo, char *buf)
{
	int n;
    int fd;

	if ((access(myfifo, F_OK) == 0))
	{
		fd = open(myfifo, O_RDONLY);
		if ((n = read(fd, buf, PATH_MAX - 1)) > 0)
		{
			buf[n] = '\0';
			close(fd);
		}
		else
		{
			perror("readfifo: Fail to read");
			close(fd);
			return NULL;
		}
	}

	return buf;
}

int writefifo(char *myfifo, char *message)
{
	/* Writes message to myfifo */
	int fd;
    fd = open(myfifo, O_WRONLY);
    write(fd, message, strlen(message));
    close(fd);
	return 0;
}

char cutcmd(char *message)
{
	int i;
	int count = strlen(message);
	char c = message[0];

	for (i=0; i < count; i++)
		message[i] = message[i+2];
	return c;
}

struct Fifomsg splitmsg(char *message)
{
	printf("splitmsg: El mensaje: %s\n", message); //FIXME
	struct Fifomsg msg;
	int count = strlen(message);
	char aux[PATH_MAX];
	int j = 0;
	int o = 0;
	int a = 0;

	for (long unsigned int i = 0; i < sizeof(msg.args)/sizeof(msg.args[0]); i++)
		strcpy(msg.args[i], "");

	for (int i = 0; i < count; i++)
	{
		if (message[i] != '|')
		{
			aux[j] = message[i];
			j++;
		}
		else
		{
			aux[j] = '\0';
			j++;
			//printf("splitmsg: %s\n", aux);
			if (o == 0) strcpy(msg.cmd, aux);
			else if (o == 1) strcpy(msg.pid, aux);
			else if (o == 2) strcpy(msg.fifo, aux);
			else
			{
				strcpy(msg.args[a], aux);
				a++;
			}
			o++;

			j = 0;
		}
	}
	/*
	printf("msg.cmd = %s\n"
	"msg.pid = %s\n"
	"msg.fifo = %s\n"
	"msg.args[0] = %s\n"
	"msg.args[1] = %s\n"
	"msg.args[2] = %s\n"
	"msg.args[3] = %s\n", msg.cmd, msg.pid, msg.fifo, msg.args[0], msg.args[1], msg.args[2], msg.args[3]);
	*/
	return msg;
}


void speakto_server(int serverpid, char *message)
{
	printf("speak to server in %s: %s\n", SERVERFIFO, message);
	kill(serverpid, SIGUSR1);
	writefifo(SERVERFIFO, message);
}

void speakto_client(int clientpid, char *fifo, char *message)
{
	printf("speak to client in %s: %s\n", fifo, message);
	kill(clientpid, SIGUSR2);
	writefifo(fifo, message);
}

void handle_sigterm(int sig)
{
	printf("Recibida señal: %d (TERM)\n", sig);
	serverquit(0);
}

void handle_sighup(int sig)
{
	printf("Recibida señal: %d (HUP)\n", sig);
	serverquit(0);
}

void handle_sigint(int sig)
{
	printf("Recibida señal: %d (INT)\n", sig);
	serverquit(0);
}

void handle_sigusr1()  // Read fifo
{
    char buf[PATH_MAX];
	struct Fifomsg client;
	readfifo(SERVERFIFO, buf);
	client = splitmsg(buf);

	if (client.cmd[0] == 'a')
	{
		printf("El mensaje: %s\n", buf); //FIXME
	}
	else
	{
		fprintf(stderr, "Failed to read fifo, %s\n", SERVERFIFO);
		serverquit(1);
	}
	printf("El mensaje sigue siendo: %s\n", buf); //FIXME
exit(0);
/*
	switch (client.cmd[0])  // No \0 char at the end of string
	{
		case 'a': // Add to pooltab
			addpool(client.args[0], client.args[1], client.args[2], client.args[3]);
			speakto_client(atoi(client.pid), client.fifo, "OK");
			break;
		case 'r': // Remove pool from pooltab
			if (is_integer(client.args[0]))
			{
				rmpool(atoi(client.args[0]));
			}
			else
				fprintf(stderr, "This is not a valid number: %s.\n", client.args[0]);
			break;
		case 'd': // Delete snapshots
			if (is_integer(client.args[0]))
			{
				delete_snapshot(atoi(client.args[0]));
			}
			else
				fprintf(stderr, "This is not a valid number: %s.\n", client.args[0]);
			break;
		case 'p': // Print config buffer
				lspool(client.pid, client.fifo);
			break;
		case 'w': // Print config buffer
			writeconfig();
			break;
		case '?':
			serverquit(1);
			break;  // Only for prevent warnings
		default:
			printf("default: La señal es '%s', los valores: '%s' \n", client.cmd, client.pid);
			fprintf(stderr, "Received wrong value: '%s'\nMaybe too fast?\n", client.cmd);
			break;
			//abort();
	}
	*/
}

void handle_sigusr2()  // Write fifo and pull server to read clientfifo
{

	char buf[PATH_MAX];
	char *message = readfifo(clientfifo, buf);
	printf("%s\n", message);
	clientquit(0);
}

int main(int argc, char **argv)
{
	int sflag, aflag, pflag, wflag, dflag, iflag, oflag, fflag, qflag, lflag, rflag, nflag, hflag, vflag;
	sflag = aflag = pflag = wflag = dflag = iflag = oflag = fflag = qflag = lflag = rflag = nflag = hflag = vflag = 0;

	char *dvalue = NULL;  // n (config entry)
	char *ivalue = NULL;  // subvolume path 
	char *ovalue = NULL;  // directory path 
	char *fvalue = NULL;  // frequency
	char *qvalue = NULL;  // quota
	char *rvalue = NULL;  // n [...] list of snapshot numbers
	/*
	char *rvalue = NULL;

	char *def_quota = DEFQUOTA;
	char *def_subvol = DEFSUBV;
	char *def_freq = DEFFREQ;

	char *env_subvol = getenv("MAKESNAPSUBV");
	char *env_freq = getenv("MAKESNAPFREQ");
	char *env_quota = getenv("MAKESNAPQUOTA");

	int index = 0; */
	int c;

	while ((c = getopt (argc, argv, "sapwr:i:o:f:q:ld:nhv")) != -1)
		switch (c)
		{
			case 's':  // Start daemon
				sflag = 1;
				break;
			case 'a':  // Add entry to config buffer
				aflag = 1;
				break;
			case 'p':  // Print numbered config entries of buffer
				pflag = 1;
				break;
			case 'w':  // Write config buffer to CONFIGFILE
				wflag = 1;
				break;
			case 'r':  // Remove the selected n buffer entries
				rflag = 1;
				rvalue = optarg;
				break;
			case 'i':  // Set the subvolume
				iflag = 1;
				ivalue = optarg;
				break;
			case 'o':  // Set the output directory
				oflag = 1;
				ovalue = optarg;
				break;
			case 'f':  // Set the frequency
				fflag = 1;
				if (timetosecs(optarg))
					fvalue = optarg;
				break;
			case 'q':  // Set the quota
				qvalue = optarg;
				break;
			case 'l':  // Show ordered list of snapshots
				lflag = 1;
				break;
			case 'd':  // Deletes the selected n snapshots
				dflag = 1;
				dvalue = optarg;
				break;
			case 'n':  // Perform a trial run with no changes made.
				nflag = 1;
				break;
			case 'h':  // Show help
				hflag = 1;
				break;
			case 'v':  // Show program version
				vflag = 1;
				break;
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

	if(nflag)  // Dry run!
		dryrun = 1;

	if(hflag)
	{
		help(0);
	}
	else if (vflag)
	{
		version();
	}
	else if (sflag)  // Start as a daemon and reads configrc
	{
		writepid();
		mkfifo(SERVERFIFO, 0666);
		puts("Daemonize!");
		for (;;)
		{
			signal(SIGTERM, handle_sigterm);
			signal(SIGHUP, handle_sighup);
			signal(SIGINT, handle_sigint);
			signal(SIGUSR1, handle_sigusr1);  // read fifo
			sleep(60);
		}
	}
	else if (aflag) // Add to daemon
	{
		//if (sflag || pflag || wflag || dflag || lflag || rflag)
		if (pflag || wflag || dflag || lflag || dflag || rflag)
		{
			fprintf(stderr, "Too many options\n");
			return 1;
		}

		if (ivalue && ovalue && fvalue && qvalue)
		{	
			char message[100000]; 

			char mypid[8];
			sprintf(mypid, "%d", getpid());
			sprintf(clientfifo, "/tmp/makesnap.%s.fifo", mypid);

			mkfifo(clientfifo, 0666);
			pid = readpid();  // Server PID
			if (pid)  // Daemon is running
			{
				sprintf(message, "a|%s|%s|%s|%s|%s|%s|",
						mypid, clientfifo, ivalue, ovalue, fvalue, qvalue);

				speakto_server(pid, message);
				for (;;)
				{
					signal(SIGTERM, handle_sigterm);
					signal(SIGHUP, handle_sighup);
					signal(SIGINT, handle_sigint);
					signal(SIGUSR2, handle_sigusr2);  // read fifo
					sleep(60);
					printf("Still waiting to server in %s\n", clientfifo);
				}
			}
			else
			{
				fprintf(stderr, "Failed to getpid from %s\n", PIDFILE);
				return 1;
			}
		}
		else
		{
			fprintf(stderr, "Please use -i, -o, -f and -q flags\n");
			return 1;
		}

	}
	else if (rflag) // Remove config entry from buffer (no writes to cfile)
	{
		//if (sflag || aflag || pflag || wflag || lflag || rflag)
		if (pflag || wflag || lflag || dflag)
		{
			fprintf(stderr, "Too many options\n");
			return 1;
		}

		char mypid[8];
		char myfifo[1024];
		char message[1024]; 

		sprintf(mypid, "%d", getpid());
		sprintf(myfifo, "/tmp/makesnap.%s.fifo", mypid);

		pid = readpid();
		if (pid)
		{
			sprintf(message, "r|%s|%s|", mypid, rvalue);
			speakto_server(pid, message);
		}
		else
		{
			fprintf(stderr, "Failed to getpid from %s\n", PIDFILE);
			return 1;
		}
	}
	else if (dflag) // Remove config entry from buffer (no writes to cfile)
	{
		//if (sflag || aflag || pflag || wflag || lflag || rflag)
		if (pflag || wflag || lflag)
		{
			fprintf(stderr, "Too many options\n");
			return 1;
		}

		char mypid[8];
		char myfifo[1024];
		char message[1024]; 

		sprintf(mypid, "%d", getpid());
		sprintf(myfifo, "/tmp/makesnap.client%s.fifo", mypid);

		pid = readpid();
		if (pid)
		{
			sprintf(message, "d|%s|%s|", mypid, dvalue);
			speakto_server(pid, message);
		}
		else
		{
			fprintf(stderr, "Failed to getpid from %s\n", PIDFILE);
			return 1;
		}
	}
	else if (pflag) // Print config buffer
	{
		if (wflag || lflag || rflag || iflag || oflag || fflag || qflag)
		{
			fprintf(stderr, "Too many options\n");
			return 1;
		}

		char message[100000]; 

		char mypid[8];
		sprintf(mypid, "%d", getpid());
		sprintf(clientfifo, "/tmp/makesnap.client%s.fifo", mypid);

		mkfifo(clientfifo, 0666);
		pid = readpid();
		if (pid)
		{
			sprintf(message, "p|%s|%s|", mypid, clientfifo);
			speakto_server(pid, message);
		}
		else
		{
			fprintf(stderr, "Failed to getpid from %s\n", PIDFILE);
			return 1;
		}
		for (;;)
		{
			signal(SIGTERM, handle_sigterm);
			signal(SIGHUP, handle_sighup);
			signal(SIGINT, handle_sigint);
			signal(SIGUSR2, handle_sigusr2);  // read fifo
			sleep(60);
			fprintf(stderr, "Still waiting to server in %s\n", clientfifo);
		}
	}
	else if (wflag) // Write config buffer to config file
	{
		if (lflag || rflag || iflag || oflag || fflag || qflag)
		{
			fprintf(stderr, "Too many options\n");
			return 1;
		}

		char mypid[8];
		char myfifo[1024];
		char message[1024]; 

		sprintf(mypid, "%d", getpid());
		sprintf(myfifo, "/tmp/makesnap.%s.fifo", mypid);

		pid = readpid();
		if (pid)
		{
			sprintf(message, "w|%s|%s|", mypid, rvalue);
			speakto_server(pid, message);
		}
		else
		{
			fprintf(stderr, "Failed to getpid from %s\n", PIDFILE);
			return 1;
		}
	}
	else
	puts("Insert retail");
	// Insert retail

		return 0;
}

