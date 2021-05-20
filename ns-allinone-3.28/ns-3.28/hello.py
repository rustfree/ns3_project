#!/usr/bin/env python
print "hello world"

name = raw_input("what is your name?")
print 'hello,'+name +'!'

import os
os.system('./waf --run scratch/scratch-simulator')
