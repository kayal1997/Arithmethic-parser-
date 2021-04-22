/*
 * TINYEXPR - Tiny recursive descent parser and evaluation engine in C
 *
 * Copyright (c) 2015-2020 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgement in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* COMPILE TIME OPTIONS */

/* Exponentiation associativity:
For a^b^c = (a^b)^c and -a^b = (-a)^b do nothing.
For a^b^c = a^(b^c) and -a^b = -(a^b) uncomment the next line.*/
/* #define TE_POW_FROM_RIGHT */

/* Logarithms
For log = base 10 log do nothing
For log = natural log uncomment the next line. */
/* #define TE_NAT_LOG */

#include "tinyexpr.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

long long gcd( long long a, long long b)
{
    long long temp;
    while (b != 0)
    {
        temp = a % b;

        a = b;
        b = temp;
    }
    return a;
}

Rational Fraction(long long n, long long d){
		long long gcd_val;
	    Rational result;
		gcd_val = gcd(n,d);
		result.numerator   = n/gcd_val;
		result.denominator = d/gcd_val;
		return result;
}

static Rational RNAN(){return Fraction(-1,1);}
static Rational RINFINITY(){return Fraction(-1000000000000000,1);} 

typedef Rational (*te_fun2)(Rational, Rational);

enum {
    TOK_NULL = TE_CLOSURE7+1, TOK_ERROR, TOK_END, TOK_SEP,
    TOK_OPEN, TOK_CLOSE, TOK_NUMBER, TOK_VARIABLE, TOK_INFIX
};


enum {TE_CONSTANT = 1};


typedef struct state {
    const char *start;
    const char *next;
    int type;
    union {Rational value; const Rational *bound; const void *function;};
    void *context;

    const te_variable *lookup;
    int lookup_len;
} state;


#define TYPE_MASK(TYPE) ((TYPE)&0x0000001F)

#define IS_PURE(TYPE) (((TYPE) & TE_FLAG_PURE) != 0)
#define IS_FUNCTION(TYPE) (((TYPE) & TE_FUNCTION0) != 0)
#define IS_CLOSURE(TYPE) (((TYPE) & TE_CLOSURE0) != 0)
#define ARITY(TYPE) ( ((TYPE) & (TE_FUNCTION0 | TE_CLOSURE0)) ? ((TYPE) & 0x00000007) : 0 )
#define NEW_EXPR(type, ...) new_expr((type), (const te_expr*[]){__VA_ARGS__})

static te_expr *new_expr(const int type, const te_expr *parameters[]) {
    const int arity = ARITY(type);
    const int psize = sizeof(void*) * arity;
    const int size = (sizeof(te_expr) - sizeof(void*)) + psize + (IS_CLOSURE(type) ? sizeof(void*) : 0);
    te_expr *ret = malloc(size);
    memset(ret, 0, size);
    if (arity && parameters) {
        memcpy(ret->parameters, parameters, psize);
    }
    ret->type = type;
    ret->bound = 0;
    return ret;
}


void te_free_parameters(te_expr *n) {
    if (!n) return;
    switch (TYPE_MASK(n->type)) {
        case TE_FUNCTION7: case TE_CLOSURE7: te_free(n->parameters[6]);     /* Falls through. */
        case TE_FUNCTION6: case TE_CLOSURE6: te_free(n->parameters[5]);     /* Falls through. */
        case TE_FUNCTION5: case TE_CLOSURE5: te_free(n->parameters[4]);     /* Falls through. */
        case TE_FUNCTION4: case TE_CLOSURE4: te_free(n->parameters[3]);     /* Falls through. */
        case TE_FUNCTION3: case TE_CLOSURE3: te_free(n->parameters[2]);     /* Falls through. */
        case TE_FUNCTION2: case TE_CLOSURE2: te_free(n->parameters[1]);     /* Falls through. */
        case TE_FUNCTION1: case TE_CLOSURE1: te_free(n->parameters[0]);
    }
}


