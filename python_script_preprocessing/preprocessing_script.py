# -*- coding: utf-8 -*-
"""
Created on Thu Apr 22 22:47:44 2021

@author: mural
"""
# import xlwt
# from xlwt import Workbook
import xlrd
from xlutils.copy import copy
import re
import aparse
# print(aparse.parser('(20)+(20*m)'))

# Give the location of the file
loc = ("D:/Arithmethic-parser-/python_script_preprocessing/Inputtypesfilesspreadsheet.xlsx")
 
# To open Workbook
old_excel = xlrd.open_workbook(loc)
new_excel = copy(old_excel)
sheet = new_excel.get_sheet(0)

file = open("D:\Arithmethic-parser-\python_script_preprocessing\myfileinputtypes.txt", "r")
for idx,line in enumerate(file,1):
  line = re.sub(r"hz||Hz", "", line)
  opr = re.findall(r'[*+-/]',line)
  opr = [o for o in opr if o!='.']
  var = re.split(r"[*+-/]", line)
  var = [v.replace('\n','') if '\n' in v else v for v in var]
  print(var)
  # var = var.remove('\n')
  invalid_units = [True for o in opr if line[line.index(o)+1].isalpha()]
  if bool(re.findall(r"[yzafEYZ]", line)) or any(invalid_units) :
    sheet.write(idx,4,'INVALID')
  else:
    s = ""
    if opr != []:
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
    replace_string = [ exp[idx:idx+2] for idx,c in enumerate(exp) if c.isnumeric() and exp[idx+1].isalpha() and not exp[idx+1]=='e']
    # print(replace_string)    
    for rstr in replace_string:
        exp = exp.replace(rstr,rstr[0]+'*'+rstr[-1])
    # print(exp)
        
    
      
    sheet.write(idx,4,exp)
    # cparsed_op = aparse.parser(exp)
    # sheet.write(idx,5,cparsed_op)

      
new_excel.save('D:/Arithmethic-parser-/python_script_preprocessing/output_excel.xls')
       