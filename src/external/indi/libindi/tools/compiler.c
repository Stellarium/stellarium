/* module to compile and execute a c-style arithmetic expression made of INDI
 * operands. operand names must contain 2 dots and be surrounded by quotes.
 * The expression is compiled and the names of each operand are stored. The
 * values of an operand can be set later by name. Evaluation uses the last
 * known operand value.
 *
 * one reason this is so nice and tight is that all opcodes are the same size
 * (an int) and the tokens the parser returns are directly usable as opcodes.
 * constants and variables are compiled as an opcode with an offset into the
 * auxiliary consts and vars arrays.
 *
 * this is not reentrant, but new expressions can be compiled as desired.
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int next_token(void);
static int chk_funcs(void);
static void skip_double(void);
static int compile(int prec);
static int execute(double *result);
static int parse_fieldname(char name[], int len);

/* parser tokens and opcodes, as necessary */
enum
{
    HALT, /* HALT = 0 serves as good initial default */

    /* binary operators (precedences in table, below) */
    ADD,
    SUB,
    MULT,
    DIV,
    AND,
    OR,
    GT,
    GE,
    EQ,
    NE,
    LT,
    LE,

    /* unary ops, precedence all UNI_PREC */
    NEG,
    NOT,

    /* symantically operands, ie, constants, variables and all functions */
    CONST,
    VAR,
    ABS,
    FLOOR,
    SIN,
    COS,
    TAN,
    ASIN,
    ACOS,
    ATAN,
    PITOK, /* built-in constant, pi */
    DEGRAD,
    RADDEG,
    LOG,
    LOG10,
    EXP,
    SQRT,
    POW,
    ATAN2,
};

/* purely tokens - never get compiled as such */
#define LPAREN 255
#define RPAREN 254
#define COMMA  253
#define ERR    (-1)

/* precedence of each of the binary operators.
 * in case of a tie, compiler associates left-to-right.
 * N.B. each entry's index must correspond to its #define!
 */
static int precedence[] = { 0, 5, 5, 6, 6, 2, 1, 4, 4, 3, 3, 4, 4 };
#define UNI_PREC 7 /* unary ops have highest precedence */

/* execute-time operand stack */
#define MAX_STACK 32
static double stack[MAX_STACK], *sp;

/* space for compiled opcodes - the "program".
 * opcodes go in lower 8 bits.
 * when an opcode has an operand (as CONST and VAR) it is really an array
 *   index in the remaining upper bits.
 */
#define MAX_PROG 32
static int program[MAX_PROG], *pc;
#define OP_SHIFT 8
#define OP_MASK  0xff

/* auxiliary operand info.
 * the operands (all but lower 8 bits) of CONST and VAR are really indeces
 * into these arrays. thus, no point in making them any longer than you have
 * bits more than 8 in your machine's int to index into it, ie, make
 *    MAX_OPX < 1 << ((sizeof(int)-1)*8)
 */
#define MAX_OPX   64 /* max number of operands */
#define MAXFLDLEN 64 /* longest allowed operand name */
typedef struct
{
    int set;              /* 1 when v has been set */
    char name[MAXFLDLEN]; /* name of operand */
    double v;             /* last known value of this operand */
} Var;
static Var vars[MAX_OPX];      /* operands */
static int nvars;              /* number of vars[] in actual use */
static double consts[MAX_OPX]; /* constants */
static int nconsts;            /* number of consts[] in actual use */

/* these are global just for easy/rapid access */
static int parens_nest;      /* to check that parens end up nested */
static char *err_msg;        /* caller provides storage; we point at it with this */
static char *cexpr, *lcexpr; /* pointers that move along caller's expression */

/* compile the given c-style expression.
 * return 0 if ok, else return -1 and a reason message in errbuf.
 */
int compileExpr(char *exp, char *errbuf)
{
    /* init the globals.
	 * also delete any flogs used in the previous program.
	 */
    cexpr   = exp;
    err_msg = errbuf;
    pc      = program;
    nvars = nconsts = 0;
    parens_nest     = 0;

    pc = program;
    if (compile(0) == ERR)
    {
        (void)sprintf(err_msg + strlen(err_msg), " near `%.10s'", lcexpr);
        return (-1);
    }
    if (pc == program)
    {
        (void)sprintf(err_msg, "Null program");
        return (-1);
    }
    *pc++ = HALT;
    return (0);
}

