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
from fractions import Fraction

# location of the input excel file
loc = ("D:/Arithmethic-parser-/python_script_preprocessing/Inputtypesfilesspreadsheet.xlsx")
 
# open Workbook,
# copy old to new excel
# Get the first sheet
old_excel = xlrd.open_workbook(loc)
new_excel = copy(old_excel)
sheet = new_excel.get_sheet(0)

# Open the text file that has all input types. 
file = open("D:\Arithmethic-parser-\python_script_preprocessing\myfileinputtypes.txt", "r")

# loop through the lines one by one   ---> Example : 20hz+20mHz
# Strip the \n at the end
# Replace hz/Hz by ''                 ---> Example : 20+20m
# Find list of all operators in the given expression and store in opr.      --->   opr= ['+']
# Split the given expression by the operators in opr and store in var.      --->   var= ['20', '20m']
from string import ascii_letters, punctuation

ascii_letters_without_e = ascii_letters.replace('e','')

# Fix the column width to 10000
for i in range(6):
    sheet.col(i).width =9000

for idx,line in enumerate(file,1):
  line = line.strip()
  line = re.sub(r"hz", "", line,flags=re.I)
  OPER = r'*+\-/'
  GOODOPERS = '+-*/\(\)'
  BADOPERS = ''.join([k for k in punctuation if k not in GOODOPERS])
  GOODUNITS = r'munpkMGTP'
  BADUNITS  = ''.join([k for k in ascii_letters_without_e if k not in GOODUNITS])
  
  opr = re.findall('[%s]' % OPER,line)
  var = re.split('[%s]' % OPER, line)

  # Check for invalid inputs(operator followed by alphabet) such as 1*M,2+m
  # Check for invalid inputs(alphabets other than valid units) such as yzafEYZ,If true write INVALID to column 4 of the excel
  # invalid_units = [True for o in opr if line[line.index(o)+1].isalpha()]
  if bool(re.findall(r"[%s]|[%s]|([%s][%s])" % (BADOPERS,BADUNITS,OPER,ascii_letters_without_e), line)) : #or any(invalid_units) :
    sheet.write(idx,3,'INVALID')
    print(f'For input={line}, output="INVALID"')
  # Recreate the expression with ( and ) on every atom includng its units
  # Replace 'e)-(' by 'e-' e.g. 4e-9 should be (4e-9) and not (4e)-(9)
  # Replace () by ''
  # 4e2M==>(4e2M)
  else:
    s=""
    s ="".join([ s+"("+var[i]+")"+opr[i] if i<len(opr) else s+"("+var[i]+")" for i in range (len(opr)+1) ])
    exp= s.replace('()','')
    exp= exp.replace('e)-(','e-')
    
    # Replace 1M,2m to 1*M and 2*m.
    replace_string = [ exp[idx:idx+2] for idx,c in enumerate(exp) if c.isnumeric() and exp[idx+1].isalpha() and not exp[idx+1]=='e']
    for rstr in replace_string:
        exp = exp.replace(rstr,rstr[0]+'*'+rstr[-1])
    
    ast=r'%s' % exp

    # Write the expression to column 4
    sheet.write(idx,3,exp)

    # Write the cparsed value to column 5
    cparsed_op = aparse.parser(ast)
    sheet.write(idx,4,cparsed_op)
    print(f'For input={line}, exp={ast}, cparsed_op = {cparsed_op}')

# Save the new excel in the following directory.      
new_excel.save('D:/Arithmethic-parser-/python_script_preprocessing/output_excel.xls')



# Compare columns
rb1 = xlrd.open_workbook('D:/Arithmethic-parser-/python_script_preprocessing/output_excel.xls')
sheet1 = rb1.sheet_by_index(0)
arithmetic_op = sheet1.col_values(1)
cparsed_op = sheet1.col_values(4)
# print('arithmetic_op=',arithmetic_op)
# print('cparsed_op=',cparsed_op)
print('sheet1.nrows=',sheet1.nrows)


for idx in range(1,sheet1.nrows):
    # print(f'Row number: {idx}')
    comp1 = str(arithmetic_op[idx])
    comp2 = str(cparsed_op[idx])
    if not comp2 =='' and  not comp1 =='':
        if Fraction(str(arithmetic_op[idx]))!= Fraction(str(cparsed_op[idx])):
            print(f'Difference in Row number: {idx+1}')
            print(f'Comparing: {comp1} and {comp2}') 
print("*****************************")
    

