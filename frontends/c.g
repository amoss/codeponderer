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
  DECL;
  FUNC;
  PARAM;
  TYPE;
  DECLPAR;
  FPTR;
  STATEMENT;
}


translationUnit : externDecl+
                ;

externDecl : functionDef
           | declaration SEMI
           | unionDef SEMI    // Pure definition (must include tag-name)
           | structDef SEMI   // Pure definition (must include tag-name)
           | STRUCT IDENT SEMI // Forward reference
           //-> ^(DECL ^(STRUCT IDENT))
           -> ^(STRUCT IDENT)
           | enumSpecifier SEMI     // Pure definition (defines values)
           | PREPRO
           ;

// (optionally) Initialised declarators
initDecl : declarator initialiser? 
         -> ^(DECL declarator initialiser?)
         ;

/* A typename / identifier ambiguity.
   Normally a grammar would use a real symbol-table and semantic actions to build state during parsing
   and query the state in the lexer to decide if a symbol is an IDENT or a TYPENAME (defined in an
   earlier typedef). This is horrific and causes an explosion of complexity in the grammar. One of the
   purposes of writing this grammar was to determine if we could avoid this slice of horror. The rule
   below is a simple counting machine that decides if the IDENT is a type or a name, without the need
   to embed symbol-tables in the check on-the-fly if the ident is a declared type. This means that the
   tree rewrites are the only processing required in the grammar, so it can be target language independent.
   TODO: There is still the more general case "foo*bar;" which needs to be covered when expressions and
         local declarations are handled
*/
declSpec : storageClass   declSpec?
         | typeQualifier  declSpec?
         | typeSpecifier+ declSpecPostType?
         | IDENT          declSpecPostType?
         | INLINE         declSpec?
         | '__inline'     declSpec?
         ;
declSpecPostType : storageClass  declSpecPostType?
                 | typeQualifier declSpecPostType?
                 | INLINE        declSpecPostType?
                 | '__inline'    declSpecPostType?
                 ;

// These three entities (variable declarations, function definition parameters and
// function prototype parameters) are each variations on defining a type. In the 
// spec the init-decl-list is optional, but this is just for the case that a struct
// is a pure definition without a declaration. Most compilers trap the other cases
// later. Here the init-decl-list must contain at least one item (even if just an IDENT)
// with a special case for struct definitions.
declaration : declSpec initDecl (COMMA initDecl)*
            -> ^(DECL declSpec initDecl+)
            | structSpecifier initDecl (COMMA initDecl)*
            -> ^(DECL structSpecifier initDecl+)
            | enumSpecifier
            -> ^(DECL enumSpecifier)      // TODO: Must be wrong, needs initDecl+ ??
            | declSpec COLON NUM    
            ;
protoDecl : declSpec STAR* fptrName OPENPAR declPar CLOSEPAR
          -> ^(PARAM declSpec STAR* fptrName ^(DECLPAR declPar) ) 
          | declSpec STAR* fptrName OPENPAR CLOSEPAR
          -> ^(PARAM declSpec STAR* fptrName )
          |               declSpec STAR* typeQualifier? IDENT? (OPENSQ notsq* CLOSESQ)?
          -> ^(PARAM declSpec STAR* IDENT?)
          | ELLIPSIS
          ;

// A declarator binds a form and name to a storage and type within a scope.
// e.g. it specifies a kind of thing (as opposed to the type of a thing).
// The standard splits this element into two-levels, not sure why... they are
// left-recursive so follow Parr's transformation onto repeating suffixes
// This is still far less messy than trying to resolve IDENTs/types during the parse...
declarator  : (STAR typeQualifier?)* (fptrName|IDENT) declTail*;

fptrName    : OPENPAR STAR IDENT (OPENPAR declPar CLOSEPAR)? CLOSEPAR
            -> ^(FPTR STAR IDENT declPar?)
            ;
notsq : ~CLOSESQ;
declTail    : OPENPAR declPar? CLOSEPAR                      // Fold cases for simplicity
            -> ^(DECLPAR declPar?)
            //| OPENSQ constExpr? CLOSESQ        // Arrays
            | OPENSQ notsq* CLOSESQ        // Arrays
            -> ^(OPENSQ notsq* CLOSESQ)
            | COLON NUM   // Only valid inside bitfields, not generally
            ;
declPar     : protoDecl (COMMA protoDecl)* // Prototypes with optional idents
            -> protoDecl+
            | declarator                   // Precedence only 
            | VOID                         // Function proto
            ;



