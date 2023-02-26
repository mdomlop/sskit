MakeSnap
========

KISS tool for make snapshots in a Btrfs filesystem.


Snapshots will be stored in [_subvolume/_]`.snapshots` directory. Here,
snapshots are named with this timestamp format: **YYYY-MM-DD_hh.mm.ss**


For simplicity, Makesnap has no configuration file, only a not interactive 
command line. It is designed for working with other tools like `crond` or
`systemd-timer`.

Usage:

- Client/Server mode:
~~~
			makesnap -s [configrc]
			makesnap -a -i subvolume -o directory -f freq -q quota
			makesnap -p
			makesnap -w
			makesnap -d entryn
~~~

- Standalone mode:

~~~
			makesnap -l [subvolume] [pool]
			makesnap -c subvolume [pool]
			makesnap -r snapshotn
			makesnap -f subvolume pool
	
			makesnap -h
			makesanp -v
~~~


With no options, the program will try to use the environment variables
`MAKESNAPSUBVOLUME` and `MAKESNAPQUOTA`, and if someone are empty, defaults to
harcoded values, which are `"/"`, and `30`, for _subvolume_ and _quota_
respectively.

In every execution a new snapshot of the _subvolume_ will be created until
reach the _quota_, and then, when quota were overpassed, will delete the oldest
snapshot in `subvolume/.snapshots/` until fit to the quota.

`.snapshots` directory will be created if it not exists and will be removed if
all the snapshots are deleted.

- Options:

	`subvolume`

	Path to subvolume that will be snapshotted.  If not supplied, the program
	will try to use the environment variable `MAKESNAPSUBVOLUME`, and if empty,
	defaults to harcoded value, which is `"/"`,

	If such path is not a btrfs subvolume, an error message will be
	displayed, and no snapshot will be created.

	`-R`

	Clean all snapshots in the [_subvolume/_]`.snapshots` directory. Then also
	deletes such directory.
	
	`-l`
	
	Show a numbered list of snapshots of the subvolume. 

	The number in front of each item is for use with the `-d`option.
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

	Deletes selected snapshot *n*. Check the `-l` option to obtain *n*.
	
	`-S` *time*
	
	Runs as a service (_daemon_) and make snapshots every *time*
