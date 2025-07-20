grammar JBLang;

program
    : preprocessorDirective* statement* EOF
    ;

preprocessorDirective
    : '#include' (STRING | INCLUDE_STRING )
    | '#define' IDENTIFIER (INTEGER | STRING)?
    ;


statement
    : functionDecl
    | varDecl
    | spawnStmt
    | returnStmt
    | exprStmt
    | block
    | structDecl
    | classDecl
    | ifStmt
    | whileStmt
    | forStmt
    | typedefDecl
    ;

typedefDecl
    : 'typedef' (typeSpec | structDecl) IDENTIFIER ';'
    ;

classDecl
    : 'class' IDENTIFIER (':' IDENTIFIER)? '{' classMember* '}' ';'?
    ;

classMember
    : ((typeSpec IDENTIFIER) | arrayDecl) ';'     # ClassField
    | typeSpec IDENTIFIER '(' paramList? ')' block # ClassMethod
    | IDENTIFIER '(' paramList? ')' (':' IDENTIFIER '(' argumentList? ')')? block # ClassConstructor
    ;

forStmt
    : 'for' '(' (varDecl | exprStmt | ';') expression? ';' expression? ')' statement
    ;

functionDecl
    : typeSpec IDENTIFIER '(' paramList? ')' block
    ;

paramList
    : param (',' param)*
    ;

param
    : ((typeSpec IDENTIFIER) | arrayDecl)
    ;

typeSpec
    : 'void'
    | 'int'
    | 'bool'
    | 'string'
    | ('struct'|'class')? IDENTIFIER
    | typeSpec '*'
    ;

arrayDecl : typeSpec  IDENTIFIER arraySize arraySize* ;

arraySize : (OPEN_BRACKET (INTEGER | IDENTIFIER) CLOSE_BRACKET) ;

structDecl
    : 'struct' IDENTIFIER '{' structMember* '}' ';'?
    ;

structMember
    : ((typeSpec IDENTIFIER) | arrayDecl) ';'
    ;

varDecl
    : ((typeSpec IDENTIFIER) | arrayDecl) ('=' expression)? ';'
    ;

spawnStmt
    : 'spawn' expression ';'
    ;

returnStmt
    : 'return' expression? ';'
    ;

exprStmt
    : expression ';'
    ;

ifStmt
    : 'if' '(' expression ')' statement ('else' statement)?
    ;

whileStmt
    : 'while' '(' expression ')' statement
    ;

block
    : '{' statement* '}'
    ;

expression
    : primary                                         # PrimaryExpr
    | expression '.' IDENTIFIER                       # MemberExpr
    | functionCall                                    # FuncCallExpr
    | expression op=('*'|'/') expression              # MulDivExpr
    | expression op=('+'|'-') expression              # AddSubExpr
    | expression op=('=='|'!='|'<'|'>'|'<='|'>=') expression # CompareExpr
    | expression '=' expression                        # AssignExpr
    | IDENTIFIER                                       # VarExpr
    | literal                                          # LiteralExpr
    | 'new' typeSpec                                   # NewExpr
    | 'new' IDENTIFIER '(' argumentList? ')'           # NewWithConstructorExpr
    | expression '->' IDENTIFIER                       # PointerMemberExpr
    | '&' expression                                   # AddressOfExpr
    | '*' expression                                   # DereferenceExpr
    | expression '[' expression ']'                    # ArrayAccessExpr
    | expression '->' IDENTIFIER '(' argumentList? ')' # MethodCallExpr
    | '{' initializerList? '}'                         # StructInitExpr
    | expression '++'                                  # PostIncrementExpr
    | expression '--'                                  # PostDecrementExpr
    | '++' expression                                  # PreIncrementExpr
    | '--' expression                                  # PreDecrementExpr
    ;

initializerList
    : initializer (',' initializer)* ','?
    ;

initializer
    : '.' IDENTIFIER '=' expression
    ;

functionCall
    : IDENTIFIER '(' argumentList? ')'
    ;

argumentList
    : expression (',' expression)*
    ;

primary
    : '(' expression ')'
    | IDENTIFIER
    | literal
    ;

literal
    : INTEGER
    | STRING
    | 'true'
    | 'false'
    ;

OPEN_BRACKET : '[' ;
CLOSE_BRACKET : ']' ;
INTEGER : [0-9]+ ;
STRING  : '"' (~["])* '"' ;
INCLUDE_STRING  : '<' (~["])* '>' '\n' ;
IDENTIFIER : [a-zA-Z_][a-zA-Z0-9_]* ;
WS : [ \t\r\n]+ -> skip ;
COMMENT : '//' ~[\r\n]* -> skip ;
