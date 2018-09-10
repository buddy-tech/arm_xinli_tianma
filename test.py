#!/usr/bin/env python3

import sys
import commands

print(sys.argv[0])
for i in range(1, len(sys.argv)):
    print( "param", i, sys.argv[i])

(status,outputinfo)=commands.getstatusoutput('ls /bin/ls')

print(status)
print(outputinfo)