/* execute the expression previously compiled with compileExpr().
 * return 0 with *vp set to the answer if ok, else return -1 with a reason
 * why not message in errbuf.
 */
int evalExpr(double *vp, char *errbuf)
{
    err_msg = errbuf;
    sp      = stack + MAX_STACK; /* grows towards lower addresses */
    pc      = program;
    return (execute(vp));
}

/* set the value for an operand with the given name to the given value.
 * return 0 if found else -1.
 */
int setOperand(char *name, double valu)
{
    int i;

    for (i = 0; i < nvars; i++)
    {
        if (strcmp(name, vars[i].name) == 0)
        {
            vars[i].v   = valu;
            vars[i].set = 1;
            return (0);
        }
    }

    return (-1);
}

/* return 0 if all operands are set, else -1 */
int allOperandsSet()
{
    int i;

    for (i = 0; i < nvars; i++)
        if (!vars[i].set)
            return (-1);
    return (0);
}

/* return a malloced array of each operand name.
 * N.B. caller must free array, and not modify names.
 */
int getAllOperands(char ***ops)
{
    int i;

    *ops = (char **)malloc(nvars * sizeof(char *));

    for (i = 0; i < nvars; i++)
        (*ops)[i] = vars[i].name;

    return (nvars);
}

/* return a malloced array of each initialized operand name.
 * N.B. caller must free array, and not modify the names.
 */
int getSetOperands(char ***ops)
{
    int i, n;

    *ops = (char **)malloc(nvars * sizeof(char *));

    for (n = i = 0; i < nvars; i++)
        if (vars[i].set)
            (*ops)[n++] = vars[i].name;

    return (n);
}

/* return a malloced array of each uninitialized operand name.
 * N.B. caller must free array, and not modify the names.
 */
int getUnsetOperands(char ***ops)
{
    int i, n;

    *ops = (char **)malloc(nvars * sizeof(char *));

    for (n = i = 0; i < nvars; i++)
        if (!vars[i].set)
            (*ops)[n++] = vars[i].name;

    return (n);
}

/* called when each different field is written.
 * this is just called by srch_log() to hide the fact from users of srch*
 * that srch is really using our vars array to store values.
 * since this gets called for all fields, it's not an error to not find name.
 * don't stop when see the first one because a term might appear more than once.
 */
void compiler_log(char* name, double value)
{
    for (Var *vp = vars; vp < &vars[nvars]; vp++)
        if (strcmp(vp->name, name) == 0)
            vp->v = value;
}

/* get and return the opcode corresponding to the next token.
 * leave with lcexpr pointing at the new token, cexpr just after it.
 * also watch for mismatches parens and proper operator/operand alternation.
 */
