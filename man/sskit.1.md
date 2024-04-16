---
title: SSKIT
section: 1
header: User Manual
footer: sskit 0.8.3b
date: 2024-04-16
---


# NAME

sskit - KISS tools for make snapshots in a Btrfs filesystem

# SYNOPSYS

**ssmk** [*-vh*] *-s* subvolume *-p* pool *-f* frequency

**sscl** [*-vh*] *-p* pool *-q* quota

**sskd** [*-vh*]

**ssct** [*-vh*] path


# DESCRIPTION

SSKit is made up of several small programs that work together, functioning
as one.

With the `ssmk` and `sscl` tools you can maintain a pool of snapshots of your
subvolumes. `ssmk` will create a snapshot in the pool directory if the given
frequency is met and `sscl`, for its part, will delete the oldest snapshots
that exceed the given quota.


## Basic operation

To do this you can use the classic method, which involves using the standard
`cron` program, or the one provided by this kit, which makes use of the
specially dedicated tool `sskd`.

### /etc/crontab

If you want to use `crond` program, add to `/etc/crontab` some lines
like these:

        # Making snapshots:
        @reboot   /usr/sbin/ssmk -s / -p /backup/root/boot    -f 0
        * * * * * /usr/sbin/ssmk -s / -p /backup/root/diary   -f 1d
        * * * * * /usr/sbin/ssmk -s /home -p /backup/home/30m -f 30m
        # Cleaning pools:
        @reboot   /usr/sbin/sscl -p /backup/root/boot  -q 30
        * * * * * /usr/sbin/sscl -p /backup/root/diary -q 30
        * * * * * /usr/sbin/sscl -p /backup/home/30m   -q 20

See cron(8) and crontab(5) for details.

As already said, alternatively to the _crond method_, `sskit` provides
a simpler tool especially dedicated to this, `sskd`, which makes use
of `/etc/sstab`.

### `/etc/sstab`

`sskit` provides a simple table named `/etc/sstab`, which is used as
configuration file when `sskd` runs.

The configuration file defines all the stuff about
what and how the snapshots of subvolumes are taked.
Read  more about it in sstab(5).

For use `sskd` instead of `cron`, edit `/etc/sstab`:

    # subvolume    pool               frequency    quota

    /              /backup/root/boot  0            30
    /              /backup/root/diary 1d           30
    /home          /backup/home/30m   30m          20

This example is functionally equivalent to the previous `cron` example.

In every execution loop, if more time than indicated by the
_frequency_ has passed, a new snapshot of the _subvolume_ will be
created inside of its own _pool_, until reach the _quota_, and then,
when quota were overpassed, it will deletes the oldest snapshot until
fit to the quota.


Edit `/etc/sstab` at your preferences and then start and enable
the proper service for your init system. `sskit` include some
services files, for example:

- System V: `/etc/init.d/sskd start`

- Systemd: `systemctl start sskd`

- Dinit: `dinitctl start sskd`

On startup **sskd** will load `/etc/sstab` into memory and
periodically creates or deletes snapshots in the specified _pool_
directory.

## Currently main tools

- _ssmk_: For making snapshots.
- _sscl_: For cleaning snapshots.
- _sskd_: Snapshot tools daemon.

## Additional tools

Additionally, it also integrates the following tools:

- _ssct_: Shows creation and changed time a snapshot.

## Not implemented yet tools

- _ssst_: Shows some statics of interest.
- _ssgui_: Graphic user interface.



# INCLUDED TOOLS

## sskd

Is the daemon. Loads `/etc/sstab` into memory and starts an infinite
loop where firstly runs `ssmk` and then `sscl` for each line of such file.

By default the loop repeats itself every 5 seconds, but it can be changed
setting a _period_ in command line.

    Usage:
            sskd [-h] [-v] [period]

    Options:
            period       Time in seconds for loop repeat. Defaults to 5.

            -h           Show this help and exit.
            -v           Show program version and exit.


## ssmk

Takes a source subvolume and creates a snapshot and a destination
directory (called pool), where previous snapshots were stored.

Checks if the last snapshot in such pool is enough old and if there
are differences with source subvolume, and then takes a new snapshot.

The snapshot name is automatically set in the `date` format
**`+%Y-%m-%d_%H-%M-%S`**.

    Usage:
            ssmk [-h] [-v] -s subvolume -p pool -f freq

    Options:
            -s subvolume Set the source subvolume.
            -p pool      Set the destination pool.
            -f freq      Set the frequency.

            -h           Show this help and exit.
            -v           Show program version and exit.

## sscl

Takes a pool directory and checks if there are more snapshots than
indicated as maximum by the quota. If then, deletes the oldest snapshots until
the quota is met.

    Usage:
            sscl [-h] [-v] -p pool -q quota

    Options:
            -p dir       Set the pool directory.
            -q quota     Set the quota.

            -h           Show this help and exit.
            -v           Show program version and exit.

## ssct

Shows when subvolume was created and when an inode in the subvolume
was last change.

    Usage:
            ssct [-h] [-v] path

    Options:
            path         Path to subvolume.

            -h           Show this help and exit.
            -v           Show program version and exit.

# NOT YET IMPLEMENTED/INCLUDED TOOLS

## ssst

Show snapshot statistics.

## ssgui

Graphic user interface.

---

# _TODO_

- Signals handling.

# SEE ALSO

sstab(5)

# AUTHOR

Manuel Domínguez López <mdomlopatgmaildotcom>

# COPYRIGHT

GPLv3
