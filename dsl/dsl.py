import io
import logging
import unittest
from playlang import Action, ShowName, Parser, Token, Rule, Precedence, Scanner, Start, StaticTokenizer

logging.basicConfig(level='DEBUG')

class MismatchError(Exception):
    pass

def throw(e, *args):
    raise e(*args)

class AddressExpr(metaclass=Parser):
    HEX = Token(r'0x[0-9a-fA-F]+',
                action=lambda ctx: int(ctx.text[2:], 16))
    OCT = Token(r'0o[0-7]+',
                action=lambda ctx: int(ctx.text[2:], 8))
    BIN = Token(r'0b[01]+',
                action=lambda ctx: int(ctx.text[2:], 2))
    INTEGER = Token(r'[0-9]+',
                    action=int)

    REFERENCE = Token(r'\$[a-zA-Z_][a-zA-Z0-9_]*',
                 action=str,
                 show_name='Reference')

    MISMATCH = Token(r'.', discard=True)

    _ = Precedence.Left
    LOR = Token(r'\|\|')

    _ = Precedence.Left
    LAND = Token(r'&&')

    _ = Precedence.Left
    OR = Token(r'\|')
    
    _ = Precedence.Left
    XOR = Token(r'\^')

    _ = Precedence.Left
    AND = Token(r'&')

    _ = Precedence.Left
    EQ = Token(r'==')
    NE = Token(r'=!')

    _ = Precedence.Left
    GT = Token(r'>')
    GE = Token(r'>=')
    LT = Token(r'<')
    LE = Token(r'<=')
    
    _ = Precedence.Left
    LSHIFT = Token(r'<<')
    RSHIFT = Token(r'>>')

    _ = Precedence.Left
    PLUS = Token(r'\+', show_name='Plus')
    MINUS = Token(r'-')

    _ = Precedence.Left
    TIMES = Token(r'\*')
    DIVIDE = Token(r'\/')
    MOD = Token(r'%')

    _ = Precedence.Increase
    LPAR = Token(r'\(')
    RPAR = Token(r'\)')
    UMINUS = Token(r'-')
    NOT = Token(r'~')
    LNOT = Token(r'!')

    _ = Scanner(
        HEX, OCT, BIN, INTEGER,
        LAND, LOR, LNOT,
        AND, OR, NOT, XOR,
        EQ, NE, GT, GE, LT, LE, 
        LSHIFT, RSHIFT, 
        MOD, PLUS, MINUS, TIMES, DIVIDE, 
        LPAR, RPAR, REFERENCE, MISMATCH)

    @Action
    @staticmethod
    def MISMATCH(context):
        raise MismatchError(f'missmatch: {context.text}')

    @Rule(HEX)
    @staticmethod
    def NUMBER(context, num):
        pass

    @Rule(OCT)
    @staticmethod
    def NUMBER(context, num):
        pass

    @Rule(BIN)
    @staticmethod
    def NUMBER(context, num):
        pass
        
    @Rule(INTEGER)
    @staticmethod
    def NUMBER(context, hex):
        pass

    @Rule(NUMBER)
    @staticmethod
    def EXPR(context, value):
        context.steps.append(value)
        return value

    @Rule(REFERENCE)
    @staticmethod
    def EXPR(context, ref):
        context.steps.append(ref)
        return context.names[ref]

    @Rule(MINUS, EXPR, precedence=UMINUS)
    @staticmethod
    def EXPR(context, l, expr):
        context.steps.append(f'-{expr}')
        return -expr

    @Rule(NOT, EXPR)
    @Rule(LNOT, EXPR)
    @staticmethod
    def EXPR(context, l, expr):
        context.steps.append(f'-{expr}')
        return -expr

    @Rule(LPAR, EXPR, RPAR)
    @staticmethod
    def EXPR(context, l, expr, r):
        context.steps.append(f'({expr})')
        return expr

    @ShowName('Expression')
    @Rule(EXPR, PLUS, EXPR)
    @Rule(EXPR, MINUS, EXPR)
    @Rule(EXPR, TIMES, EXPR)
    @Rule(EXPR, DIVIDE, EXPR)
    @Rule(EXPR, MOD, EXPR)
    @Rule(EXPR, AND, EXPR)
    @Rule(EXPR, OR, EXPR)
    @Rule(EXPR, XOR, EXPR)
    @Rule(EXPR, LAND, EXPR)
    @Rule(EXPR, LOR, EXPR)
    @Rule(EXPR, GT, EXPR)
    @Rule(EXPR, GE, EXPR)
    @Rule(EXPR, LT, EXPR)
    @Rule(EXPR, LE, EXPR)
    @Rule(EXPR, EQ, EXPR)
    @Rule(EXPR, NE, EXPR)
    @Rule(EXPR, LSHIFT, EXPR)
    @Rule(EXPR, RSHIFT, EXPR)
    @staticmethod
    def EXPR(context, l_expr, opr, r_expr):
        code = f'{l_expr}{opr}{r_expr}'
        context.steps.append(code)
        return eval(code)

    _ = Start(EXPR)

    scanner = StaticTokenizer(default_action=lambda ctx: ctx.step(len(ctx.text)))

    def __init__(self):
        self.names = {}
        self.steps = []

    def parse_string(self, string):
        return AddressExpr.parse(AddressExpr.scanner(string), context=self)