static int next_token()
{
    static const char toomv[] = "More than %d variables";
    static const char toomc[] = "More than %d constants";
    static const char badop[] = "Illegal operator";
    int tok                   = ERR; /* just something illegal */
    char c;

    while (isspace(c = *cexpr))
        cexpr++;
    lcexpr = cexpr++;

    /* mainly check for a binary operator */
    switch (c)
    {
        case ',':
            tok = COMMA;
            break;
        case '\0':
            --cexpr;
            tok = HALT;
            break; /* keep returning HALT */
        case '+':
            tok = ADD;
            break; /* compiler knows when it's really unary */
        case '-':
            tok = SUB;
            break; /* compiler knows when it's really negate */
        case '*':
            tok = MULT;
            break;
        case '/':
            tok = DIV;
            break;
        case '(':
            parens_nest++;
            tok = LPAREN;
            break;
        case ')':
            if (--parens_nest < 0)
            {
                (void)sprintf(err_msg, "Too many right parens");
                return (ERR);
            }
            else
                tok = RPAREN;
            break;
        case '|':
            if (*cexpr == '|')
            {
                cexpr++;
                tok = OR;
            }
            else
            {
                (void)strcpy(err_msg, badop);
                return (ERR);
            }
            break;
        case '&':
            if (*cexpr == '&')
            {
                cexpr++;
                tok = AND;
            }
            else
            {
                (void)strcpy(err_msg, badop);
                return (ERR);
            }
            break;
        case '=':
            if (*cexpr == '=')
            {
                cexpr++;
                tok = EQ;
            }
            else
            {
                (void)strcpy(err_msg, badop);
                return (ERR);
            }
            break;
        case '!':
            if (*cexpr == '=')
            {
                cexpr++;
                tok = NE;
            }
            else
            {
                tok = NOT;
            }
            break;
        case '<':
            if (*cexpr == '=')
            {
                cexpr++;
                tok = LE;
            }
            else
                tok = LT;
            break;
        case '>':
            if (*cexpr == '=')
            {
                cexpr++;
                tok = GE;
            }
            else
                tok = GT;
            break;
    }

    if (tok != ERR)
        return (tok);

    /* not op so check for a constant, variable or function */
    if (isdigit(c) || c == '.')
    {
        /* looks like a constant.
	     * leading +- already handled
	     */
        if (nconsts > MAX_OPX)
        {
            (void)sprintf(err_msg, toomc, MAX_OPX);
            return (ERR);
        }
        consts[nconsts] = atof(lcexpr);
        tok             = CONST | (nconsts++ << OP_SHIFT);
        skip_double();
    }
    else if (isalpha(c))
    {
        /* check list of functions */
        tok = chk_funcs();
        if (tok == ERR)
        {
            (void)sprintf(err_msg, "Bad function");
            return (ERR);
        }
    }
    else if (c == '"')
    {
        /* a variable */
        if (nvars > MAX_OPX)
        {
            (void)sprintf(err_msg, toomv, MAX_OPX);
            return (ERR);
        }
        if (parse_fieldname(vars[nvars].name, MAXFLDLEN) < 0)
        {
            (void)sprintf(err_msg, "Bad field");
            return (ERR);
        }
        else
            tok = VAR | (nvars++ << OP_SHIFT);
    }

    if (tok != ERR)
        return (tok);

    /* what the heck is it? */
    (void)sprintf(err_msg, "Syntax error");
    return (ERR);
}

/* return funtion token, else ERR.
 * if find one, update cexpr too.
 */
static int chk_funcs()
{
    static struct
    {
        char *st_name;
        int st_tok;
    } symtab[] = {
        /* be sure to put short names AFTER longer ones.
	      * otherwise, order does not matter.
	      */
        { "abs", ABS },   { "floor", FLOOR }, { "acos", ACOS },     { "asin", ASIN },     { "atan2", ATAN2 },
        { "atan", ATAN }, { "cos", COS },     { "degrad", DEGRAD }, { "exp", EXP },       { "log10", LOG10 },
        { "log", LOG },   { "pi", PITOK },    { "pow", POW },       { "raddeg", RADDEG }, { "sin", SIN },
        { "sqrt", SQRT }, { "tan", TAN },
    };
    for (int i = 0; i < (int)(sizeof(symtab) / sizeof(symtab[0])); i++)
    {
        int l = strlen(symtab[i].st_name);

        if (strncmp(lcexpr, symtab[i].st_name, l) == 0)
        {
            cexpr += l - 1;
            return (symtab[i].st_tok);
        }
    }
    return (ERR);
}

/* move cexpr on past a double.
 * allow sci notation.
 * no need to worry about a leading '-' or '+' but allow them after an 'e'.
 * TODO: this handles all the desired cases, but also admits a bit too much
 *   such as things like 1eee2...3. geeze; to skip a double right you almost
 *   have to go ahead and crack it!
 */
static void skip_double()
{
    int sawe = 0; /* so we can allow '-' or '+' right after an 'e' */

    for (;;)
    {
        char c = *cexpr;
        if (isdigit(c) || c == '.' || (sawe && (c == '-' || c == '+')))
        {
            sawe = 0;
            cexpr++;
        }
        else if (c == 'e')
        {
            sawe = 1;
            cexpr++;
        }
        else
            break;
    }
}

/* call this whenever you want to dig out the next (sub)expression.
 * keep compiling instructions as long as the operators are higher precedence
 * than prec (or until see HALT, COMMA or RPAREN) then return that
 * "look-ahead" token.
 * if error, fill in a message in err_msg[] and return ERR.
 */