paramDecl   : declSpec declarator 
            -> ^(PARAM declSpec declarator)
            | ELLIPSIS
            ;

// Todo: check declSpecs replacement
functionDef : declSpec (STAR typeQualifier?)* 
              IDENT OPENPAR ((paramDecl (COMMA paramDecl)*)? | VOID) CLOSEPAR 
              OPENBRA statement* CLOSEBRA
             -> ^(FUNC IDENT ^(OPENBRA statement*) paramDecl* declSpec)
            ;

// Replaced above?
//paramDecl : declSpecs declarator 
//          -> ^(PARAM declarator declSpecs)
//          ;

// Need two-level split over balanced parathesis to handle commas in compound initialisers.
notinit      : ~(SEMI|COMMA|OPENBRA|CLOSEBRA) ; 
notinitInner : ~(SEMI|OPENBRA|CLOSEBRA) ; 
anyinit : notinit* (OPENBRA anyinitInside CLOSEBRA)* notinit* ;
anyinitInside : notinitInner* (OPENBRA anyinitInside CLOSEBRA notinitInner*)* ;
initialiser //: ASSIGN (constExpr | compoundInit)
            : ASSIGN anyinit
            -> ^(ASSIGN anyinit)
            ;


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
              | '__const'
              | '__restrict'
              | VOLATILE
              ;

unionSpecifier : UNION IDENT? OPENBRA structDeclList CLOSEBRA
               | UNION IDENT
               ;
unionDef  :   UNION  IDENT   OPENBRA structDeclList CLOSEBRA 
         -> ^(UNION  IDENT structDeclList ) ;
structDef :   STRUCT IDENT   OPENBRA structDeclList CLOSEBRA
         -> ^(STRUCT IDENT structDeclList ) ;

structSpecifier : STRUCT IDENT? OPENBRA structDeclList CLOSEBRA
                -> ^(STRUCT IDENT structDeclList)
                | STRUCT IDENT
                -> ^(STRUCT IDENT)
                ;

structDeclList : (declaration SEMI | PREPRO)+
               -> declaration+
               ;

enumSpecifier : ENUM IDENT? OPENBRA enumList CLOSEBRA
              -> ^(ENUM IDENT? enumList)
              | ENUM IDENT
              -> ^(ENUM IDENT)
              ;

enumList : enumerator ( COMMA enumerator )*
         -> enumerator+
         ;

//enumerator : IDENT '=' constExpr
//           -> ^(ENUM IDENT constExpr)
notenum : ~(COMMA|CLOSEBRA);
enumerator : IDENT '=' notenum+
           -> ^(ENUM IDENT)
           | IDENT
           -> ^(ENUM IDENT)
           ;

notscope  : ~(OPENBRA|CLOSEBRA) ;
compoundStmt : OPENBRA notscope* (compoundStmt notscope*)* CLOSEBRA 
            -> ^(OPENBRA compoundStmt*) 
             ;

notExpr : ~(OPENPAR|CLOSEPAR);
expr : OPENPAR notExpr* (expr notExpr*)* CLOSEPAR ;

goop: ~(OPENBRA|CLOSEBRA|SEMI);
statement : OPENBRA statement* CLOSEBRA
          | IF expr statement (ELSE statement)?
          -> ^(IF expr statement statement?)
          | WHILE expr statement
          -> ^(WHILE expr statement)
          | FOR expr statement
          -> ^(FOR expr statement)
          | SWITCH expr compoundStmt
          -> ^(SWITCH expr compoundStmt)
          | goop+ SEMI
          -> ^(STATEMENT goop+)
          | PREPRO
          ;



//compoundInit : OPENBRA constExpr (COMMA constExpr)* COMMA? CLOSEBRA ;
compoundInit : OPENBRA notscope* (compoundInit notscope*)* CLOSEBRA ;
constExpr : NUM 
          | STR 
          | CHARLIT 
          | IDENT 
          | AMP IDENT
          | compoundInit ;

fragment DIGIT : '0'..'9'
               ;
fragment ALPHA : 'a'..'z'
               | 'A'..'Z'
               | '_'
               ;
// GNU-c extensions that we simply want to ignore
GNU : '__attribute' '__'? (' ' | '\t')* '((' GNUbra '))' REPLACEHIDDEN ;
fragment GNUbra: (~('('|')') )* ( '(' GNUbra ')' )?;
GNUEXPR: '({' (options {greedy=false;}:.)* '})' REPLACEHIDDEN ;
GNUext : '__extension__' REPLACEHIDDEN ;