void te_free(te_expr *n) {
    if (!n) return;
    te_free_parameters(n);
    free(n);
}

/*
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
    'da': int(1e1),    # deca
    'h': int(1e2),    # hecto
    'k': int(1e3),    # kilo
    'M': int(1e6),    # mega 
    'G': int(1e9),    # giga
    'T': int(1e12),   # tera
    'P': int(1e15),   # peta
    'E': int(1e18),   # exa
    'Z': int(1e21),   # zetta
    'Y': int(1e24),   # yotta
*/

#define Yacto 10000000000000000000
Rational add( Rational a,  Rational b){
	 long long num;
	 long long den;
	 long long gcd_val;
	 Rational  result;
	
	num = a.numerator*b.denominator+b.numerator*a.denominator;
	den = a.denominator * b.denominator;
	gcd_val = gcd(num,den);
	result.numerator   = num/gcd_val;
	result.denominator = den/gcd_val;
	return result;
}
    Rational sub ( Rational a, Rational b){
       long long num;
       long long den;
       long long gcd_val;
        Rational result;
       num = a.numerator*b.denominator-b.numerator*a.denominator;
	   den = a.denominator * b.denominator;
	   gcd_val = gcd(num,den);
	   result.numerator   = num/gcd_val;
	   result.denominator = den/gcd_val;
	   return result;
}
Rational mul(Rational a,Rational b){
       long long num;
       long long den;
       long long gcd_val;
        Rational result;
       num = a.numerator*b.numerator;
	   den = a.denominator * b.denominator;
	   gcd_val = gcd(num,den);
	   result.numerator   = num/gcd_val;
	   result.denominator = den/gcd_val;
	   return result;
}
Rational divide(Rational a,Rational b){
       long long num;
       long long den;
       long long gcd_val;
        Rational result;
       num = a.numerator*b.denominator;
	   den = a.denominator * b.numerator;
	   gcd_val = gcd(num,den);
	   result.numerator   = num/gcd_val;
	   result.denominator = den/gcd_val;
	   return result;
}

Rational tenpow(Rational a,Rational b){
       long long num;
       long long den;
       long long gcd_val;
       Rational result;
		if (b.denominator == 1){
			//printf("DEBUG: power of fraction\n");
		   num = a.numerator*(long long)pow(10,b.numerator);
		   den = a.denominator ;
		   gcd_val = gcd(num,den);
		   result.numerator   = num/gcd_val;
		   result.denominator = den/gcd_val;
		}
		else{
			//printf("ERROR: Cannot handle power of fraction\n");
		}
	   return result;
}
Rational negate(Rational a){
        Rational result;
		result.numerator = -a.numerator;
		result.denominator = a.denominator;
		return result;
}

//static Rational y(void) {return Fraction(1,1000000000000000000000000);}//yocto
//static Rational z(void) {return Fraction(1,1000000000000000000);}//zepto
//static Rational f(void) {return Fraction(1,1000000000000000);}//femto
static Rational p(void) {return Fraction(1,1000000000000);}//pico
static Rational n(void) {return Fraction(1,1000000000);}//nano
static Rational u(void) {return Fraction(1,1000000);}//micro
static Rational m(void) {return Fraction(1,1000);}//milli
static Rational c(void) {return Fraction(1,100);}//centi
static Rational d(void) {return Fraction(1,10);}//deci
static Rational da(void){return Fraction(10,1);}//deca
static Rational h(void) {return Fraction(100,1);}//hectogcm
static Rational k(void) {return Fraction(1000,1);}//kilo
static Rational M(void) {return Fraction(1000000,1);}//mega
static Rational Mhz(void) {return Fraction(1000000,1);}//mega
static Rational G(void) {return Fraction(1000000000,1);}//giga
static Rational T(void) {return Fraction(100000000000,1);}//tera
static Rational P(void) {return Fraction(100000000000000,1);}//peta
//static Rational E(void) {return Fraction(1000000000000000000,1);}//exa
//static Rational Y(void) {return Fraction(1000000000000000000000000,1);}//yotta
//static Rational Z(void) {return Fraction(1000000000000000000000,1);}//zetto


