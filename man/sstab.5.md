---
title: SSTAB
section: 5
header: User Manual
footer: sskit 0.6b
date: March 04, 2023
---


# NAME

sstab - Configuration file of sskit.

# LOCATION
	`/etc/sstab`


# DESCRIPTION

Snapshots table (`/etc/sstab`) is part of sskit, but
its siple format allow that can be used for other
utilities.

When  you  run `sstd` it will read this file and then
it will make backups (snapshots) of indicated
subvolumes at desired frequency until reach a defined
quota. I  quota were  reached,  then  it will remove
the older backups to keep the number of backups
indicated in quota.

The configuration file is mandatory. If there is not such file, the program will fail.


# FORMAT

Only lines beginning with '`/`' character will be
processed. This means that any line that does not
start with / will be ignored.

First field is the source btrfs _subvolume_, the
second one is the _pool_, a directory where snapshots
will be stored.

Third field is the _frequency_ in which snapshots will
be captured and stored. If this is _0_, means that
only once (on program start) will be checked. Append s, m, h, d or y to say seconds, minutes, hours, days
or years. If no character is appened, then means
seconds.

And fourth field, _quota_, is how many snapshots will
be stored in pool. When this number was reached, the
olders snapshots will be deleted until the total number of snapshots does not exceed the quota.

# EXAMPLE:


    # subvolume    pool               frequency    quota
 
    /              /backup/root/boot  0            30
    /              /backup/root/diary 1d           30
    /home          /backup/home/30m   30m          20



# SEE ALSO

sskit(1)
	
# AUTHOR

Manuel Domínguez López <mdomlopatgmaildotcom>
	
# COPYRIGHT

GPLv3