ASM: '__asm__' WS* '(' STR ')' REPLACEHIDDEN;

OPENPAR  : '(' ;
CLOSEPAR : ')' ;
OPENBRA  : '{' ;
CLOSEBRA : '}' ;
OPENSQ   : '[' ;
CLOSESQ  : ']' ;
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
INLINE   : 'inline';

STRUCT   : 'struct';
UNION    : 'union';
ENUM     : 'enum';

IF     : 'if';
ELSE   : 'else';
WHILE  : 'while';
FOR    : 'for';
SWITCH : 'switch';

COMMA : ',' ;
STAR  : '*' ;
SEMI  : ';' ;
NUM : '-' DIGIT+ ('.' DIGIT+)?
    | DIGIT+ ('.' DIGIT+)?
    | '0x' ('0'..'9'|'a'..'f'|'A'..'F')+
    ;
IDENT : ALPHA (ALPHA | DIGIT)* ;

STR : '"' ('\\\\' | '\\"' | ~'"')* '"' ;
CHARLIT : '\'' (~('\'') | ESC) '\'' ;

fragment ESC : '\\' ('a' | 'b' | 'v' | 'f' | 'r' | 't' | 'n' | '\'' | '\\' | DIGIT+) ;

// Preprocess commands will be awkward because of the interaction with whitespace
PREPRO: '#'  (' '|'\t')* ('include' | 'define' | 'undef' | 'if' | 'endif' | 'else' | 'elif' | 'error' 
              | 'pragma' | 'line') (~'\n')* '\n' ;
//HASHINCLUDE: '#' (' '|'\t')* 'include' (~'\n')* '\n' ;
//HASHDEFINE: '#' (' '|'\t')* 'define' (~'\n')* '\n' ;
//HASHUNDEF: '#' (' '|'\t')* 'undef' (~'\n')* '\n' ;
//HASHIF: '#' (' '|'\t')* 'if' (~'\n')* '\n' ;
//HASHENDIF: '#' (' '|'\t')* 'endif' (~'\n')* '\n' ;
//HASHELSE: '#' (' '|'\t')* 'else' (~'\n')* '\n' ;
//HASHELIF: '#' (' '|'\t')* 'elif' (~'\n')* '\n' ;
//HASHERROR: '#' (' '|'\t')* 'error' (~'\n')* '\n' ;
//HASHPRAGMA: '#' (' '|'\t')* 'pragma' (~'\n')* '\n' ;
//HASHLINE: '#' (' '|'\t')* 'line' (~'\n')* '\n' ;

COM : '/' '/' (~'\n')* '\n' REPLACEHIDDEN
    | OPENCOM (options {greedy=false;}: .)* CLOSECOM REPLACEHIDDEN
    ;
WS : (' ' | '\\\n' | '\n' | '\t')+ REPLACEHIDDEN
   ;

ELLIPSIS : '...';
ASSRIGHT : '>>=';
ASSLEFT  : '<<=';
ASSPLUS  : '+='	;
ASSMINUS : '-=' ;
ASSMUL   : '*=' ;
ASSDIV   : '/='	;
ASSMOD   : '%='	;
ASSAND   : '&='	;
ASSXOR   : '^=' ;
ASSOR    : '|='	;

OPRIGHT  : '>>' ;
OPLEFT   : '<<' ;
OPINC    : '++' ;
OPDEC    : '--' ;

FOLLOW   : '->' ;
SELECT   : '.'  ;

OPLOGNOT : '!'  ;
OPLOGAND : '&&' ;
OPLOGOR  : '||' ;
OPLESSEQ : '<=' ;
OPGREATEREQ : '>=' ;
OPLOGLESS : '<' ;
OPLOGGREATER : '>' ;
OPEQUALS : '==' ;
OPNOTEQ  : '!=' ;

OPPLUS   : '+' ;
OPMINUS  : '-' ;
// OPMUL    : '*' ;           Need to fix precedence
OPDIV    : '/' ;
OPMOD    : '%' ;

AMP      : '&' ;
QUESTION : '?' ;
COLON    : ':' ;
ASSIGN   : '=' ; 
OPBINOR  : '|' ;
OPBINNOT : '~' ;
OPBINXOR : '^' ;
