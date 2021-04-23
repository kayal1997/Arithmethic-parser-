# -*- coding: utf-8 -*-
"""
Created on Thu Apr 22 22:47:44 2021

@author: mural
"""
import re
f = open("D:\Arithmethic-parser-\python_script_preprocessing\myfileinputtypes.txt", "r")
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
    
    
import xlwt
from xlwt import Workbook
  
# Workbook is created
wb = Workbook()
  
# add_sheet is used to create sheet.
sheet1 = wb.add_sheet('Sheet 1')
  
sheet1.write(1, 0, 'ISBT DEHRADUN')
sheet1.write(2, 0, 'SHASTRADHARA')
sheet1.write(3, 0, 'CLEMEN TOWN')
sheet1.write(4, 0, 'RAJPUR ROAD')
sheet1.write(5, 0, 'CLOCK TOWER')
sheet1.write(0, 1, 'ISBT DEHRADUN')
sheet1.write(0, 2, 'SHASTRADHARA')
sheet1.write(0, 3, 'CLEMEN TOWN')
sheet1.write(0, 4, 'RAJPUR ROAD')
sheet1.write(0, 5, 'CLOCK TOWER')
  
wb.save('xlwt example.xls')
   