static int compile(prec) int prec;
{
    int expect_binop = 0; /* set after we have seen any operand.
				 * used by SUB so it can tell if it really 
				 * should be taken to be a NEG instead.
				 */
    int tok          = next_token();
    int *oldpc;

    for (;;)
    {
        int p;
        if (tok == ERR)
            return (ERR);
        if (pc - program >= MAX_PROG)
        {
            (void)sprintf(err_msg, "Program is too long");
            return (ERR);
        }

        /* check for special things like functions, constants and parens */
        switch (tok & OP_MASK)
        {
            case COMMA:
                return (tok);
            case HALT:
                return (tok);
            case ADD:
                if (expect_binop)
                    break; /* procede with binary addition */
                /* just skip a unary positive(?) */
                tok = next_token();
                if (tok == HALT)
                {
                    (void)sprintf(err_msg, "Term expected after unary +");
                    return (ERR);
                }
                continue;
            case SUB:
                if (expect_binop)
                    break; /* procede with binary subtract */
                oldpc = pc;
                tok   = compile(UNI_PREC);
                if (oldpc == pc)
                {
                    (void)sprintf(err_msg, "Term expected after unary -");
                    return (ERR);
                }
                *pc++        = NEG;
                expect_binop = 1;
                continue;
            case NOT:
                oldpc = pc;
                tok   = compile(UNI_PREC);
                if (oldpc == pc)
                {
                    (void)sprintf(err_msg, "Term expected after unary !");
                    return (ERR);
                }
                *pc++        = NOT;
                expect_binop = 1;
                continue;
            /* one-arg functions */
            case ABS:
            case FLOOR:
            case SIN:
            case COS:
            case TAN:
            case ASIN:
            case ACOS:
            case ATAN:
            case DEGRAD:
            case RADDEG:
            case LOG:
            case LOG10:
            case EXP:
            case SQRT:
                /* eat up the function's parenthesized argument */
                if (next_token() != LPAREN)
                {
                    (void)sprintf(err_msg, "expecting '(' after function");
                    return (ERR);
                }
                oldpc = pc;
                if (compile(0) != RPAREN || oldpc == pc)
                {
                    (void)sprintf(err_msg, "1-arg function arglist error");
                    return (ERR);
                }
                *pc++        = tok;
                tok          = next_token();
                expect_binop = 1;
                continue;
            /* two-arg functions */
            case POW:
            case ATAN2:
                /* eat up the function's parenthesized arguments */
                if (next_token() != LPAREN)
                {
                    (void)sprintf(err_msg, "Saw a built-in function: expecting (");
                    return (ERR);
                }
                oldpc = pc;
                if (compile(0) != COMMA || oldpc == pc)
                {
                    (void)sprintf(err_msg, "1st of 2-arg function arglist error");
                    return (ERR);
                }
                oldpc = pc;
                if (compile(0) != RPAREN || oldpc == pc)
                {
                    (void)sprintf(err_msg, "2nd of 2-arg function arglist error");
                    return (ERR);
                }
                *pc++        = tok;
                tok          = next_token();
                expect_binop = 1;
                continue;
            /* constants and variables are just like 0-arg functions w/o ()'s */
            case CONST:
            case PITOK:
            case VAR:
                *pc++        = tok;
                tok          = next_token();
                expect_binop = 1;
                continue;
            case LPAREN:
                oldpc = pc;
                if (compile(0) != RPAREN)
                {
                    (void)sprintf(err_msg, "Unmatched left paren");
                    return (ERR);
                }
                if (oldpc == pc)
                {
                    (void)sprintf(err_msg, "Null expression");
                    return (ERR);
                }
                tok          = next_token();
                expect_binop = 1;
                continue;
            case RPAREN:
                return (RPAREN);
        }

        /* everything else is a binary operator */
        if (tok < ADD || tok > LE)
        {
            printf("Bug! Bogus token: %d\n", tok);
            abort();
        }
        p = precedence[tok];
        if (p > prec)
        {
            int newtok;
            oldpc  = pc;
            newtok = compile(p);
            if (newtok == ERR)
                return (ERR);
            if (oldpc == pc)
            {
                (void)strcpy(err_msg, "Term or factor expected");
                return (ERR);
            }
            *pc++        = tok;
            expect_binop = 1;
            tok          = newtok;
        }
        else
            return (tok);
    }
}

/* "run" the program[] compiled with compile().
 * if ok, return 0 and the final result,
 * else return -1 with a reason why not message in err_msg.
 */
