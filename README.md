MakeSnap
========

KISS tools for make snapshots in a Btrfs filesystem.

On startup MakeSnap loads `/etc/sstab` into memory and creates snapshots, that are stored in the specified _pool_ directory. 

#### Ejemplo de `/etc/sstab`

    # subvolume    pool               frequency    quota
 
    /              /backup/root/boot  0            30
    /              /backup/root/diary 1d           30
    /home          /backup/home/30m   30m          20


In every execution cicle, if more time than indicated by the _frequency_ has passed, a new snapshot of the _subvolume_ will be created inside of its own _pool_, until reach the _quota_, and then, when quota were overpassed, it will deletes the oldest snapshot until fit to the quota.

MakeSnap is made up of several programs that work together, functioning as one.

mksnpd
------

Is the daemon. Loads `/etc/sstab` into memory and runs first `mksnp` and then `clsnp` for each line.

mksnp
-----
Creates a snapshot of the subvolume in the specified directory, only if the minimum frequency time is met. And only if there have been changes. The snapshot name is automatically set in the format **`+%Y-%m-%d_%H-%M-%S`**.


clsnp
-----

Delete the oldest snapshots until the quota is met.

stsnp
-----

Show snapshot statistics (Not implemented yet).

---
_TODO_
-----

- System calls.