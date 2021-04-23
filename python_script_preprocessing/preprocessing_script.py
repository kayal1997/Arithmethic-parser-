# -*- coding: utf-8 -*-
"""
Created on Thu Apr 22 22:47:44 2021

@author: mural
"""
import xlwt
from xlwt import Workbook
import xlrd
from xlutils.copy import copy
import re

# Give the location of the file
loc = ("D:/Arithmethic-parser-/python_script_preprocessing/Inputtypesfilesspreadsheet.xlsx")
 
# To open Workbook
old_excel = xlrd.open_workbook(loc)
new_excel = copy(old_excel)
sheet = new_excel.get_sheet(0)

file = open("D:\Arithmethic-parser-\python_script_preprocessing\myfileinputtypes.txt", "r")
for idx,line in enumerate(file,1):
  line = re.sub(r"hz||Hz", "", line)
  if bool(re.findall(r"[yzafEYZ]", line)):
    sheet.write(idx,1,'INVALID')
  else:
    opr = re.findall(r'[*+-/]',line)
    var = re.split(r"[*+-/]", line)
    s = ""
    if opr != [] or opr != ['.'] :
        for i in range (len(opr)):
            if opr[i] == '.':
                s=s+"("+var[i]+opr[i]
            elif s!="" and s[-1]==".":
                s=s+var[i]+")"+opr[i]
            else:
                s= s+"("+var[i]+")"+opr[i]
    exp = s+"("+var[-1][:-1]+")"
    exp = exp.replace('e)-(','e-')
    exp= exp.replace('.(','.')
    exp= exp.replace('.(','.')
    exp= exp.replace('()','')
      
    sheet.write(idx,1,exp)

      
new_excel.save('D:/Arithmethic-parser-/python_script_preprocessing/new_fileName.xls')
       