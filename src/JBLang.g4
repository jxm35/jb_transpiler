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
    | ifStmt
    | whileStmt
    | typedefDecl    // Add this
    ;

typedefDecl
    : 'typedef' (typeSpec | structDecl) IDENTIFIER ';'
    ;

functionDecl
    : typeSpec IDENTIFIER '(' paramList? ')' block  // C-style: return type before function name
    ;

paramList
    : param (',' param)*
    ;

param
    : ((typeSpec IDENTIFIER) | arrayDecl)  // C-style: type before parameter name
    ;

typeSpec
    : 'void'
    | 'int'
    | 'bool'
    | 'string'
    | 'struct'? IDENTIFIER        // For struct types
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
    : ((typeSpec IDENTIFIER) | arrayDecl) ('=' expression)? ';'  // C-style: type before variable name
    ;

spawnStmt
    : 'spawn' expression ';'  // Kept for concurrent execution
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
    : primary                                       # PrimaryExpr
    | expression '.' IDENTIFIER                     # MemberExpr
    | functionCall                                  # FuncCallExpr
    | expression op=('*'|'/') expression           # MulDivExpr
    | expression op=('+'|'-') expression           # AddSubExpr
    | expression op=('=='|'!='|'<'|'>'|'<='|'>=') expression # CompareExpr
    | expression '=' expression                     # AssignExpr
    | IDENTIFIER                                    # VarExpr
    | literal                                       # LiteralExpr
    | 'new' typeSpec                                # NewExpr
    | expression '->' IDENTIFIER                    # PointerMemberExpr
    | '&' expression                               # AddressOfExpr
    | '*' expression                               # DereferenceExpr
    | expression '[' expression ']'                 # ArrayAccessExpr
    | '{' initializerList? '}'                         # StructInitExpr
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