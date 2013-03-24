/* ANSI C Grammar for ANTLR3.

   Copyright (C) 2013   Andrew Moss.  Blekinge Institute of Technology.
   This code is licensed under the LGPLv2.1. See the included LICENSE for details.

   Parses ANSI C 2011, but ignores several problems that can be solved inside
   a grammar using semantic actions. This produces a target-independent grammar
   that creates a parser for use in C / Java / Python where the additional
   checks can be done as a subsequent pass (i.e resolve identifiers/types using
   the symbol table.

   Parts of this work are based on the C language grammar maintained by Jutta 
   Degener, and on the ANTLR translation by Terence Parr, other reference sources
   include the SGI C Language Reference, which we sadly cannot included in the
   repository because of copyright restrictions, but which was available at:
     http://techpubs.sgi.com/library/manuals/0000/007-0701-150/pdf/007-0701-150.pdf
   Although the C Library Reference guide by Eric Huss has a different focus the
   first chapter is a good specification of the language:
     http://www.acm.uiuc.edu/webmonkeys/book/c_guide/
*/

grammar REPLACENAME;      // Overwritten by Makefile
options {
  backtrack=true;
  language=REPLACELANG;   // Overwritten by Makefile
  output=AST;
}

tokens {
  //BOLLOCKS;   uncomment to break????
  DECL;
  FUNC;
  PARAM;
  TYPE;
  DECLPAR;
}


translationUnit : externDecl+
                ;

externDecl : functionDef
           | declaration SEMI
           ;

// (optionally) Initialised declarators
initDecl : declarator initialiser? 
         -> ^(DECL declarator initialiser?)
         ;

// This is where we account for typedefs. Normally a grammar would use a real symbol
// table and semantic actions to check on-the-fly if the ident is a declared type.
// In order to keep this grammar language independent we allow an arbitrary IDENT
// as a type and defer the validity check until a later pass (thus no complex semantic
// actions in the grammar).
typeWrapper : typeSpecifier+ 
            | IDENT 
            ;

// These three entities (variable declarations, function definition parameters and
// function prototype parameters) are each variations on defining a type.
declaration : storageClass? typeQualifier? typeWrapper initDecl (COMMA initDecl)*
            -> ^(DECL storageClass? typeWrapper typeQualifier? initDecl+) ;
paramDecl   :               typeWrapper STAR* IDENT 
            -> ^(PARAM typeWrapper ^(DECL STAR* IDENT)) ;
protoDecl :               typeWrapper typeQualifier? STAR*
          -> ^(PARAM typeWrapper typeQualifier STAR*);

// A declarator binds a form and name to a storage and type within a scope.
// e.g. it specifies a kind of thing (as opposed to the type of a thing).
// The standard splits this element into two-levels, not sure why... they are
// left-recursive so follow Parr's transformation onto repeating suffixes
// This is still far less mesy than trying to resolve IDENTs/types during the parse...
declarator  : (STAR typeQualifier?)* IDENT declTail*;
declTail    : OPENPAR declPar CLOSEPAR                      // Fold cases for simplicity
            -> ^(DECLPAR declPar)
            | OPENSQ constExpr CLOSESQ        // Arrays
            ;
declPar     : declarator                   // Precedence only
            | paramDecl (COMMA paramDecl)* // Prototypes with idents 
            -> paramDecl+
            | protoDecl (COMMA protoDecl)* // Prototypes with no idents
            -> protoDecl+
            | STAR IDENT
            -> ^(PARAM STAR IDENT)    // Can't use new token, breaks parser !?!
            ;




// Todo: check declSpecs replacement
functionDef : storageClass? typeSpecifier* typeQualifier? IDENT OPENPAR (paramDecl (COMMA paramDecl)*)? CLOSEPAR compoundStmt
             -> ^(FUNC IDENT compoundStmt paramDecl* storageClass? typeSpecifier* typeQualifier?)
            ;

// Replaced above?
//paramDecl : declSpecs declarator 
//          -> ^(PARAM declarator declSpecs)
//          ;


initialiser : EQUALS (constExpr | arrayInit);

arrayInit : OPENBRA constExpr (COMMA constExpr)* CLOSEBRA ;

// Type can be prefixed by one of
storageClass : TYPEDEF      // Not a variable then.
             | EXTERN       // Pull in at link-time
             | STATIC       // Lifetime of program
             | AUTO         // Default storage - lifetime of scope
             | REGISTER     // Largely deprecated, treated as hint
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

typeQualifier : CONST
              | VOLATILE
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



constExpr : NUM | STR | CHARLIT;

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
OPENCOM  : '/*' ;
CLOSECOM : '*/' ;

VOID     : 'void' ;
CHAR     : 'char' ;
SHORT    : 'short' ;
INT      : 'int' ;
LONG     : 'long' ;
FLOAT    : 'float' ;
DOUBLE   : 'double' ;
SIGNED   : 'signed' ;
UNSIGNED : 'unsigned' ;
TYPEDEF  : 'typedef';
EXTERN   : 'extern';
STATIC   : 'static';
AUTO     : 'auto';
REGISTER : 'register';
CONST    : 'const';
VOLATILE : 'volatile';

COMMA : ',' ;
STAR  : '*' ;
SEMI  : ';' ;
NUM : '-' DIGIT+ ('.' DIGIT+)?
    | DIGIT+ ('.' DIGIT+)?
    | '0x' ('0'..'9'|'a'..'f'|'A'..'F')+
    ;
IDENT : ALPHA (ALPHA | DIGIT)* ;

STR : '"' (~'"')* '"' ;
CHARLIT : '\'' ~('\'') '\'' ;

COM : '/' '/' (~'\n')* '\n' REPLACEHIDDEN
    | OPENCOM (options {greedy=false;}: .)* CLOSECOM REPLACEHIDDEN
    ;
WS : (' ' | '\n' | '\t')+ REPLACEHIDDEN
   ;
