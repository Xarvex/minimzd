#!/usr/bin/python

import os
import sys

if (len(sys.argv) != 2):
    sys.exit('Usage: tools/abspath.py dir1')

print(os.path.abspath(sys.argv[1]))
