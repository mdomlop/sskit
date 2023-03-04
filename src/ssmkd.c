#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() */
#include <linux/limits.h>  /* for PATH_MAX */
#include <unistd.h>  /* for getpid() */
#include <signal.h>


#define PROGRAM      "SnaphotDaemon"
#define EXECUTABLE   "ssmkd"
#define DESCRIPTION  "Daemon for making snapshots in a Btrfs filesystem."
#define PKGNAME      "sstools"
#define VERSION      "0.2a"
#define URL          "https://github.com/mdomlop/sstools"
#define LICENSE      "GPLv3+"
#define AUTHOR       "Manuel Domínguez López"
#define NICK         "mdomlop"
#define MAIL         "zqbzybc@tznvy.pbz"

#define CONFIGFILE "/etc/sstab"
#define PIDFILE "/tmp/ssmkd.pid"

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
	char cmd_mk[PATH_MAX];
	char cmd_cl[PATH_MAX];

	strcpy(cmd_mk, "ssmk -i ");
	strcat(cmd_mk, subv);
	strcat(cmd_mk, " -o ");
	strcat(cmd_mk, pool);
	strcat(cmd_mk, " -f ");
	strcat(cmd_mk, freq);

	//printf("\n%s\n", cmd_mk);
	system(cmd_mk);

	sleep(1);  // A short breath. Maybe not necessary

	strcpy(cmd_cl, "sscl -p ");
	strcat(cmd_cl, pool);
	strcat(cmd_cl, " -q ");
	strcat(cmd_cl, quota);

	//printf("\n%s\n", cmd_cl);
	system(cmd_cl);
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
	printf("Received signal: %d (TERM)\n", sig);
	quit(0);
}

void handle_sighup(int sig)  // FIXME: Stop the program execution
{
	printf("Received signal: %d (HUP)\n", sig);
	puts("Reloading config... Not implemented.");
	loadconfig();
	printconfig();
	puts("Done!");
}

void handle_sigint(int sig)
{
	printf("Received signal: %d (INT)\n", sig);
	quit(0);
}

void handle_sigusr1(int sig)  // TODO: Change verbosity
{
	printf("Received signal: %d (USR1)\n", sig);
	puts("Changing verbosity... Not implemented.");
}
void handle_sigusr2(int sig)  // Show statics
{
	printf("Received signal: %d (USR2)\n", sig);
	puts("Show statics... Not implemented.");
	printconfig();
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

