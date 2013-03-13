/* ANSI C Grammar for ANTLR3.

   Copyright (C) 2013   Andrew Moss.  Blekinge Institute of Technology.
   This code is licensed under the LGPLv2.1. See the included LICENSE for details.

   Parses ANSI C 2011, but ignores several problems that can be solved inside
   a grammar using semantic actions. This produces a target-independent grammar
   that creates a parser for use in C / Java / Python where the additional
   checks can be done as a subsequent pass (i.e resolve identifiers/types using
   the symbol table.

   Parts of this work are based on the C language grammar maintained by Jutta 
   Degener, and on the ANTLR translation by Terence Parr.
*/

grammar REPLACENAME;      // Overwritten by Makefile
options {
  backtrack=true;
  language=REPLACELANG;   // Overwritten by Makefile
  output=AST;
}

tokens {
  DECL;
  FUNC;
}


translationUnit : externDecl+
                ;

externDecl : functionDecl
           | declaration
           ;

declaration : declSpecs? declarator initialiser? SEMI
            -> ^(DECL declarator declSpecs? initialiser?)
            ;

functionDecl : declSpecs? declarator OPENPAR paramList CLOSEPAR /*declList?*/ compoundStmt
             -> ^(FUNC declarator paramList compoundStmt declSpecs?)
            ;

paramList : paramDecl ( COMMA paramDecl )*
          ;

paramDecl : declSpecs declarator 
          ;


declSpecs : (storageClass | typeSpecifier | typeQualifier)+
          ;

declarator : IDENT 
           | IDENT OPENSQ constExpr CLOSESQ
           | STAR declarator
           ;

initialiser : EQUALS constExpr ;

storageClass : 'typedef'
             | 'extern'
             | 'static'
             | 'auto'
             // | 'thread_local'
             | 'register'
             ;

typeSpecifier : VOID
              | CHAR
              | SHORT
              | INT
              | LONG
              | FLOAT
              | DOUBLE
              | SIGNED
              | UNSIGNED
              | unionSpecifier
              | structSpecifier
              | enumSpecifier
              ;

typeQualifier : 'const'
              | 'volatile'
              ;

unionSpecifier : 'union' IDENT? '{' structDeclList '}'
               | 'union' IDENT
               ;

structSpecifier : 'struct' IDENT? '{' structDeclList '}'
                | 'struct' IDENT
                ;

structDeclList : declaration+
               ;

enumSpecifier : ENUM IDENT? '{' enumList '}'
              | ENUM IDENT
              ;

enumList : enumerator ( COMMA enumerator )+
         ;

enumerator : IDENT
     | IDENT '=' constExpr
     ;

notscope : ~(OPENBRA|CLOSEBRA) ;
compoundStmt : OPENBRA notscope* compoundStmt* notscope* CLOSEBRA 
            -> ^(OPENBRA compoundStmt*) 
             ;



constExpr : NUM | STR ;

ENUM : 'enum'
             ;

fragment DIGIT : '0'..'9'
               ;
fragment ALPHA : 'a'..'z'
               | 'A'..'Z'
               | '_'
               ;

OPENPAR  : '(' ;
CLOSEPAR : ')' ;
OPENBRA  : '{' ;
CLOSEBRA : '}' ;
OPENSQ   : '[' ;
CLOSESQ  : ']' ;
EQUALS   : '=' ;

VOID     : 'void' ;
CHAR     : 'char' ;
SHORT    : 'short' ;
INT      : 'int' ;
LONG     : 'long' ;
FLOAT    : 'float' ;
DOUBLE   : 'double' ;
SIGNED   : 'signed' ;
UNSIGNED : 'unsigned' ;

COMMA : ',' ;
STAR  : '*' ;
SEMI  : ';' ;
NUM : DIGIT+ ;
IDENT : ALPHA (ALPHA | DIGIT)* ;

STR : '"' (~'"')* '"' ;

WS : (' ' | '\n' | '\t')+ REPLACEHIDDEN
   ;
