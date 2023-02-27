#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() */
#include <linux/limits.h>  /* for PATH_MAX */
#include <unistd.h>  /* for getpid() */
#include <signal.h>


#define PROGRAM     "MakeSnapD"
#define EXECUTABLE  "mksnpd"
#define DESCRIPTION "Daemon for making snapshots in a Btrfs filesystem."
#define VERSION     "0.1a"
#define URL         "https://github.com/mdomlop/mksnp"
#define LICENSE     "GPLv3+"
#define AUTHOR      "Manuel Domínguez López"
#define NICK        "mdomlop"
#define MAIL        "zqbzybc@tznvy.pbz"

#define CONFIGFILE "/etc/sstab"
//#define CONFIGFILE "/home/mdl/Documentos/Programas/github/makesnap/src/sstab.test"
#define SNAPLISTSIZE 64000  /* Btrfs has not this limit, but I think this value is very safe. */
#define PIDFILE "/tmp/mksnpd.pid"

int init = 0;  // For executing only once (on application start).
int sleepsecs = 5;  // Sleep time for daemon.

char configtab[SNAPLISTSIZE][PATH_MAX];  // List of entries for automatic snapshotting
int configtab_c = 0;  // Number of elements in configtab


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
		fprintf(stderr, "Config file not found: %s\n", CONFIGFILE);
		exit(1);
	}
	// No fail here.

	configtab_c = tab_index;

	return tab_index;  // Number of elements in config. configtab_c = loadconfig();
}

void printconfig(void)
{
	for (int i = 0; i < configtab_c; i++)
		printf("%s\n", configtab[i]);
}

void execconfig(void)
{
	char cmd_mksnap[PATH_MAX];
	char cmd_clsnap[PATH_MAX];

	char auxtab[configtab_c][PATH_MAX];  // List of entries for automatic snapshotting

	// make a copy
	for (int i = 0; i < configtab_c; i++)
	{
		strcpy(auxtab[i], configtab[i]);
	}

	for (int i = 0; i < configtab_c; i++)
	{
		char subvol[PATH_MAX];
		char pool[PATH_MAX];
		char freq[8];
		char  quota[8];

		char delim[] = " ";
		char *ptr = strtok(auxtab[i], delim);

		if (ptr != NULL)
		{
			strcpy(subvol, ptr);
			ptr = strtok(NULL, delim);
		} else { exit(1); }

		if (ptr != NULL)
		{
			strcpy(pool, ptr);
			ptr = strtok(NULL, delim);
		} else { exit(1); }

		if (ptr != NULL)
		{
			strcpy(freq, ptr);
			ptr = strtok(NULL, delim);
		} else { exit(1); }

		if (ptr != NULL)
		{
			strcpy(quota, ptr);
			ptr = strtok(NULL, delim);
		} else { exit(1); }

		/* Only execute once if freq is 0 */
		if ((init != 0) && (atoi(freq) == 0))
			continue;
		else
		{
			strcpy(cmd_mksnap, "mksnp -i ");
			strcat(cmd_mksnap, subvol);
			strcat(cmd_mksnap, " -o ");
			strcat(cmd_mksnap, pool);
			strcat(cmd_mksnap, " -f ");
			strcat(cmd_mksnap, freq);

			//printf("\n%s\n", cmd_mksnap);
			system(cmd_mksnap);

			sleep(1);

			strcpy(cmd_clsnap, "clsnp -p ");
			strcat(cmd_clsnap, pool);
			strcat(cmd_clsnap, " -q ");
			strcat(cmd_clsnap, quota);

			//printf("\n%s\n", cmd_clsnap);
			system(cmd_clsnap);
		}

		init = 1;  // Switch it to 1 for skip 0 freqs
	}
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
	unlink(PIDFILE);
	exit(0);
}

void handle_sighup(int sig)
{
	printf("Recibida señal: %d (HUP)\n", sig);
	puts("Reloading config...");
	loadconfig();
	puts("Done!");
}

void handle_sigint(int sig)
{
	printf("Recibida señal: %d (INT)\n", sig);
	unlink(PIDFILE);
	exit(0);
}

void handle_sigusr1(int sig)  // Write fifo and pull server to read clientfifo
{
	printf("Recibida señal: %d (USR1)\n", sig);
}
void handle_sigusr2(int sig)  // Write fifo and pull server to read clientfifo
{
	printf("Recibida señal: %d (USR2)\n", sig);
}

int main(void)
{
	loadconfig();
	// printconfig();

	writepid();
	//puts("Daemonize!");
	//printf("PID is: %d\n", getpid());
	for (;;)
	{
		signal(SIGTERM, handle_sigterm);
		signal(SIGHUP, handle_sighup);  // reload config: loadconfig()
		signal(SIGINT, handle_sigint);
		signal(SIGUSR1, handle_sigusr1);  // read fifo
		signal(SIGUSR2, handle_sigusr2);  // read fifo

		execconfig();
		sleep(sleepsecs);
	}

	unlink(PIDFILE);
	return 0;
}