static Rational fac(Rational a) {/* simplest version of fac */
    if (a.numerator < 0.0)
        return RNAN();
    if (a.numerator > UINT_MAX)
        return RINFINITY();
    unsigned int ua = (unsigned int)(a.numerator);
    unsigned long int result = 1, i;
    for (i = 1; i <= ua; i++) {
        if (i > ULONG_MAX / result)
            return RINFINITY();
        result *= i;
    }
    return Fraction(result,1);
}
static Rational ncr(Rational n, Rational r) {
    if (n.numerator < 0.0 || r.numerator < 0.0 || n.numerator < r.denominator) return RNAN();
    if (n.numerator > UINT_MAX || r.numerator > UINT_MAX) return RINFINITY();
    unsigned long int un = (unsigned int)(n.numerator), ur = (unsigned int)(r.numerator), i;
    unsigned long int result = 1;
    if (ur > un / 2) ur = un - ur;
    for (i = 1; i <= ur; i++) {
        if (result > ULONG_MAX / (un - ur + i))
            return RINFINITY();
        result *= un - ur + i;
        result /= i;
    }
    return Fraction(result,1);
}
static Rational npr(Rational n, Rational r) {return mul(ncr(n, r) , fac(r));}

static const te_variable functions[] = {
    /* must be in alphabetical order */
	//{"E",E,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"G",G,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"M",M,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"Mhz",Mhz,       TE_FUNCTION0 | TE_FLAG_PURE, 0},	
	{"P",P,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"T",T,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	//{"Y",Y,           TE_FUNCTION0 | TE_FlAG_PURE, 0},
	//{"Z",Z,           TE_FUNCTION0 | TE_FlAG_PURE, 0},
    {"abs", fabs,     TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"acos", acos,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"asin", asin,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"atan", atan,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"atan2", atan2,  TE_FUNCTION2 | TE_FLAG_PURE, 0},
	{"c",c, 		  TE_FUNCTION0 | TE_FLAG_PURE, 0},
    //{"ceil", ceil,    TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"cos", cos,      TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"cosh", cosh,    TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"d",d,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"da",da,         TE_FUNCTION0 | TE_FLAG_PURE, 0},
	// {"exp", exp,      TE_FUNCTION0 | TE_FLAG_PURE, 0},
	//{"f",f,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"fac", fac,      TE_FUNCTION0 | TE_FLAG_PURE, 0},
   // {"floor", floor,  TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"h",h,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"k",k,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"ln",log,        TE_FUNCTION0 | TE_FLAG_PURE, 0},
#ifdef TE_NAT_LOG
    {"log", log,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
#else
    {"log", log10,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
#endif
    {"log10", log10,  TE_FUNCTION1 | TE_FLAG_PURE, 0},
	{"m",m,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	{"n",n,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"ncr", ncr,      TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"npr", npr,      TE_FUNCTION2 | TE_FLAG_PURE, 0},
	{"p",p,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"pow", pow,      TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"sin", sin,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"sinh", sinh,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"sqrt", sqrt,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
	{"tan", tan,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"tanh", tanh,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
	{"u",u,           TE_FUNCTION0 | TE_FLAG_PURE, 0},
	//{"y",y,           TE_FUNCTION1 | TE_FLAG_PURE, 0},
	//{"z",z,           TE_FUNCTION0 | TE_FLAG_PURE, 0},

    {0, 0, 0, 0}
};

static const te_variable *find_builtin(const char *name, int len) {
    int imin = 0;
    int imax = sizeof(functions) / sizeof(te_variable) - 2;

    /*Binary search.*/
    while (imax >= imin) {
        const int i = (imin + ((imax-imin)/2));
        int c = strncmp(name, functions[i].name, len);
        if (!c) c = '\0' - functions[i].name[len];
        if (c == 0) {
            return functions + i;
        } else if (c > 0) {
            imin = i + 1;
        } else {
            imax = i - 1;
        }
    }

    return 0;
}

static const te_variable *find_lookup(const state *s, const char *name, int len) {
    int iters;
    const te_variable *var;
    if (!s->lookup) return 0;

    for (var = s->lookup, iters = s->lookup_len; iters; ++var, --iters) {
        if (strncmp(name, var->name, len) == 0 && var->name[len] == '\0') {
            return var;
        }
    }
    return 0;
}

static Rational comma(Rational a, Rational b){(void) a; return b;}

Rational convert_str(const char *st, char **end){
  	char *numer = (char *)malloc(sizeof(char)*strlen(st));
	char *dummy_pt; // Return value of the integer conversion I do not need.
	double dummy_num; // Dummy float number that it would have converted that I do not need.
	char dot;
	int point_found, point_loc, remaining_str;
  	int i;
	int j;
  	long int denom;
  	dot='.';
	j=0;
	point_found = 0;
	remaining_str = 0;
  	for(i = 0; i < strlen(st); i++)
  	{
		if ((st[i] >= '0' && st[i] <= '9') || st[i] == dot){
			if(st[i] == dot)  
			{
				point_found = 1;
				point_loc=i;
			}
			else{
				numer[j]=st[i];
				j = j+1;
			}
		}
		else{
			remaining_str = 1;
			break;
		}
	}
	*end = st+i;
	/*
	if (remaining_str==1){
		*end = st+i;
	}
	else{
		*end = st+i+1;
	}
	*/
	// printf("DEBUG: Remaining string %s\n",**end);
	if(point_found == 1){
		//printf("DEBUG: i: %d, point_loc=%d, power=%d\n",i,point_loc,i-point_loc-1);
		denom=pow(10,i-point_loc-1);
	}
	else{
		denom=1;
	}
	//printff("DEBUG: begining st pointer :%d\n",st);
	//printff("DEBUG: What I set :%d\n",*end);
	// dummy_num = strtod(st,end); // 1*M, 1.4*M, 1e4
	// //printff("DEBUG: begining st pointer :%d\n",st);
	// printf("DEBUG: strtod sets to :%d\n",*end);
	// printf("DEBUG: Dummy number found is : %f\n",dummy_num);
  	Rational result;
	result = Fraction((long long)strtol(numer, &dummy_pt,10), (long long)denom);
  	// printf("DEBUG: Convert Str: (%lld, %lld)\n",result.numerator,result.denominator);
	free(numer);
	return result;
	
}

void next_token(state *s) {
    s->type = TOK_NULL;

    do {

        if (!*s->next){
            s->type = TOK_END;
            return;
        }

        /* Try reading a number. */
        if ((s->next[0] >= '0' && s->next[0] <= '9') || s->next[0] == '.') {
			// printf("DEBUG: Number = %s\n",s->next);
            s->value = convert_str(s->next, (char**)&s->next);
            s->type = TOK_NUMBER;
        } else {
            /* Look for a variable or builtin function call. */
            if (isalpha(s->next[0]) &&!( s->next[0]=='e' || s->next[0]=='E')) {
                const char *start;
                start = s->next;
                while (isalpha(s->next[0]) || isdigit(s->next[0]) || (s->next[0] == '_')) s->next++;
                
                const te_variable *var = find_lookup(s, start, s->next - start);
                if (!var) var = find_builtin(start, s->next - start);

                if (!var) {
                    s->type = TOK_ERROR;
                } else {
                    switch(TYPE_MASK(var->type))
                    {
                        case TE_VARIABLE:
                            s->type = TOK_VARIABLE;
                            s->bound = var->address;
                            break;

                        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:         /* Falls through. */
                        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:         /* Falls through. */
                            s->context = var->context;                                                  /* Falls through. */

                        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:     /* Falls through. */
                        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:     /* Falls through. */
                            s->type = var->type;
                            s->function = var->address;
                            break;
                    }
                }

            } else {
                /* Look for an operator or special character. */
                switch (s->next++[0]) {
                    case '+': s->type = TOK_INFIX; s->function = add; break;
                    case '-': s->type = TOK_INFIX; s->function = sub; break;
                    case '*': s->type = TOK_INFIX; s->function = mul; break;
                    case '/': s->type = TOK_INFIX; s->function = divide; break;
                    case 'e': s->type = TOK_INFIX; s->function = tenpow; break;
                    case 'E': s->type = TOK_INFIX; s->function = tenpow; break;
                    case '^': s->type = TOK_INFIX; s->function = pow; break;
                    case '%': s->type = TOK_INFIX; s->function = fmod; break;
                    case '(': s->type = TOK_OPEN; break;
                    case ')': s->type = TOK_CLOSE; break;
                    case ',': s->type = TOK_SEP; break;
                    case ' ': case '\t': case '\n': case '\r': break;
                    default: s->type = TOK_ERROR; break;
                }
            }
        }
    } while (s->type == TOK_NULL);
}


static te_expr *list(state *s);
static te_expr *expr(state *s);
static te_expr *power(state *s);

static te_expr *base(state *s) {
    /* <base>      =    <constant> | <variable> | <function-0> {"(" ")"} | <function-1> <power> | <function-X> "(" <expr> {"," <expr>} ")" | "(" <list> ")" */
    te_expr *ret;
    int arity;

    switch (TYPE_MASK(s->type)) {
        case TOK_NUMBER:
            ret = new_expr(TE_CONSTANT, 0);
			//printff("Return Value set = (%lld,%lld)\n",s->value.numerator, s->value.denominator);
            ret->value = s->value;
            next_token(s);
            break;

        case TOK_VARIABLE:
            ret = new_expr(TE_VARIABLE, 0);
            ret->bound = s->bound;
            next_token(s);
            break;

        case TE_FUNCTION0:
        case TE_CLOSURE0:
            ret = new_expr(s->type, 0);
            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[0] = s->context;
            next_token(s);
            if (s->type == TOK_OPEN) {
                next_token(s);
                if (s->type != TOK_CLOSE) {
                    s->type = TOK_ERROR;
                } else {
                    next_token(s);
                }
            }
            break;

        case TE_FUNCTION1:
        case TE_CLOSURE1:
            ret = new_expr(s->type, 0);
            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[1] = s->context;
            next_token(s);
            ret->parameters[0] = power(s);
            break;

        case TE_FUNCTION2: case TE_FUNCTION3: case TE_FUNCTION4:
        case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
        case TE_CLOSURE2: case TE_CLOSURE3: case TE_CLOSURE4:
        case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            arity = ARITY(s->type);

            ret = new_expr(s->type, 0);
            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[arity] = s->context;
            next_token(s);

            if (s->type != TOK_OPEN) {
                s->type = TOK_ERROR;
            } else {
                int i;
                for(i = 0; i < arity; i++) {
                    next_token(s);
                    ret->parameters[i] = expr(s);
                    if(s->type != TOK_SEP) {
                        break;
                    }
                }
                if(s->type != TOK_CLOSE || i != arity - 1) {
                    s->type = TOK_ERROR;
                } else {
                    next_token(s);
                }
            }

            break;

        case TOK_OPEN:
            next_token(s);
            ret = list(s);
            if (s->type != TOK_CLOSE) {
                s->type = TOK_ERROR;
            } else {
                next_token(s);
            }
            break;

        default:
            ret = new_expr(0, 0);
            s->type = TOK_ERROR;
            ret->value = RNAN();
            break;
    }

    return ret;
}


static te_expr *power(state *s) {
    /* <power>     =    {("-" | "+")} <base> */
    int sign = 1;
    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        if (s->function == sub) sign = -sign;
        next_token(s);
    }

    te_expr *ret;

    if (sign == 1) {
        ret = base(s);
    } else {
        ret = NEW_EXPR(TE_FUNCTION1 | TE_FLAG_PURE, base(s));
        ret->function = negate;
    }

    return ret;
}

#ifdef TE_POW_FROM_RIGHT
static te_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    te_expr *ret = power(s);

    int neg = 0;

    if (ret->type == (TE_FUNCTION1 | TE_FLAG_PURE) && ret->function == negate) {
        te_expr *se = ret->parameters[0];
        free(ret);
        ret = se;
        neg = 1;
    }

    te_expr *insertion = 0;

    while (s->type == TOK_INFIX && (s->function == pow)) {
        te_fun2 t = s->function;
        next_token(s);

        if (insertion) {
            /* Make exponentiation go right-to-left. */
            te_expr *insert = NEW_EXPR(TE_FUNCTION2 | TE_FLAG_PURE, insertion->parameters[1], power(s));
            insert->function = t;
            insertion->parameters[1] = insert;
            insertion = insert;
        } else {
            ret = NEW_EXPR(TE_FUNCTION2 | TE_FLAG_PURE, ret, power(s));
            ret->function = t;
            insertion = ret;
        }
    }

    if (neg) {
        ret = NEW_EXPR(TE_FUNCTION1 | TE_FLAG_PURE, ret);
        ret->function = negate;
    }

    return ret;
}
#else
static te_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    te_expr *ret = power(s);

    while (s->type == TOK_INFIX && (s->function == pow)) {
        te_fun2 t = s->function;
        next_token(s);
        ret = NEW_EXPR(TE_FUNCTION2 | TE_FLAG_PURE, ret, power(s));
        ret->function = t;
    }

    return ret;
}
#endif



static te_expr *term(state *s) {
    /* <term>      =    <factor> {("*" | "/" | "%") <factor>} */
    te_expr *ret = factor(s);

    while (s->type == TOK_INFIX && (s->function == mul || s->function == divide || s->function == fmod || s->function == tenpow)) {
        te_fun2 t = s->function;
        next_token(s);
        ret = NEW_EXPR(TE_FUNCTION2 | TE_FLAG_PURE, ret, factor(s));
        ret->function = t;
    }

    return ret;
}


static te_expr *expr(state *s) {
    /* <expr>      =    <term> {("+" | "-") <term>} */
    te_expr *ret = term(s);

    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        te_fun2 t = s->function;
        next_token(s);
        ret = NEW_EXPR(TE_FUNCTION2 | TE_FLAG_PURE, ret, term(s));
        ret->function = t;
    }

    return ret;
}


static te_expr *list(state *s) {
    /* <list>      =    <expr> {"," <expr>} */
    te_expr *ret = expr(s);

    while (s->type == TOK_SEP) {
        next_token(s);
        ret = NEW_EXPR(TE_FUNCTION2 | TE_FLAG_PURE, ret, expr(s));
        ret->function = comma;
    }

    return ret;
}


#define TE_FUN(...) ((Rational(*)(__VA_ARGS__))n->function)
#define M(e) te_eval(n->parameters[e])


Rational te_eval(const te_expr *n) {
    if (!n) return RNAN();

    switch(TYPE_MASK(n->type)) {
        case TE_CONSTANT: return n->value;
        case TE_VARIABLE: return *n->bound;

        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
            switch(ARITY(n->type)) {
                case 0: return TE_FUN(void)();
                case 1: return TE_FUN(Rational)(M(0));
                case 2: return TE_FUN(Rational, Rational)(M(0), M(1));
                case 3: return TE_FUN(Rational, Rational, Rational)(M(0), M(1), M(2));
                case 4: return TE_FUN(Rational, Rational, Rational, Rational)(M(0), M(1), M(2), M(3));
                case 5: return TE_FUN(Rational, Rational, Rational, Rational, Rational)(M(0), M(1), M(2), M(3), M(4));
                case 6: return TE_FUN(Rational, Rational, Rational, Rational, Rational, Rational)(M(0), M(1), M(2), M(3), M(4), M(5));
                case 7: return TE_FUN(Rational, Rational, Rational, Rational, Rational, Rational, Rational)(M(0), M(1), M(2), M(3), M(4), M(5), M(6));
                default: return RNAN();
            }

        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            switch(ARITY(n->type)) {
                case 0: return TE_FUN(void*)(n->parameters[0]);
                case 1: return TE_FUN(void*, Rational)(n->parameters[1], M(0));
                case 2: return TE_FUN(void*, Rational, Rational)(n->parameters[2], M(0), M(1));
                case 3: return TE_FUN(void*, Rational, Rational, Rational)(n->parameters[3], M(0), M(1), M(2));
                case 4: return TE_FUN(void*, Rational, Rational, Rational, Rational)(n->parameters[4], M(0), M(1), M(2), M(3));
                case 5: return TE_FUN(void*, Rational, Rational, Rational, Rational, Rational)(n->parameters[5], M(0), M(1), M(2), M(3), M(4));
                case 6: return TE_FUN(void*, Rational, Rational, Rational, Rational, Rational, Rational)(n->parameters[6], M(0), M(1), M(2), M(3), M(4), M(5));
                case 7: return TE_FUN(void*, Rational, Rational, Rational, Rational, Rational, Rational, Rational)(n->parameters[7], M(0), M(1), M(2), M(3), M(4), M(5), M(6));
                default: return RNAN();
            }

        default: return RNAN();
    }

}

#undef TE_FUN
#undef M

static void optimize(te_expr *n) {
    /* Evaluates as much as possible. */
    if (n->type == TE_CONSTANT) return;
    if (n->type == TE_VARIABLE) return;

    /* Only optimize out functions flagged as pure. */
    if (IS_PURE(n->type)) {
        const int arity = ARITY(n->type);
        int known = 1;
        int i;
        for (i = 0; i < arity; ++i) {
            optimize(n->parameters[i]);
            if (((te_expr*)(n->parameters[i]))->type != TE_CONSTANT) {
                known = 0;
            }
        }
        if (known) {
            const Rational value = te_eval(n);
            te_free_parameters(n);
            n->type = TE_CONSTANT;
            n->value = value;
        }
    }
}


te_expr *te_compile(const char *expression, const te_variable *variables, int var_count, int *error) {
    state s;
    s.start = s.next = expression;
    s.lookup = variables;
    s.lookup_len = var_count;

    next_token(&s);
    te_expr *root = list(&s);

    if (s.type != TOK_END) {
        te_free(root);
        if (error) {
            *error = (s.next - s.start);        
            if (*error == 0) *error = 1;
        }
        return 0;
    } else {
        optimize(root);
        if (error) *error = 0;
        return root;
    }
}

Rational te_interp(const char *expression, int *error) {
    te_expr *n = te_compile(expression, 0, 0, error);
    Rational ret;
    if (n) {
        ret = te_eval(n);
        te_free(n);
    } else {
        ret = RNAN();
    }
    return ret;
}

static void pn (const te_expr *n, int depth) {
    int i, arity;
    //printff("%*s", depth, "");

    switch(TYPE_MASK(n->type)) {
    case TE_CONSTANT: /*printff("%f\n", n->value);*/ break;
    case TE_VARIABLE: /*printff("bound %p\n", n->bound);*/ break;

    case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
    case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
    case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
    case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
         arity = ARITY(n->type);
         //printff("f%d", arity);
         /*for(i = 0; i < arity; i++) {
             printf(" %p", n->parameters[i]);
         }
		 
		 
         printf("\n");*/
         for(i = 0; i < arity; i++) {
             pn(n->parameters[i], depth + 1);
         }
         break;
    }
}


void te_print(const te_expr *n) {
    pn(n, 0);
}
