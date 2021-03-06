%{
#include "parser.h"
#include <cstdio>

int yylex(void);
void yyerror(const char * error);

extern char * yytext;
extern FILE * yyin;

extern int line_number;

// main program
Program * main_program;
%}

%expect 1

%token KEYWORD_AND
%token KEYWORD_ARRAY
%token KEYWORD_COLON_EQUAL
%token KEYWORD_CLASS
%token KEYWORD_COLON
%token KEYWORD_COMMA
%token KEYWORD_DO
%token KEYWORD_DOT
%token KEYWORD_DOT_DOT
%token KEYWORD_ELSE
%token KEYWORD_END
%token KEYWORD_EQUAL
%token KEYWORD_EXTENDS
%token KEYWORD_FUNCTION
%token KEYWORD_GREATER_EQUAL
%token KEYWORD_GREATER
%token KEYWORD_IF
%token KEYWORD_LEFT_BRACKET
%token KEYWORD_LESS_EQUAL
%token KEYWORD_LEFT_PARENS
%token KEYWORD_LESS
%token KEYWORD_MINUS
%token KEYWORD_MOD
%token KEYWORD_NEW
%token KEYWORD_NOT
%token KEYWORD_LESS_GREATER
%token KEYWORD_OF
%token KEYWORD_OR
%token KEYWORD_BEGIN
%token KEYWORD_PLUS
%token KEYWORD_PRINT
%token KEYWORD_PROGRAM
%token KEYWORD_RIGHT_BRACKET
%token KEYWORD_RIGHT_PARENS
%token KEYWORD_SEMICOLON
%token KEYWORD_SLASH
%token KEYWORD_STAR
%token KEYWORD_THEN
%token KEYWORD_THIS
%token KEYWORD_INTEGER
%token KEYWORD_BOOLEAN
%token KEYWORD_REAL
%token KEYWORD_CHAR
%token KEYWORD_VAR
%token KEYWORD_WHILE

%token <literal_integer> TOKEN_LITERAL_INTEGER
%token <literal_real> TOKEN_LITERAL_REAL
%token <literal_string> TOKEN_LITERAL_STRING
%token <literal_boolean> TOKEN_LITERAL_BOOLEAN
%token <identifier> TOKEN_IDENTIFIER

%type <type_denoter> type_denoter
%type <identifier_list> identifier_list
%type <function_designator> function_designator
%type <expression_list> params
%type <variable_declaration> variable_declaration
%type <variable_declaration_list> variable_declaration_list
%type <variable_access> variable_access
%type <assignment_statment> assignment_statement
%type <object_instantiation> object_instantiation
%type <print_statement> print_statement
%type <expressoin> expression
%type <comparison_operator> comparison_operator
%type <statement> statement
%type <statement_list> compound_statement
%type <statement_list> statement_list
%type <if_statement> if_statement
%type <while_statement> while_statement
%type <indexed_variable> indexed_variable
%type <attribute_designator> attribute_designator
%type <method_designator> method_designator
%type <expression_list> expression_list
%type <additive_expression> additive_expression
%type <multiplicative_expression> multiplicative_expression
%type <negatable_expression> negatable_expression
%type <_int> sign
%type <primary_expression> primary_expression
%type <array_type> array_type
%type <class_block> class_block
%type <variable_declaration_list> variable_declaration_part
%type <function_declaration_list> function_declaration_part
%type <function_declaration_list> function_declaration_list
%type <function_declaration> function_declaration
%type <function_block> function_block
%type <class_list> class_list
%type <class_declaration> class_declaration
%type <program> program
%type <additive_operator> additive_operator
%type <multiplicative_operator> multiplicative_operator

