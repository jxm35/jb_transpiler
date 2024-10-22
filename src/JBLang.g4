grammar JBLang;

program
    : statement* EOF
    ;

statement
    : functionDecl
    | varDecl
    | spawnStmt
    | returnStmt
    | exprStmt
    | block
    ;

functionDecl
    : 'func' IDENTIFIER '(' paramList? ')' typeSpec? block
    ;

paramList
    : param (',' param)*
    ;

param
    : IDENTIFIER typeSpec
    ;

typeSpec
    : IDENTIFIER  // For now, just simple types like 'int'
    ;

varDecl
    : 'var' IDENTIFIER typeSpec? ('=' expression)? ';'
    ;

spawnStmt
    : 'spawn' expression ';'  // expression must be a function call
    ;

returnStmt
    : 'return' expression? ';'
    ;

exprStmt
    : expression ';'
    ;

block
    : '{' statement* '}'
    ;

expression
    : primary                                       # PrimaryExpr
    | expression '.' IDENTIFIER                     # MemberExpr
    | functionCall                                 # FuncCallExpr
    | expression op=('*'|'/') expression          # MulDivExpr
    | expression op=('+'|'-') expression          # AddSubExpr
    | expression op=('=='|'!='|'<'|'>') expression# CompareExpr
    | expression '=' expression                    # AssignExpr
    | IDENTIFIER                                   # VarExpr
    | literal                                      # LiteralExpr
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

INTEGER : [0-9]+ ;
STRING  : '"' (~["])* '"' ;
IDENTIFIER : [a-zA-Z_][a-zA-Z0-9_]* ;
WS : [ \t\r\n]+ -> skip ;
COMMENT : '//' ~[\r\n]* -> skip ;
