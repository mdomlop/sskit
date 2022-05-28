#!/usr/bin/env python3

import setproctitle
import Snapman.functions
import Snapman.args
from Snapman.quit import *  # include os, sys


def main():
    if Snapman.args.pidfile:
        Snapman.functions.pidfile()
    if Snapman.args.sample:
        Snapman.functions.sample()
    elif Snapman.args.help:
        Snapman.functions.usage()
    elif Snapman.args.version:
        Snapman.functions.version()
    elif Snapman.args.sections:
        if not Snapman.args.command \
        or Snapman.args.command in Snapman.args.list_arg:
            Snapman.functions.sections()
        elif Snapman.args.command in Snapman.args.snapshot_arg:
            Snapman.functions.section_snapshots()
        elif Snapman.args.command in Snapman.args.new_arg:
            Snapman.functions.section_newsnapshot()
        elif Snapman.args.command in Snapman.args.clean_arg:
            Snapman.functions.section_clean()
        elif Snapman.args.command in Snapman.args.info_arg:
            Snapman.functions.command in Snapman.args.info()
        elif Snapman.args.command in Snapman.args.prop_arg:
            Snapman.functions.section_properties()
        else:
            clean_exit('Invalid command for section option: ' +
                        Snapman.args.command +                     
                        '\nValid commands are: ' +
                        'list snapshots newsnapshot clean' +
                        'information properties',
                        BADCOM)                   
    elif Snapman.args.subvolumes:
        if not Snapman.args.command \
        or Snapman.args.command in Snapman.args.list_arg:
            Snapman.functions.subvolumes()
        elif Snapman.args.command in Snapman.args.snapshot_arg:
            Snapman.functions.subvolume_snapshots()
        elif Snapman.args.command in Snapman.args.section_arg:
            Snapman.functions.subvolume_sections()
        elif Snapman.args.command in Snapman.args.clean_arg:
            Snapman.functions.subvolume_clean()
        elif Snapman.args.command in Snapman.args.info_arg:
            Snapman.functions.subvolume_info()
        else:
            clean_exit('Invalid command for subvolume option: ' +
                        Snapman.args.command +                     
                        '\nValid commands are: ' +
                        'list snapshots sections clean information',
                        BADCOM)                   
    elif Snapman.args.snapshots:
        if not Snapman.args.command \
        or Snapman.args.command in Snapman.args.list_arg:
            Snapman.functions.snapshots()
        elif Snapman.args.command in Snapman.args.info_arg:
            Snapman.functions.snapshot_info()
        else:
            clean_exit('Invalid command for snapshot option: ' +
                        Snapman.args.command +                     
                        '\nValid commands are: list information',
                        BADCOM)                   
    elif Snapman.args.gui:
        Snapman.functions.gui()
    elif Snapman.args.daemon:
        Snapman.functions.daemon()
    else:
        Snapman.functions.run_once()


if __name__ == '__main__':
    setproctitle.setproctitle(' '.join(sys.argv))
    main()
    clean_exit()
