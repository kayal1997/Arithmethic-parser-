# -*- coding: utf-8 -*-
"""
Created on Thu Apr 22 22:47:44 2021
"""

file = open("myfile.txt", "r")
for line in file:
 # print(line)
  import re
  line = re.sub("hz", "", line)
  line = re.sub("Hz", "", line)
  print(line)