%union {
    TypeDenoter * type_denoter;
    Identifier * identifier;
    IdentifierList * identifier_list;
    FunctionDesignator * function_designator;
    VariableDeclarationList * variable_declaration_list;
    VariableDeclaration * variable_declaration;
    VariableAccess * variable_access;
    AssignmentStatement * assignment_statment;
    ObjectInstantiation * object_instantiation;
    PrintStatement * print_statement;
    Expression * expressoin;
    ComparisonOperator * comparison_operator;
    Statement * statement;
    StatementList * statement_list;
    IfStatement * if_statement;
    WhileStatement * while_statement;
    IndexedVariable * indexed_variable;
    AttributeDesignator * attribute_designator;
    MethodDesignator * method_designator;
    ExpressionList * expression_list;
    AdditiveExpression * additive_expression;
    MultiplicativeExpression * multiplicative_expression;
    NegatableExpression * negatable_expression;
    PrimaryExpression * primary_expression;
    ArrayType * array_type;
    ClassBlock * class_block;
    FunctionDeclarationList * function_declaration_list;
    FunctionDeclaration * function_declaration;
    FunctionBlock * function_block;
    ClassList * class_list;
    ClassDeclaration * class_declaration;
    Program * program;
    AdditiveOperator * additive_operator;
    MultiplicativeOperator * multiplicative_operator;
    int _int;
    LiteralInteger * literal_integer;
    LiteralReal * literal_real;
    LiteralString * literal_string;
    LiteralBoolean * literal_boolean;
}

%%

program : KEYWORD_PROGRAM TOKEN_IDENTIFIER KEYWORD_SEMICOLON class_list KEYWORD_DOT {
    main_program = new Program($2, $4);
};

class_list : class_declaration class_list {
    $$ = new ClassList($1, $2);
} | class_declaration {
    $$ = new ClassList($1, NULL);
};

class_declaration :
    KEYWORD_CLASS TOKEN_IDENTIFIER                                  KEYWORD_BEGIN class_block KEYWORD_END {
    $$ = new ClassDeclaration($2, NULL, $4);
} | KEYWORD_CLASS TOKEN_IDENTIFIER KEYWORD_EXTENDS TOKEN_IDENTIFIER KEYWORD_BEGIN class_block KEYWORD_END {
    $$ = new ClassDeclaration($2, $4, $6);
};

class_block : variable_declaration_part function_declaration_part {
    $$ = new ClassBlock($1, $2);
};

variable_declaration_part : KEYWORD_VAR variable_declaration_list KEYWORD_SEMICOLON {
    $$ = $2;
} | {
    $$ = NULL;
};

variable_declaration_list : variable_declaration_list KEYWORD_SEMICOLON variable_declaration {
    $$ = new VariableDeclarationList($3, $1);
} | variable_declaration {
    $$ = new VariableDeclarationList($1, NULL);
};

identifier_list : TOKEN_IDENTIFIER KEYWORD_COMMA identifier_list {
    $$ = new IdentifierList($1, $3);
} | TOKEN_IDENTIFIER {
    $$ = new IdentifierList($1, NULL);
};

type_denoter : array_type {
    $$ = new TypeDenoter($1);
} | TOKEN_IDENTIFIER {
    $$ = new TypeDenoter($1);
} | KEYWORD_INTEGER {
    $$ = new TypeDenoter(TypeDenoter::INTEGER);
} | KEYWORD_REAL {
    $$ = new TypeDenoter(TypeDenoter::REAL);
} | KEYWORD_CHAR {
    $$ = new TypeDenoter(TypeDenoter::CHAR);
} | KEYWORD_BOOLEAN {
    $$ = new TypeDenoter(TypeDenoter::BOOLEAN);
};

array_type : KEYWORD_ARRAY KEYWORD_LEFT_BRACKET TOKEN_LITERAL_INTEGER KEYWORD_DOT_DOT TOKEN_LITERAL_INTEGER KEYWORD_RIGHT_BRACKET KEYWORD_OF type_denoter {
    $$ = new ArrayType($3, $5, $8);
};


function_declaration_part : function_declaration_list {
    $$ = $1;
} | {
    $$ = NULL;
};

function_declaration_list : function_declaration KEYWORD_SEMICOLON function_declaration_list {
    $$ = new FunctionDeclarationList($1, $3);
} | function_declaration function_declaration_list {
    $$ = new FunctionDeclarationList($1, $2);
} | function_declaration {
    $$ = new FunctionDeclarationList($1, NULL);
};

