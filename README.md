MakeSnap
========

KISS tool for make snapshots in a Btrfs filesystem.


Snapshots will be stored in [_subvolume/_]`.snapshots` directory. Here snapshots
are named with this timestamp format: **YYYY-MM-DD_hh.mm.ss**


For simplicity, Makesnap has no configuration file, only a non interactive 
command line. It is designed for working with other tools like `crond` or
`systemd-timer`.

- Usage:

~~~
	makesnap [-cflhv] [-q quota] [-r restore] [-d delete]  [subvolume]
~~~


With no options, the program will trie to use the current working directory 
as a subvolume. If it is not a btrfs subvolume, program will exit. Otherwise,
will make a new snapshot under `./subvolume/`, and if quota were overpassed, 
will delete the older subvolume. `./subvolume` directory will be created
if it not exists.


- Options:

	`subvolume`

	Path to subvolume. If not supplied, current directory will be used.
	If current directory is not a btrfs subvolume, an error message will be
	displayed, and no snapshot will be created.

	`-c`

	Clean all snapshots in the [_subvolume/_]`.snapshots` directory. Then also
	deletes such directory.
	
	`-f`
	
	Forces creation of the snapshot. Regardless of the value of the option `-a`.
	
	`-l`
	
	Show a numbered list of snapshots of the subvolume. 

	The number in front of each item is for use with the `-r` or the `-d`option.
	The higher number, the newest snapshot. Ever *0* will be the oldest.
	
	`-h`
	
	Show a short help and exit.
	
	`-v`
	
	Show program version and exit.

	`-q` *30*

	The maximum quota number of snapshots allowed. If supplied number is less
	than the number of snapshots in *destination* directory, the remaining ones
	will be deleted, starting with the oldest.
	
	If no quota is provided, defaults to environment variable MAKESNAPQUOTA,
	and, if such variable is empty, defaults to harcoded value `30'.

	`-r` *n*

	Restore source subvolume from selected snapshot *n*. Check the `-l` option
	to obtain *n*.

	`-d --delete` *n*

	Deletes selected snapshot *n*. Check the `-l` option to obtain *n*.
