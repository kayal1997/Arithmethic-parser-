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
  # print(f'o:{x}')
  # x = re.sub("Hz", "", x)
  if bool(re.findall(r"[yzafEYZ]", x)):
        pass
        #print( "Invalid" )
  else:
        # # Seperate by ()

        for op in ['+','-','/','*']:
            if op in x:
                x = x.split(op)
                x = op.join(['('+s.replace('\n','')+')' if not (s.startswith('(') and s.endswith(')')) else s.replace('\n','') for s in x ])
        x= x.replace('()','')
        print(x)
        x= x.replace('(((','(')
        x= x.replace(')))',')')
        print('x==',x)