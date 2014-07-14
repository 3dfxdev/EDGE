#!/bin/bash
sh-elf-addr2line -s -p -f -i -e $1 < $2 | sed 'N;s/:[0-9]*//;P;D' | sort -n | uniq -c | sort -n -r | less