function_declaration :
    KEYWORD_FUNCTION TOKEN_IDENTIFIER                                                                                               KEYWORD_SEMICOLON function_block {
    $$ = new FunctionDeclaration($2, NULL, NULL, $4);
} | KEYWORD_FUNCTION TOKEN_IDENTIFIER                                                                    KEYWORD_COLON type_denoter KEYWORD_SEMICOLON function_block {
    $$ = new FunctionDeclaration($2, NULL, $4, $6);
} | KEYWORD_FUNCTION TOKEN_IDENTIFIER KEYWORD_LEFT_PARENS variable_declaration_list KEYWORD_RIGHT_PARENS                            KEYWORD_SEMICOLON function_block {
    $$ = new FunctionDeclaration($2, $4, NULL, $7);
} | KEYWORD_FUNCTION TOKEN_IDENTIFIER KEYWORD_LEFT_PARENS variable_declaration_list KEYWORD_RIGHT_PARENS KEYWORD_COLON type_denoter KEYWORD_SEMICOLON function_block {
    $$ = new FunctionDeclaration($2, $4, $7, $9);
};

variable_declaration : identifier_list KEYWORD_COLON type_denoter {
    $$ = new VariableDeclaration($1, $3);
} | KEYWORD_VAR identifier_list KEYWORD_COLON type_denoter {
    $$ = new VariableDeclaration($2, $4);
};

function_block : variable_declaration_part compound_statement {
    $$ = new FunctionBlock($1, $2);
};

compound_statement : KEYWORD_BEGIN statement_list KEYWORD_END {
    $$ = $2;
};

statement_list : statement KEYWORD_SEMICOLON statement_list {
    $$ = new StatementList($1, $3);
} | statement {
    $$ = new StatementList($1, NULL);
};

statement : function_designator {
    // convert "a(...)" to "this.a(...)"
    $$ = new Statement(new MethodDesignator(new VariableAccess(VariableAccess::THIS), $1));
} | method_designator {
    $$ = new Statement($1);
} | attribute_designator {
    // convert "a.b" to "a.b()"
    $$ = new Statement(new MethodDesignator($1->owner, new FunctionDesignator($1->identifier, NULL)));
} | assignment_statement {
    $$ = new Statement($1);
} | if_statement {
    $$ = new Statement($1);
} | print_statement {
    $$ = new Statement($1);
} | while_statement {
    $$ = new Statement($1);
} | compound_statement {
    $$ = new Statement($1);
} | TOKEN_IDENTIFIER {
    // convert "a" to "this.a()"
    $$ = new Statement(new MethodDesignator(new VariableAccess(VariableAccess::THIS), new FunctionDesignator($1, NULL)));
} | {
    $$ = NULL;
};

assignment_statement : variable_access KEYWORD_COLON_EQUAL expression {
    $$ = new AssignmentStatement($1, $3);
};

if_statement : KEYWORD_IF expression KEYWORD_THEN statement KEYWORD_ELSE statement {
    $$ = new IfStatement($2, $4, $6);
} | KEYWORD_IF expression KEYWORD_THEN statement {
    $$ = new IfStatement($2, $4, NULL);
};

print_statement : KEYWORD_PRINT expression {
    $$ = new PrintStatement($2, false);
} | KEYWORD_PRINT expression KEYWORD_COMMA {
    $$ = new PrintStatement($2, true);
};

while_statement : KEYWORD_WHILE expression KEYWORD_DO statement {
    $$ = new WhileStatement($2, $4);
};

variable_access : TOKEN_IDENTIFIER {
    $$ = new VariableAccess($1);
} | indexed_variable {
    $$ = new VariableAccess($1);
} | attribute_designator {
    $$ = new VariableAccess($1);
} | KEYWORD_THIS {
    $$ = new VariableAccess(VariableAccess::THIS);
};

indexed_variable : variable_access KEYWORD_LEFT_BRACKET expression_list KEYWORD_RIGHT_BRACKET {
    $$ = new IndexedVariable($1, $3);
};

expression_list : expression KEYWORD_COMMA expression_list {
    $$ = new ExpressionList($1, $3);
} | expression {
    $$ = new ExpressionList($1, NULL);
};

expression : additive_expression {
    $$ = new Expression($1);
} | additive_expression comparison_operator additive_expression {
    $$ = new Expression($1, $2, $3);
};

