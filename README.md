SSKit (SnapShot Kit)
====================

KISS tools for make snapshots in a Btrfs filesystem.

SSKit is made up of several small programs that work together, functioning
as one. Currently:

- sskd: Daemon for making snapshots.
- ssmk: Makes snapshots only if necessary.
- sscl: Clean snapshots when is necessary.

Additionally, it also integrates the following tools:

- ssct: Shows Creation time and Changed time of a snapshot.
- ssst: Shows some statics of interest. (Not implemented yet)
- ssgui: Graphic user interface. (Not implemented yet)


Basic operation
---------------

On startup sskd loads `/etc/sstab` into memory and creates snapshots, that are
stored in the specified _pool_ directory.

#### Example of `/etc/sstab`

    # subvolume    pool               frequency    quota

    /              /backup/root/boot  0            30
    /              /backup/root/diary 1d           30
    /home          /backup/home/30m   30m          20


In every execution loop, if more time than indicated by the _frequency_ has
passed, a new snapshot of the _subvolume_ will be created inside of its own
_pool_, until reach the _quota_, and then, when quota were overpassed, it will
deletes the oldest snapshot until fit to the quota.

Included commands
-----------------

### sskd

Is the daemon. Loads `/etc/sstab` into memory and runs first `ssmk` and then
`sscl` for each line.

### ssmk

Creates a snapshot of the subvolume in the specified directory, only if the
minimum frequency time is met. And only if there have been changes. The
snapshot name is automatically set in the format **`+%Y-%m-%d_%H-%M-%S`**.


### sscl

Delete the oldest snapshots until the quota is met.

### ssct

Shows when subvolume was created and when an inode in the subvolume was last
change.

### ssst

Show snapshot statistics (_not implemented yet_).

---

Compilation
-----------

Yout need libbtrfsutil. It is the only dependency.

For install in Debian 13 run:

	apt install libbtrfsutil-dev

Other distributions may have a similar name for it.

### Installation

Once you have it in your system, run:

	make && make install

### Packaging

For Debian GNU/Linux:

	make pkg_debian

For Arch Linux:

	make pkg_arch

_TODO_
------

- System calls.

Compilation
See [changelog](https://github.com/mdomlop/sskit/blob/main/changelog.md)
