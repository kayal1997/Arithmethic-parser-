# -*- coding: utf-8 -*-
"""
Created on Thu Apr 22 22:47:44 2021

@author: mural
"""
import re
f = open("D:\Arithmethic-parser-\python_preprocessing_script\myfile.txt", "r")
for x in f:
  print("************************************")
  print(x)
  x = re.sub(r"hz||Hz", "", x)
  if bool(re.findall(r"[yzafEYZ]", x)):
    pass
  else:
    opr = re.findall(r'[*+-/]',x)
    var = re.split(r"[*+-/]", x)
    s = ""
    # print(f'var:{var},opt:{opr}')
    if opr != [] or opr != ['.'] :
        for i in range (len(opr)):
            if opr[i] == '.':
                s=s+"("+var[i]+opr[i]
            elif s!="" and s[-1]==".":
                s=s+var[i]+")"+opr[i]
            else:
                s= s+"("+var[i]+")"+opr[i]
    eqn = s+"("+var[-1][:-1]+")"
    eqn = eqn.replace('e)-(','e-')
    eqn= eqn.replace('.(','.')
    eqn= eqn.replace('.(','.')
    eqn= eqn.replace('()','')
    print(f'sub:{eqn}')
   