comparison_operator : KEYWORD_EQUAL {
    $$ = new ComparisonOperator(ComparisonOperator::EQUAL, line_number);
} | KEYWORD_LESS_GREATER {
    $$ = new ComparisonOperator(ComparisonOperator::NOT_EQUAL, line_number);
} | KEYWORD_LESS {
    $$ = new ComparisonOperator(ComparisonOperator::LESS, line_number);
} | KEYWORD_GREATER {
    $$ = new ComparisonOperator(ComparisonOperator::GREATER, line_number);
} | KEYWORD_LESS_EQUAL {
    $$ = new ComparisonOperator(ComparisonOperator::LESS_EQUAL, line_number);
} | KEYWORD_GREATER_EQUAL {
    $$ = new ComparisonOperator(ComparisonOperator::GREATER_EQUAL, line_number);
};

additive_expression : multiplicative_expression {
    $$ = new AdditiveExpression($1);
} | additive_expression additive_operator multiplicative_expression {
    $$ = new AdditiveExpression($1, $2, $3);
};

additive_operator : KEYWORD_PLUS {
    $$ = new AdditiveOperator(AdditiveOperator::PLUS, line_number);
} | KEYWORD_MINUS {
    $$ = new AdditiveOperator(AdditiveOperator::MINUS, line_number);
} | KEYWORD_OR {
    $$ = new AdditiveOperator(AdditiveOperator::OR, line_number);
};

multiplicative_expression : negatable_expression {
    $$ = new MultiplicativeExpression($1);
} | multiplicative_expression multiplicative_operator negatable_expression {
    $$ = new MultiplicativeExpression($1, $2, $3);
};

multiplicative_operator : KEYWORD_STAR {
    $$ = new MultiplicativeOperator(MultiplicativeOperator::TIMES, line_number);
} | KEYWORD_SLASH {
    $$ = new MultiplicativeOperator(MultiplicativeOperator::DIVIDE, line_number);
} | KEYWORD_MOD {
    $$ = new MultiplicativeOperator(MultiplicativeOperator::MOD, line_number);
} | KEYWORD_AND {
    $$ = new MultiplicativeOperator(MultiplicativeOperator::AND, line_number);
};

negatable_expression : sign negatable_expression {
    $$ = new NegatableExpression($1, $2);
} | primary_expression {
    $$ = new NegatableExpression($1);
};

sign : KEYWORD_PLUS {
    $$ = 1;
} | KEYWORD_MINUS {
    $$ = -1;
};

primary_expression : TOKEN_LITERAL_INTEGER {
    $$ = new PrimaryExpression($1);
} | TOKEN_LITERAL_REAL {
    $$ = new PrimaryExpression($1);
} | TOKEN_LITERAL_STRING {
    $$ = new PrimaryExpression($1);
} | TOKEN_LITERAL_BOOLEAN {
    $$ = new PrimaryExpression($1);
} | variable_access {
    $$ = new PrimaryExpression($1);
} | function_designator {
    // convert "a()" to "this.a()"
    $$ = new PrimaryExpression(new MethodDesignator(new VariableAccess(VariableAccess::THIS), $1));
} | method_designator {
    $$ = new PrimaryExpression($1);
} | object_instantiation {
    $$ = new PrimaryExpression($1);
} | KEYWORD_LEFT_PARENS expression KEYWORD_RIGHT_PARENS {
    $$ = new PrimaryExpression($2);
} | KEYWORD_NOT primary_expression {
    $$ = new PrimaryExpression($2);
};

function_designator : TOKEN_IDENTIFIER params {
    $$ = new FunctionDesignator($1, $2);
};

params : KEYWORD_LEFT_PARENS expression_list KEYWORD_RIGHT_PARENS {
    $$ = $2;
} | KEYWORD_LEFT_PARENS KEYWORD_RIGHT_PARENS {
    $$ = NULL;
};


attribute_designator : variable_access KEYWORD_DOT TOKEN_IDENTIFIER {
    $$ = new AttributeDesignator($1, $3);
};

method_designator: variable_access KEYWORD_DOT function_designator {
    $$ = new MethodDesignator($1, $3);
};

object_instantiation: KEYWORD_NEW TOKEN_IDENTIFIER {
    $$ = new ObjectInstantiation($2);
} | KEYWORD_NEW TOKEN_IDENTIFIER params {
    $$ = new ObjectInstantiation($2, $3);
};

%%

Program * parse_input(char * filename) {
    if (filename != NULL) {
        yyin = fopen(filename, "r");
        if (yyin == NULL) {
            fprintf(stderr, "%s not found.\n", filename);
            exit(1);
        }
    }
    yyparse();
    return main_program;
}

