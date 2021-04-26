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
# print(aparse.parser('20*m'))
# print(aparse.parser('20e6'))

# location of the input excel file
loc = ("D:/Arithmethic-parser-/python_script_preprocessing/Inputtypesfilesspreadsheet.xlsx")
 
# open Workbook,
# copy old to new excel
# Get the first sheet
old_excel = xlrd.open_workbook(loc)
new_excel = copy(old_excel)
sheet = new_excel.get_sheet(0)

# Open the text file that has all input types. 
file = open("D:\Arithmethic-parser-\python_script_preprocessing\myfileinputtypes_1.txt", "r")

# loop through the lines one by one   ---> Example : 20hz+20mHz
# Strip the \n at the end
# Replace hz/Hz by ''                 ---> Example : 20+20m
# Find list of all operators in the given expression and store in opr.      --->   opr= ['+']
# Split the given expression by the operators in opr and store in var.      --->   var= ['20', '20m']

for idx,line in enumerate(file,1):
  line = line.strip()
  line = re.sub(r"hz", "", line,flags=re.I)
  opr = re.findall(r'[*+\-/]',line)
  var = re.split(r'[*+\-/]', line)

  # Check for invalid inputs(operator followed by alphabet) such as 1*M,2+m
  # Check for invalid inputs(alphabets other than valid units) such as yzafEYZ,If true write INVALID to column 4 of the excel
  invalid_units = [True for o in opr if line[line.index(o)+1].isalpha()]
  if bool(re.findall(r"[yzafEYZ]", line)) or any(invalid_units) :
    sheet.write(idx,4,'INVALID')

  # Recreate the expression with ( and )
  # Replace 'e)-(' by 'e-'
  # Replace () by ''
  else:
    s=""
    s ="".join([ s+"("+var[i]+")"+opr[i] if i<len(opr) else s+"("+var[i]+")" for i in range (len(opr)+1) ])
    exp = s.replace('e)-(','e-')
    exp= exp.replace('()','')
    
    # Replace 1M,2m to 1*M and 2*m.
    replace_string = [ exp[idx:idx+2] for idx,c in enumerate(exp) if c.isnumeric() and exp[idx+1].isalpha() and not exp[idx+1]=='e']
    for rstr in replace_string:
        exp = exp.replace(rstr,rstr[0]+'*'+rstr[-1])
    
    # Write the expression to column 4
    sheet.write(idx,4,exp)
    
    # Fix the column width to 10000
    for i in range(6):
        sheet.col(i).width =9000
    
    # Write the cparsed value to column 5
    # cparsed_op = f'Fraction{aparse.parser(exp)})'
    # sheet.write(idx,5,cparsed_op)

# Save the new excel in the following directory.      
new_excel.save('D:/Arithmethic-parser-/python_script_preprocessing/output_excel.xls')