def escape(s):
    if s == 'b': return '\b'
    if s == 'n': return '\n'
    if s == 't': return '\t'
    return s

class CommandParser(metaclass=Parser):
    STRING = Token(action=lambda ctx: ctx.value.getvalue())
    BARE_STRING_BEGIN_ESCAPE = Token(r'\\.', discard=True, action=lambda ctx: ctx.enter('bare_string', io.StringIO(escape(ctx.text[1]))))
    BARE_STRING_BEGIN_ANY = Token(r'.', discard=True, action=lambda ctx: ctx.enter('bare_string', io.StringIO(ctx.text)))
    BARE_STRING_ESCAPE = Token(r'\\.', discard=True, action=lambda ctx: ctx.value.write(escape(ctx.text[1])))
    BARE_STRING_CHAR_LPAR = Token(r'[^ \t\v\n\r]', trailing=r'[,()]', discard=True, action=lambda ctx: ctx.value.write(ctx.text))
    BARE_STRING_CHAR = Token(r'[^ \t\v\n\r]', discard=True, action=lambda ctx: ctx.value.write(ctx.text))
    BARE_STRING_END = Token(r'.|\n', discard=True, action=lambda ctx: ctx.leave()) # space
    BARE_STRING_EOF = Token(discard=True, is_eof=True, action=lambda ctx: ctx.leave())
    BARE_STRING_BOUNDARY = Token(r'[ ]', action=lambda ctx: ctx.leave())

    QUOTED_STRING_BEGIN = Token(r'"', discard=True, action=lambda ctx: ctx.enter('quoted_string', io.StringIO()))
    QUOTED_STRING_END = Token(r'"', discard=True, action=lambda ctx:ctx.leave())
    QUOTED_STRING_ESCAPE = Token(r'\\n', discard=True, action=lambda ctx: ctx.value.write('\n'))
    QUOTED_STRING_CHAR = Token(r'.', discard=True, action=lambda ctx:ctx.value.write(ctx.text))

    BOUNDARY = Token(r'[ ]', discard=True)

    _ = Scanner(
        BOUNDARY,
        QUOTED_STRING_BEGIN,
        BARE_STRING_BEGIN_ESCAPE, BARE_STRING_BEGIN_ANY)

    _ = Scanner(
        BARE_STRING_ESCAPE,
        BARE_STRING_CHAR_LPAR,
        BARE_STRING_CHAR,
        BARE_STRING_END,
        BARE_STRING_EOF,
        BARE_STRING_BOUNDARY,
        name="bare_string",
        capture=STRING
    )

    _ = Scanner(QUOTED_STRING_END,
        QUOTED_STRING_ESCAPE,
        QUOTED_STRING_CHAR,
        name="quoted_string",
        capture=STRING)

    @Rule()
    def LIST(self):
        pass

    @Rule(STRING)
    def LIST(self, s):
        pass

    @Rule(LIST, STRING)
    def LIST(self, lst, s):
        pass

    _ = Start(LIST)

    tokenizer = StaticTokenizer(default_action=lambda ctx: ctx.step(len(ctx.text)))

    def __init__(self):
        pass

    def parse_string(self, string):
        return CommandParser.parse(CommandParser.tokenizer(string), context=self)

if __name__ == '__main__':
    from playlang.cplusplus import generate
    generate(AddressExpr)
    generate(CommandParser)
