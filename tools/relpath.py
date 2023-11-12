#!/usr/bin/python

import os
import sys

argc = len(sys.argv)
if (argc != 2) and (argc != 3):
    sys.exit('Usage: tools/relpath.py dir1 OR tools/relpath.py dir1 dir2')

print(os.path.relpath(sys.argv[1], sys.argv[2] if argc >= 3 else None))