static int execute(result) double *result;
{
    int instr;

    do
    {
        instr = *pc++;
        switch (instr & OP_MASK)
        {
            /* put these in numberic order so hopefully even the dumbest
	     * compiler will choose to use a jump table, not a cascade of ifs.
	     */
            case HALT:
                break; /* outer loop will stop us */
            case ADD:
                sp[1] = sp[1] + sp[0];
                sp++;
                break;
            case SUB:
                sp[1] = sp[1] - sp[0];
                sp++;
                break;
            case MULT:
                sp[1] = sp[1] * sp[0];
                sp++;
                break;
            case DIV:
                sp[1] = sp[1] / sp[0];
                sp++;
                break;
            case AND:
                sp[1] = sp[1] && sp[0] ? 1 : 0;
                sp++;
                break;
            case OR:
                sp[1] = sp[1] || sp[0] ? 1 : 0;
                sp++;
                break;
            case GT:
                sp[1] = sp[1] > sp[0] ? 1 : 0;
                sp++;
                break;
            case GE:
                sp[1] = sp[1] >= sp[0] ? 1 : 0;
                sp++;
                break;
            case EQ:
                sp[1] = sp[1] == sp[0] ? 1 : 0;
                sp++;
                break;
            case NE:
                sp[1] = sp[1] != sp[0] ? 1 : 0;
                sp++;
                break;
            case LT:
                sp[1] = sp[1] < sp[0] ? 1 : 0;
                sp++;
                break;
            case LE:
                sp[1] = sp[1] <= sp[0] ? 1 : 0;
                sp++;
                break;
            case NEG:
                *sp = -*sp;
                break;
            case NOT:
                *sp = (double)(*sp == 0);
                break;
            case CONST:
                *--sp = consts[instr >> OP_SHIFT];
                break;
            case VAR:
                *--sp = vars[instr >> OP_SHIFT].v;
                break;
            case PITOK:
                *--sp = 4.0 * atan(1.0);
                break;
            case ABS:
                *sp = fabs(*sp);
                break;
            case FLOOR:
                *sp = floor(*sp);
                break;
            case SIN:
                *sp = sin(*sp);
                break;
            case COS:
                *sp = cos(*sp);
                break;
            case TAN:
                *sp = tan(*sp);
                break;
            case ASIN:
                *sp = asin(*sp);
                break;
            case ACOS:
                *sp = acos(*sp);
                break;
            case ATAN:
                *sp = atan(*sp);
                break;
            case DEGRAD:
                *sp *= atan(1.0) / 45.0;
                break;
            case RADDEG:
                *sp *= 45.0 / atan(1.0);
                break;
            case LOG:
                *sp = log(*sp);
                break;
            case LOG10:
                *sp = log10(*sp);
                break;
            case EXP:
                *sp = exp(*sp);
                break;
            case SQRT:
                *sp = sqrt(*sp);
                break;
            case POW:
                sp[1] = pow(sp[1], sp[0]);
                sp++;
                break;
            case ATAN2:
                sp[1] = atan2(sp[1], sp[0]);
                sp++;
                break;
            default:
                (void)sprintf(err_msg, "Bug! bad opcode: 0x%x", instr);
                return (-1);
        }
        if (sp < stack)
        {
            (void)sprintf(err_msg, "Runtime stack overflow");
            return (-1);
        }
        else if (sp - stack > MAX_STACK)
        {
            (void)sprintf(err_msg, "Bug! runtime stack underflow");
            return (-1);
        }
    } while (instr != HALT);

    /* result should now be on top of stack */
    if (sp != &stack[MAX_STACK - 1])
    {
        (void)sprintf(err_msg, "Bug! stack has %ld items", (long int)(MAX_STACK - (sp - stack)));
        return (-1);
    }
    *result = *sp;
    return (0);
}

/* starting with lcexpr pointing at a string expected to be a field name,
 * ie, at a '"', fill into up to the next '"' into name[], including trailing 0.
 * if there IS no '"' within len-1 chars, return -1, else 0.
 * the only sanity check is the string contains two dots.
 * when return, leave lcexpr alone but move cexpr to just after the second '"'.
 */
static int parse_fieldname(name, len) char name[];
int len;
{
    char c    = '\0';
    int ndots = 0;

    cexpr = lcexpr + 1;
    while (--len > 0 && (c = *cexpr++) != '"' && c)
    {
        *name++ = c;
        ndots += c == '.';
    }
    if (len == 0 || c != '"' || ndots != 2)
        return (-1);
    *name = '\0';
    return (0);
}
