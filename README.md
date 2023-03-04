SSTools (SnapShot tools)
========================

KISS tools for make snapshots in a Btrfs filesystem.

SSTools is made up of several small programs that work together, functioning
as one. Currently:

- ssmkd   --- System daemon
- ssmk  --- Makes snapshots only if necessary.
- sscl  --- Clean snapshots when is necessary.

Additionally, it also integrates the following tools:

- ssct  --- Shows Creation time and Changed time of a snapshot.
- ssst  --- Shows some statics of interest. (Not implemented yet)


Basic operation
---------------

On startup snpd loads `/etc/sstab` into memory and creates snapshots, that are
stored in the specified _pool_ directory. 

Time when an inode in this subvolume was last change

#### Example of `/etc/sstab` snapshot table

    # subvolume    pool               frequency    quota
 
    /              /backup/root/boot  0            30
    /              /backup/root/diary 1d           30
    /home          /backup/home/30m   30m          20


In every execution loop, if more time than indicated by the _frequency_ has
passed, a new snapshot of the _subvolume_ will be created inside of its own
_pool_, until reach the _quota_, and then, when quota were overpassed, it will
deletes the oldest snapshot until fit to the quota.


ssmkd
-----

Is the daemon. Loads `/etc/sstab` into memory and runs first `ssmk` and then
`sscl` for each line.

ssmk
-----
Creates a snapshot of the subvolume in the specified directory, only if the
minimum frequency time is met. And only if there have been changes. The
snapshot name is automatically set in the format **`+%Y-%m-%d_%H-%M-%S`**.


sscl
-----

Delete the oldest snapshots until the quota is met.

ssct
-----

Shows when subvolume was created and when an inode in the subvolume was last
change.

ssst
-----

Show snapshot statistics (Not implemented yet).

---
_TODO_
-----

- System calls.
