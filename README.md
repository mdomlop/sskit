MakeSnap
========

KISS tool for make snapshots in a Btrfs filesystem.

- Usage:

~~~
	makesnap [options]
~~~

- Options:

	`-o --origin` */dir/a*

	Original subvolume. If not supplied, current directory will be used.
	If current directory is not a btrfs subvolume, an error message will be
	displayed, and no snapshot will be created.

	`-d --destination` */dir/b* 

	Destination directory for storing snapshots. If not supplied, 
	defaults to *origin*/`.snapshots`.
	
	Here snapshots are named with this timestamp format: **YYYY-MM-DD_hh.mm.ss**

	`-a --age` *5[s]*

	Maximum difference in time from the las snapshot were taken and the
	pretended new one. If such difference is minor, no snapshot will be taken.
	
	Suffix indicates time scale. They can be:

	- `s` for seconds (you can omit this one).
	- `m` for minutes.
	- `h` for hours.
	- `d` for days.

	`-m --maximum` *30*
	
	The maximum number of snapshots allowed.If supplied number is less than 
	the number of snapshots in *destination* directory, the remaining ones
	will be deleted, starting with the oldest.

	`-n --needed`

	Checks if there are any differences between the source subvolume and 
	the latest snapshot. If there is none, the intended snapshot will not 
	be created.

	`-r --reaonly`

	Make readonly snapshot.

	`-l --list`
	
	List all snapshots of source subvolume in destination directory.

	`-L --listnumbered`

	Same as before, but add a number in front of each item. This is needed 
	for `--restore` option.

	`-R --restore`

	Restore source subvolume from selected snapshot in *destination*

	`-d --delete`

	Deletes selected snapshot.
