#!/usr/bin/env python3
import re
import logging
import operator
from enum import Enum
from functools import reduce
from collections import namedtuple
from fractions import Fraction

import pyparsing as p
from chipconfig import aparse

# print('aparse==',aparse.parser('2*M'))

logger = logging.getLogger(__name__)

PPM_PARSER_RE = re.compile(r"^(?P<upstream>.*)(?P<signal>[+-])\s*(?P<float>[0-9.]+)\s*ppm$")

SI_SUFFIX = {
    'y': Fraction(1, int(1e24)),  # yocto
    'z': Fraction(1, int(1e21)),  # zepto
    'a': Fraction(1, int(1e18)),  # atto
    'f': Fraction(1, int(1e15)),  # femto
    'p': Fraction(1, int(1e12)),  # pico
    'n': Fraction(1, int(1e9)),   # nano
    'μ': Fraction(1, int(1e6)),   # micro
    'm': Fraction(1, int(1e3)),   # milli
    'c': Fraction(1, int(1e2)),   # centi
    'd': Fraction(1, int(1e1)),   # deci
    'da': Fraction(1e1),  # deca
    'h': Fraction(1e2),   # hecto
    'k': Fraction(1e3),   # kilo
    'M': Fraction(1e6),   # mega
    'G': Fraction(1e9),   # giga
    'T': Fraction(1e12),  # tera
    'P': Fraction(1e15),  # peta
    'E': Fraction(1e18),  # exa
    'Z': Fraction(1e21),  # zetta
    'Y': Fraction(1e24),  # yotta
}

# Useful for unparsing fractions into beautiful strings
_si_suffix = dict()
_si_suffix.update(SI_SUFFIX)
for s in ['c', 'd', 'da', 'h']:
    _si_suffix.pop(s)  # Remove suffixes not divisible by 3
_si_suffix[''] = 1  # Add a null suffix
SI_SUFFIX_UNPARSE = dict(sorted([(v, suffix) for suffix, v in _si_suffix.items()], key=lambda t: t[0]))

SI_SUFFIX['u'] = SI_SUFFIX['μ']  # micro - acceptable simplification

Operator = namedtuple('Operator', ['precedence', 'asssociativity', 'function'])
L, R = ('Left', 'Right')
OPERATORS = {
    '/': Operator(3, L, operator.truediv),
    '*': Operator(3, L, operator.mul),
    '+': Operator(2, L, operator.add),
    '-': Operator(2, L, operator.sub),
}

# Recognize many separators
FRACTION_SEPARATORS = '.,'


class Units(Enum):
    Hz = ('Hz', 'hz')  # 'hz' is technically invalid
    s = ('s',)


# Normalize fractional separator
def parse_fractionalSeparator(tokens):
    # logger.debug(':: %r', tokens)
    single_token = tokens[0]
    assert single_token in FRACTION_SEPARATORS
    # logger.info('Parsing: %s', single_token)
    return FRACTION_SEPARATORS[0]


def parse_signalNumber(tokens):
    # logger.debug(':: %r', tokens)
    single_token = tokens[0]
    logger.info('Parsing: %s', single_token)
    return int(single_token)


def parse_floatNumber(tokens):
    # logger.debug(':: %r', tokens)
    integer = tokens.get('int', 0)
    frac = tokens.get('frac', '')
    value = '%d%s' % (integer, frac)
    logger.info('Parsing: %s', value)
    return Fraction(value)


def parse_sciNumber(tokens):
    # logger.debug(':: %r', tokens)
    assert len(tokens) == 1
    single_token = tokens[0]
    mantissa = single_token['mantissa']
    exponent = single_token.get('exponent')
    if exponent:
        result = 10 ** abs(exponent)
        if exponent < 0:
            result = Fraction(1, result)
        result *= mantissa
        logger.info('Parsing: %rE%r = %r', mantissa, exponent, result)
        return result
    else:
        return mantissa


def parse_siNumber(tokens):
    # logger.debug(':: %r', tokens)
    assert len(tokens) == 1
    single_token = tokens[0]
    number = single_token['sciNumber']
    suffix = single_token.get('suffix')
    if suffix:
        suffix_value = SI_SUFFIX[suffix]
        result = number * suffix_value
    else:
        result = number
    logger.info('Parsing: %r%s = %r', number, suffix or '', result)
    return result


def ungroupTokens(tokens):
    assert len(tokens) == 1
    return list(tokens[0])


