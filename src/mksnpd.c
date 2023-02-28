#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() */
#include <linux/limits.h>  /* for PATH_MAX */
#include <unistd.h>  /* for getpid() */
#include <signal.h>


#define PROGRAM      "MakeSnapD"
#define EXECUTABLE   "mksnpd"
#define DESCRIPTION  "Daemon for making snapshots in a Btrfs filesystem."
#define PKGNAME      "makesnap"
#define VERSION      "0.1a"
#define URL          "https://github.com/mdomlop/makesnap"
#define LICENSE      "GPLv3+"
#define AUTHOR       "Manuel Domínguez López"
#define NICK         "mdomlop"
#define MAIL         "zqbzybc@tznvy.pbz"

#define CONFIGFILE "/etc/sstab"
#define PIDFILE "/tmp/mksnpd.pid"

/* Btrfs has not this limit, but I think this value is very safe. */
#define SNAPLISTSIZE 64000

int initboot = 1;  // For executing only once (on application start).
int sleepsecs = 5;  // Sleep time for daemon.

/* List of entries for automatic snapshotting */
char configtab[SNAPLISTSIZE][PATH_MAX];
int configtab_c = 0;  // Number of elements in configtab


void quit(int n)
{
	unlink(PIDFILE);
	exit(n);
}

int loadconfig(void)
{
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

				if (buffer[0] == '/')  // All lines must start with / [...]
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
	else
	{
		fprintf(stderr, "Fatal error: "
		"Config file not found: %s\n", CONFIGFILE);
		quit(1);
	}
	// No fail here.

	configtab_c = tab_index;

	/* Number of elements in config. Then: configtab_c == loadconfig() */
	return tab_index;
}

void printconfig(void)
{
	for (int i = 0; i < configtab_c; i++)
		printf("%s\n", configtab[i]);
}


void commands(char *subv, char *pool, char *freq, char *quota)
{
	char cmd_mksnap[PATH_MAX];
	char cmd_clsnap[PATH_MAX];

	strcpy(cmd_mksnap, "mksnp -i ");
	strcat(cmd_mksnap, subv);
	strcat(cmd_mksnap, " -o ");
	strcat(cmd_mksnap, pool);
	strcat(cmd_mksnap, " -f ");
	strcat(cmd_mksnap, freq);

	//printf("\n%s\n", cmd_mksnap);
	system(cmd_mksnap);

	sleep(1);  // A short breath. Maybe not necessary

	strcpy(cmd_clsnap, "clsnp -p ");
	strcat(cmd_clsnap, pool);
	strcat(cmd_clsnap, " -q ");
	strcat(cmd_clsnap, quota);

	//printf("\n%s\n", cmd_clsnap);
	system(cmd_clsnap);
}

void runconfig(void)
{
	char subvol[PATH_MAX];
	char pool[PATH_MAX];
	char freq[8];
	char quota[8];
	char delim[] = " ";
	char *ptr;

	/* Because strok() is destructive, better operate with a copy */
	char auxtab[configtab_c][PATH_MAX];
	for (int i = 0; i < configtab_c; i++)  // make a copy
		strcpy(auxtab[i], configtab[i]);

	for (int i = 0; i < configtab_c; i++)
	{
		ptr = strtok(auxtab[i], delim);

		/* Get first field: subvolume */
		if (ptr != NULL)
		{
			strcpy(subvol, ptr);
			ptr = strtok(NULL, delim);
		}
		else
		{
			fprintf(stderr, "Fatal error: "
			"Can not get subvolume name from config file: %s\n", CONFIGFILE);
			quit(1);
		}

		/* Get the pool */
		if (ptr != NULL)
		{
			strcpy(pool, ptr);
			ptr = strtok(NULL, delim);
		}
		else
		{
			fprintf(stderr, "Fatal error: "
			"Can not get pool name from config file: %s\n", CONFIGFILE);
			quit(1);
		}

		/* Get the frequency */
		if (ptr != NULL)
		{
			strcpy(freq, ptr);
			ptr = strtok(NULL, delim);
		}
		else
		{
			fprintf(stderr, "Fatal error: "
			"Can not get frequency name from config file: %s\n", CONFIGFILE);
			quit(1);
		}

		/* Get the quota */
		if (ptr != NULL)
		{
			strcpy(quota, ptr);
			ptr = strtok(NULL, delim);
		}
		else
		{
			fprintf(stderr, "Fatal error: "
			"Can not get quota name from config file: %s\n", CONFIGFILE);
			quit(1);
		}

		/* Only execute once if freq is 0 */
		if (atoi(freq) == 0)
		{
			if (initboot)  // We are on first execution cicle
			{
				commands(subvol, pool, freq, quota);
			}
		}
		else
		{
			commands(subvol, pool, freq, quota);
		}
	}
	initboot = 0;  // Switch it to 1 for skip 0-numbered freqs
}

int writepid(void)
{
	char *pidfile = PIDFILE;
	FILE *fd = fopen(pidfile, "w");

	fprintf(fd, "%d", getpid());
	fclose(fd);
	return 0;
}


void handle_sigterm(int sig)
{
	printf("Recibida señal: %d (TERM)\n", sig);
	quit(0);
}

void handle_sighup(int sig)  // FIXME: Stop the program execution
{
	printf("Recibida señal: %d (HUP)\n", sig);
	puts("Reloading config...");
	loadconfig();
	puts("Done!");
}

void handle_sigint(int sig)
{
	printf("Recibida señal: %d (INT)\n", sig);
	quit(0);
}

void handle_sigusr1(int sig)  // TODO: Change verbosity
{
	printf("Recibida señal: %d (USR1)\n", sig);
}
void handle_sigusr2(int sig)  // Write fifo and pull server to read clientfifo
{
	printf("Recibida señal: %d (USR2)\n", sig);
}


int main(void)
{
	if (loadconfig() > 0)  // More than 0 lines loaded from config.
	{
		writepid();

		/* Daemonizing */
		for (;;)
		{
			signal(SIGTERM, handle_sigterm);
			signal(SIGHUP, handle_sighup);  // reload config: loadconfig()
			signal(SIGINT, handle_sigint);
			signal(SIGUSR1, handle_sigusr1);  // Change verbosity
			signal(SIGUSR2, handle_sigusr2);  // Show some statics?

			runconfig();
			sleep(sleepsecs);
		}
	}

	quit(0);
}

