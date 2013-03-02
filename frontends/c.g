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
}

translationUnit : externDecl+
                ;

externDecl : functionDecl
           | declaration
           ;

declaration : declSpecs? IDENT ';'
            ;

functionDecl : declSpecs? declarator /*declList?*/ compoundStmt
            ;


declSpecs : (storageClass | typeSpecifier | typeQualifier)+
          ;

declarator : IDENT ;

storageClass : 'typedef'
             | 'extern'
             | 'static'
             | 'auto'
             // | 'thread_local'
             | 'register'
             ;

typeSpecifier : 'void'
              | 'char'
              | 'short'
              | 'int'
              | 'long'
              | 'float'
              | 'double'
              | 'signed'
              | 'unsigned'
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

enumList : enum ( ',' enum )+
         ;

enum : IDENT
     | IDENT '=' constExpr
     ;


compoundStmt : '{' compoundStmt+ '}' ;



constExpr : NUM ;

ENUM : 'enum'
             ;

fragment DIGIT : '0'..'9'
               ;
fragment ALPHA : 'a'..'z'
               | 'A'..'Z'
               | '_'
               ;

NUM : DIGIT+
    ;

IDENT : ALPHA (ALPHA | DIGIT)+
      ;