def shunting_yard(input_list):
    output, stack = [], []
    for t in input_list:
        logger.debug('Shunting: %r', t)
        if isinstance(t, int) or isinstance(t, Fraction):
            logger.debug('  Token is a number: %r', t)
            output.append(t)  # Push it to the output queue
        elif t in OPERATORS:  # Token is an "operator"
            op = OPERATORS.get(t)
            logger.debug('  Token is an operator: %r', t)
            while stack:  # While
                stack_top = stack[-1]
                logger.debug('    Top of the Stack: %r', stack_top)
                op_top = OPERATORS.get(stack_top)
                if op_top:  # Stack top is another operator
                    if (op.asssociativity == L and op.precedence <= op_top.precedence):
                        # Operator:L && Operator Top Stack has smaller precedence
                        # Pop operator from the stack to the output
                        logger.debug('    Move from Stack to Output')
                        output.append(stack.pop())
                    elif (op.asssociativity == R and op.precedence < op_top.precedence):
                        logger.debug('    Move from Stack to Output')
                        output.append(stack.pop())
                    else:
                        break
                else:  # Stack top is a paren (
                    break  # Don't remove anything else
            stack.append(t)
        elif t == '(':  # Token is a left bracket
            logger.debug('  Token is (')
            stack.append(t)  # Push it to the stack
        elif t == ')':  # Token is a right bracket
            logger.debug('  Token is )')
            while stack:
                stack_top = stack[-1]
                if stack_top == '(':
                    stack.pop()  # Pop the left bracket from the stack
                    break
                else:
                    logger.debug('    Move from Stack to Output')
                    output.append(stack.pop())  # TODO: If this pop fails, parens mismatch
        else:  # Token is invalid
            raise Exception('Invalid Token: %r' % t)
        logger.debug('Shunting:')
        logger.debug('  Output: %r', output)
        logger.debug('  Stack : %r', stack)
    while stack:
        output.append(stack.pop())
    return (output, stack)


def compute_rpn(stack):
    logger.debug('Stack: %r', stack)
    op = stack.pop()
    if op in OPERATORS:  # Binary Operator
        op2 = compute_rpn(stack)
        op1 = compute_rpn(stack)
        logger.debug('  Operation: %s %s %s', op1, op, op2)
        return OPERATORS.get(op).function(op1, op2)
    else:  # Number
        logger.debug('  Number: %r', op)
        return Fraction(op)


def build_parser(unit: Units):
    signal = p.Char('+-')
    fractional_separator = p.Char(FRACTION_SEPARATORS).setParseAction(parse_fractionalSeparator)
    digits = p.OneOrMore(p.Char(p.nums))
    signalNumber = p.Combine(p.Optional(signal) + digits).setParseAction(parse_signalNumber)
    suffixes = p.Or([p.Literal(s) for s in list(SI_SUFFIX)])
    floatFrac = p.Combine(fractional_separator + digits)('frac')
    floatNumber = p.Or([
        # NUM.NUM
        p.Combine(
            signalNumber('int') + p.Optional(floatFrac)
        ),
        # NUM.
        p.Combine(
            signalNumber('int') + fractional_separator
        ),
        # .NUM
        floatFrac,
    ]).setParseAction(parse_floatNumber)
    sciNumber = p.Combine(
        floatNumber('mantissa') +
        p.Optional(
            p.CaselessLiteral('E') + signalNumber('exponent')
        )
    ).setParseAction(parse_sciNumber)
    if unit is None:
        siNumber = p.Group(
            sciNumber('sciNumber') +
            p.Optional(
                suffixes('suffix')
            )
        ).setParseAction(parse_siNumber)
        # `siNumber` + Optional(units) fails on ambiguous units/SI suffixes
        number = siNumber
    else:
        units = p.Or([p.Literal(u) for u in unit.value])
        # On ambiguous units/SI suffixes, units win
        siUnitsNumber = p.Group(
            sciNumber.setResultsName('sciNumber') +
            p.Or([
                units,
                p.Optional(
                    suffixes.setResultsName('suffix') + p.Optional(units)
                )
            ])
        ).setParseAction(parse_siNumber)
        number = siUnitsNumber

    leftParen = p.Literal('(')
    rightParen = p.Literal(')')

    expr = p.Forward()
    atom = p.Group(number | p.Group(leftParen + expr + rightParen)).setParseAction(ungroupTokens)
    # e_simple = p.ZeroOrMore(leftParen | rightParen | number | p.Word(''.join(OPERATORS.keys())))
    expr << atom + p.ZeroOrMore(p.Group(p.Char(''.join(OPERATORS.keys())) + expr).setParseAction(ungroupTokens))

    return expr


def raw_parser(input_string: str, debug: bool = False, unit: Units = None):
    if input_string.strip() == '':
        return None
    pexpr = build_parser(unit)
    try:
        raw = pexpr.parseString(input_string, parseAll=True)
        logger.debug('Raw: %r', raw)

        def flatten_expr(accumulator, obj):
            subobj = None
            if isinstance(obj, p.ParseResults):
                subobj = reduce(flatten_expr, list(obj), [])
            elif isinstance(obj, tuple):
                subobj = list(obj)
            else:
                subobj = [obj]
            return operator.concat(accumulator, subobj)
        parsed = reduce(flatten_expr, raw, [])
        logger.debug('Parsed: %r', parsed)
        output_stack, operator_stack = shunting_yard(parsed)
        logger.debug('Shunting Yard Output: %r', output_stack)
        calc = compute_rpn(output_stack)
        logger.debug('RPN Result: %r', calc)
        return calc
    except p.ParseBaseException as e:
        if debug:
            raise e
        else:
            return None


# Public Parser | Defaults to supporting no Units
def parser(value, *args, ppm=True, **kwargs):
    """
    Wrap `raw_parser` to support ppm expressions.

    Supports:
    - $EXPR + X ppm
    - $EXPR - X ppm
    New Value:
    - $EXPR * (1 + (X / 1e6))
    - $EXPR * (1 - (X / 1e6))
    """
    v = PPM_PARSER_RE.match(value)
    if v is not None:
        upstream = v.group('upstream')
        signal = v.group('signal')
        f = v.group('float')
        try:
            float(f.replace(',', '.'))
            # Disallow multiple expressions to the left of the PPM value
            for op in OPERATORS.keys():
                if op in upstream:
                    raise ValueError
        except ValueError:
            return None
        else:
            uvalue = parser(upstream, *args, **kwargs)
            if uvalue is None:
                return uvalue
            else:
                string = '(%s) * (1 %s (%s / 1M))' % (uvalue, signal, f)
                logger.debug('String: %s', string)
                return raw_parser(string)
    else:
        return raw_parser(value, *args, **kwargs)


# Public Parsers | One for each `Units`
def parser_Hz(*args, **kwargs):
    return parser(*args, **kwargs, unit=Units.Hz)


def parser_s(*args, **kwargs):
    return parser(*args, **kwargs, unit=Units.s)

def cparser(frequency):
    cparsed_freq = aparse.parser(str(frequency))
    return Fraction(cparsed_freq)


if __name__ == '__main__':
    import sys
    logging.basicConfig(level=logging.DEBUG, format='%(levelname)s:%(funcName)s: %(message)s')
    #input_string = sys.argv[1] if len(sys.argv) > 1 else '3 + 4 * 2 / ( 1 - 5 ) * ( 2 - 3 )'
    # logging.info('Parse: "%s"', input_string)
    import xlrd
    from xlutils.copy import copy
    import re
    import aparse 
    loc = ("D:/AuraAuro_GUIgen2/chipconfig/Inputtypesfilesspreadsheet.xlsx")
 
# open Workbook,
# copy old to new excel
# Get the first sheet
    old_excel = xlrd.open_workbook(loc)
    new_excel = copy(old_excel)
    sheet = new_excel.get_sheet(0)
    
    # Open the text file that has all input types. 
    file = open("D:\AuraAuro_GUIgen2\chipconfig\myfileinputtypes.txt", "r")

    for idx,line in enumerate(file,1):
      # line = line.strip()
      # line = re.sub(r"hz", "", line,flags=re.I)
      # OPER = r'*+\-/'
      # GOODOPERS = '+-*/\(\)'
      # BADOPERS = ''.join([k for k in punctuation if k not in GOODOPERS])
      # GOODUNITS = r'munpkMGTP'
      # BADUNITS  = ''.join([k for k in ascii_letters if k not in GOODUNITS])
      
      # opr = re.findall('[%s]' % OPER,line)
      # var = re.split('[%s]' % OPER, line)
    
      # # Check for invalid inputs(operator followed by alphabet) such as 1*M,2+m
      # # Check for invalid inputs(alphabets other than valid units) such as yzafEYZ,If true write INVALID to column 4 of the excel
      # # invalid_units = [True for o in opr if line[line.index(o)+1].isalpha()]
      # if bool(re.findall(r"[%s]|[%s]|([%s][%s])" % (BADOPERS,BADUNITS,OPER,ascii_letters), line)) : #or any(invalid_units) :
      #   sheet.write(idx,4,'INVALID')
      #   print(f'For input={line}, output="INVALID"')
      # # Recreate the expression with ( and ) on every atom includng its units
      # # Replace 'e)-(' by 'e-' e.g. 4e-9 should be (4e-9) and not (4e)-(9)
      # # Replace () by ''
      # # 4e2M==>(4e2M)
      # else:
      #   s=""
      #   s ="".join([ s+"("+var[i]+")"+opr[i] if i<len(opr) else s+"("+var[i]+")" for i in range (len(opr)+1) ])
      #   exp= s.replace('()','')
      #   exp= exp.replace('e)-(','e-')
        
      #   # Replace 1M,2m to 1*M and 2*m.
      #   replace_string = [ exp[idx:idx+2] for idx,c in enumerate(exp) if c.isnumeric() and exp[idx+1].isalpha() and not exp[idx+1]=='e']
      #   for rstr in replace_string:
      #       exp = exp.replace(rstr,rstr[0]+'*'+rstr[-1])
        
        ast=r'%s' % line
    
        # Write the expression to column 4
        # sheet.write(idx,4,exp)
    
        # Write the cparsed value to column 5
        cparsed_op = parser_Hz(ast)
        sheet.write(idx,1,str(cparsed_op))
        # print(f'For input={line}, exp={ast}, cparsed_op = {cparsed_op}')
    
    # Save the new excel in the following directory.      
    new_excel.save('D:/AuraAuro_GUIgen2/chipconfig/output_excel.xls')   
    # r = parser_Hz('2Hz')
    # print('Result:', r)
