#include "semantic_checker.h"
#include "utils.h"
using Utils::err_header;

#include <map>
#include <iostream>
#include <sstream>
#include <cassert>

bool SemanticChecker::check(Program * program, SymbolTable * symbol_table)
{
    return SemanticChecker(program, symbol_table).internal_check();
}

SemanticChecker::SemanticChecker(Program * program, SymbolTable * symbol_table) :
    m_program(program),
    m_symbol_table(symbol_table),
    m_success(true),
    m_recursive_error(false){}

bool SemanticChecker::internal_check()
{
    // check the main class and constructor
    if (m_symbol_table->has_key(m_program->identifier->text)) {
        ClassSymbolTable * class_symbols = m_symbol_table->get(m_program->identifier->text);
        if (class_symbols->function_symbols->has_key(m_program->identifier->text)) {
            // make sure it has no parameters
            FunctionSymbolTable * function_symbols = class_symbols->function_symbols->get(m_program->identifier->text);
            if (function_symbols->function_declaration->parameter_list != NULL) {
                std::cerr << err_header(function_symbols->function_declaration->identifier->line_number) <<
                    "constructor for main class \"" << class_symbols->class_declaration->identifier->text <<
                    "\" must have no parameters" << std::endl;
                m_success = false;
            }
        } else {
            std::cerr << err_header(class_symbols->class_declaration->identifier->line_number) <<
                "main class \"" << class_symbols->class_declaration->identifier->text <<
                "\" must have a parameterless constructor" << std::endl;
            m_success = false;
        }
    } else {
        std::cerr << err_header(m_program->identifier->line_number) << "missing program class" << std::endl;
        m_success = false;
    }

    // check classes
    for (ClassList * class_list = m_program->class_list; class_list != NULL; class_list = class_list->next) {
        ClassDeclaration * class_declaration = class_list->item;
        m_class_id = class_declaration->identifier->text;

        check_variable_declaration_list(class_declaration->class_block->variable_list, true);

        // check functions
        for (FunctionDeclarationList * function_list = class_declaration->class_block->function_list; function_list != NULL; function_list = function_list->next) {
            FunctionDeclaration * function_declaration = function_list->item;
            m_function_id = function_declaration->identifier->text;

            check_variable_declaration_list(function_declaration->parameter_list, false);
            check_variable_declaration_list(function_declaration->block->variable_list, true);
            if (function_declaration->type != NULL)
                check_type(function_declaration->type, false);

            StatementList * statement_list = function_declaration->block->statement_list;
            check_statement_list(statement_list);
        }
    }

    return m_success;
}

void SemanticChecker::check_variable_declaration_list(VariableDeclarationList * _variable_list, bool allow_arrays)
{
    for (VariableDeclarationList * variable_list = _variable_list; variable_list != NULL; variable_list = variable_list->next)
        check_variable_declaration(variable_list->item, allow_arrays);
}

void SemanticChecker::check_variable_declaration(VariableDeclaration * variable, bool allow_arrays)
{
    check_type(variable->type, allow_arrays);
}

void SemanticChecker::check_type(TypeDenoter * type, bool allow_arrays)
{
    switch(type->type) {
        case TypeDenoter::INTEGER:
            break;
        case TypeDenoter::REAL:
            break;
        case TypeDenoter::CHAR:
            break;
        case TypeDenoter::BOOLEAN:
            break;
        case TypeDenoter::CLASS:
            // make sure the class is declared
            if (! m_symbol_table->has_key(type->class_identifier->text)) {
                std::cerr << err_header(type->class_identifier->line_number) <<
                    "class \"" << type->class_identifier->text << "\" is not defined" << std::endl;
                m_success = false;
            }
            break;
        case TypeDenoter::ARRAY:
            if (!allow_arrays) {
                std::cerr << err_header(type->array_type->min->line_number) << "parameters and return values are not allowed to be arrays" << std::endl;
                m_success = false;
                break;
            }
            // make sure the range is valid
            if (! (type->array_type->max->value >= type->array_type->min->value)) {
                std::cerr << err_header(type->array_type->min->line_number) << "invalid array range: [" <<
                    type->array_type->min->value << ".." << type->array_type->max->value << "]" << std::endl;
                m_success = false;
            }
            break;
        default:
            assert(false);
    }
}

void SemanticChecker::check_statement_list(StatementList * _statement_list)
{
    for (StatementList * statement_list = _statement_list; statement_list != NULL; statement_list = statement_list->next)
        check_statement(statement_list->item);
}

bool SemanticChecker::types_equal(TypeDenoter * type1, TypeDenoter * type2)
{
    if (type1->type == type2->type) {
        if (type1->type == TypeDenoter::ARRAY) {
            // make sure arrays are same size and of same type
            bool size_equal = (type1->array_type->max - type1->array_type->min) ==
                (type2->array_type->max - type2->array_type->min);
            return size_equal && types_equal(type1->array_type->type, type2->array_type->type);
        } else if (type1->type == TypeDenoter::CLASS) {
            return type1->class_identifier->text.compare(type2->class_identifier->text) == 0;
        } else {
            return true;
        }
    } else {
        return false;
    }
}

bool SemanticChecker::is_ancestor(TypeDenoter * child, TypeDenoter * ancestor)
{
    assert(child->type == TypeDenoter::CLASS);
    assert(ancestor->type == TypeDenoter::CLASS);
    if (child->class_identifier->text.compare(ancestor->class_identifier->text) == 0) {
        return true;
    } else {
        ClassDeclaration * child_declaration = m_symbol_table->get(child->class_identifier->text)->class_declaration;
        if (child_declaration->parent_identifier == NULL)
            return false;
        return is_ancestor(new TypeDenoter(child_declaration->parent_identifier), ancestor);
    }
}

bool SemanticChecker::structurally_equivalent(TypeDenoter * left_type, TypeDenoter * right_type)
{
    assert(left_type->type == TypeDenoter::CLASS);
    assert(right_type->type == TypeDenoter::CLASS);

    if (m_recursive_error)
        return true;

    VariableTable * left_fields = m_symbol_table->get(left_type->class_identifier->text)->variables;
    VariableTable * right_fields = m_symbol_table->get(right_type->class_identifier->text)->variables;
    for (int i=0; i < left_fields->count() || i < right_fields->count(); ++i) {
        // if we get past the end of one of them, they have differing numbers of fields.
        if (i >= left_fields->count())
            return false;
        if (i >= right_fields->count())
            return false;
        // each field has to be assignment compatible
        if (!assignment_valid(left_fields->get(i)->type, right_fields->get(i)->type))
            return false;

    }

    return true;
}

bool SemanticChecker::assignment_valid(TypeDenoter * left_type, TypeDenoter * right_type)
{
    // rules for assignment
    // X = X - OK, but if it's an array, has to be the same size
    // integer = char - OK
    // real = integer/char - OK
    // A = B - OK if A is an ancestor of B or if A and B's fields are respectively compatible
    if (left_type->type == right_type->type) {
        if (left_type->type == TypeDenoter::ARRAY) {
            bool size_equal = (left_type->array_type->max->value - left_type->array_type->min->value) ==
                (right_type->array_type->max->value - right_type->array_type->min->value);
            return size_equal && assignment_valid(left_type->array_type->type, right_type->array_type->type);
        } else if (left_type->type == TypeDenoter::CLASS) {
            return is_ancestor(left_type, right_type) ||
                structurally_equivalent(left_type, right_type);
        } else {
            return true;
        }
    } else if (left_type->type == TypeDenoter::INTEGER && right_type->type == TypeDenoter::CHAR) {
        return true;
    } else if (left_type->type == TypeDenoter::REAL && (right_type->type == TypeDenoter::INTEGER || right_type->type == TypeDenoter::CHAR)) {
        return true;
    } else {
        return false;
    }
}

std::string SemanticChecker::type_to_string(TypeDenoter * type)
{
    std::stringstream ss;
    switch(type->type) {
        case TypeDenoter::INTEGER:
            ss << "integer";
            break;
        case TypeDenoter::REAL:
            ss << "real";
            break;
        case TypeDenoter::CHAR:
            ss << "char";
            break;
        case TypeDenoter::BOOLEAN:
            ss << "boolean";
            break;
        case TypeDenoter::CLASS:
            ss << type->class_identifier->text;
            break;
        case TypeDenoter::ARRAY:
            ss << "array[" << type->array_type->min->value << ".." << type->array_type->max->value << "] of " <<
                type_to_string(type->array_type->type);
            break;
        default:
            assert(false);
    }
    return ss.str();
}

void SemanticChecker::check_statement(Statement * statement)
{
    if (statement == NULL)
        return;
    switch(statement->type) {
        case Statement::ASSIGNMENT:
        {
            TypeDenoter * left_type = check_variable_access(statement->assignment->variable, true);
            TypeDenoter * right_type = check_expression(statement->assignment->expression);
            if (left_type == NULL || right_type == NULL)
                break; // problem elsewhere
            if (! assignment_valid(left_type, right_type)) {
                Identifier * identifier = find_identifier(statement->assignment->variable);
                if (left_type->type == TypeDenoter::CLASS && right_type->type == TypeDenoter::CLASS) {
                    std::cerr << err_header(identifier->line_number) <<
                        "class \"" << type_to_string(right_type) << "\" is not a base class of \"" <<
                        type_to_string(left_type) << "\" in the assignment" << std::endl;
                } else {
                    std::cerr << err_header(identifier->line_number) <<
                        "cannot assign \"" << type_to_string(right_type) << "\" to \"" <<
                        type_to_string(left_type) << "\"" << std::endl;
                }
                m_success = false;
            }
            break;
        }
        case Statement::IF:
            check_expression(statement->if_statement->expression);
            check_statement(statement->if_statement->then_statement);
            if (statement->if_statement->else_statement != NULL)
                check_statement(statement->if_statement->else_statement);
            break;
        case Statement::PRINT:
            check_expression(statement->print_statement->expression);
            break;
        case Statement::WHILE:
            check_expression(statement->while_statement->expression);
            check_statement(statement->while_statement->statement);
            break;
        case Statement::COMPOUND:
            check_statement_list(statement->compound_statement);
            break;
        case Statement::METHOD:
            check_method_designator(statement->method);
            break;
        default:
            assert(false);
    }
}

TypeDenoter * SemanticChecker::check_expression(Expression * expression)
{
    TypeDenoter * type;
    if (expression->right == NULL) {
        // it's just the type of the first additive expression
        type = check_additive_expression(expression->left);
    } else {
        TypeDenoter * left_type = check_additive_expression(expression->left);
        TypeDenoter * right_type = check_additive_expression(expression->right);
        if (! assignment_valid(left_type, right_type) && !assignment_valid(right_type, left_type)) {
            std::cerr << err_header(expression->_operator->line_number) <<
                    type_to_string(left_type) << " and " << type_to_string(right_type) << " are not comparable." << std::endl;
            m_success = false;
            return NULL;
        }
        // we're looking at a compare operator, so it always returns a boolean
        type = new TypeDenoter(TypeDenoter::BOOLEAN);
    }
    expression->type = type; // cache the type
    return type;
}

// when we do a multiplicitive or additive operation, what is the return type?
TypeDenoter * SemanticChecker::combined_type(TypeDenoter * left_type, TypeDenoter * right_type)
{
    // valid addition types:
    // char +       char =      char
    // integer +    integer =   integer
    // integer +    char =      integer
    // real +       integer =   real
    // real +       real =      real
    // real +       char =      real
    // bool +       bool =      bool
    if (left_type->type == TypeDenoter::CHAR && right_type->type == TypeDenoter::CHAR) {
        return new TypeDenoter(TypeDenoter::CHAR);
    } else if (left_type->type == TypeDenoter::INTEGER && right_type->type == TypeDenoter::INTEGER) {
        return new TypeDenoter(TypeDenoter::INTEGER);
    } else if (left_type->type == TypeDenoter::REAL && right_type->type == TypeDenoter::REAL) {
        return new TypeDenoter(TypeDenoter::REAL);
    } else if ((left_type->type == TypeDenoter::INTEGER && right_type->type == TypeDenoter::CHAR) ||
        (left_type->type == TypeDenoter::CHAR && right_type->type == TypeDenoter::INTEGER))
    {
        return new TypeDenoter(TypeDenoter::INTEGER);
    } else if ((left_type->type == TypeDenoter::REAL && right_type->type == TypeDenoter::INTEGER) ||
        (left_type->type == TypeDenoter::INTEGER && right_type->type == TypeDenoter::REAL))
    {
        return new TypeDenoter(TypeDenoter::REAL);
    } else if ((left_type->type == TypeDenoter::REAL && right_type->type == TypeDenoter::CHAR) ||
        (left_type->type == TypeDenoter::CHAR && right_type->type == TypeDenoter::REAL))
    {
        return new TypeDenoter(TypeDenoter::REAL);
    } else if (left_type->type == TypeDenoter::BOOLEAN && right_type->type == TypeDenoter::BOOLEAN) {
        return new TypeDenoter(TypeDenoter::BOOLEAN);
    } else {
        // anything else is invalid
        return NULL;
    }
}

TypeDenoter * SemanticChecker::check_additive_expression(AdditiveExpression * additive_expression)
{
    TypeDenoter * type;
    TypeDenoter * right_type = check_multiplicitive_expression(additive_expression->right);
    if (additive_expression->left == NULL) {
        // it's just the type of the right
        type = right_type;
    } else {
        TypeDenoter * left_type = check_additive_expression(additive_expression->left);
        if (left_type == NULL || right_type == NULL) {
            // semantic error occurred down the stack
            return NULL;
        }
        type = combined_type(left_type, right_type);
    }
    additive_expression->type = type;
    return type;
}

TypeDenoter * SemanticChecker::check_multiplicitive_expression(MultiplicativeExpression * multiplicative_expression) {
    TypeDenoter * type;
    TypeDenoter * right_type = check_negatable_expression(multiplicative_expression->right);
    if (multiplicative_expression->left == NULL) {
        // it's just the type of the right
        type = right_type;
    } else {
        TypeDenoter * left_type = check_multiplicitive_expression(multiplicative_expression->left);
        if (left_type == NULL || right_type == NULL) {
            // semantic error occurred down the stack
            return NULL;
        }
        type = combined_type(left_type, right_type);
    }
    multiplicative_expression->type = type;
    return type;
}

TypeDenoter * SemanticChecker::check_negatable_expression(NegatableExpression * negatable_expression) {
    TypeDenoter * type;
    if (negatable_expression->type == NegatableExpression::SIGN) {
        type = check_negatable_expression(negatable_expression->next);
    } else if (negatable_expression->type == NegatableExpression::PRIMARY) {
        type = check_primary_expression(negatable_expression->primary_expression);
    } else {
        assert(false);
        return NULL;
    }
    negatable_expression->variable_type = type;
    return type;
}

TypeDenoter * SemanticChecker::check_primary_expression(PrimaryExpression * primary_expression) {
    TypeDenoter * type;
    switch (primary_expression->type) {
        case PrimaryExpression::VARIABLE:
            type = check_variable_access(primary_expression->variable);
            break;
        case PrimaryExpression::INTEGER:
            type = new TypeDenoter(TypeDenoter::INTEGER);
            break;
        case PrimaryExpression::REAL:
            type = new TypeDenoter(TypeDenoter::REAL);
            break;
        case PrimaryExpression::BOOLEAN:
            type = new TypeDenoter(TypeDenoter::BOOLEAN);
            break;
        case PrimaryExpression::STRING:
        {
            std::string str = primary_expression->literal_string->value;
            int str_len = (int) str.length();
            if (str_len == 1) {
                type = new TypeDenoter(TypeDenoter::CHAR);
            } else {
                type = new TypeDenoter(new ArrayType(new LiteralInteger(0, 0), new LiteralInteger(str_len-1, 0), new TypeDenoter(TypeDenoter::CHAR)));
            }
            break;
        }
        case PrimaryExpression::METHOD:
            type = check_method_designator(primary_expression->method);
            break;
        case PrimaryExpression::OBJECT_INSTANTIATION:
            type = check_object_instantiation(primary_expression->object_instantiation);
            break;
        case PrimaryExpression::PARENS:
            type = check_expression(primary_expression->parens_expression);
            break;
        case PrimaryExpression::NOT:
            type = check_primary_expression(primary_expression->not_expression);
            break;
        default:
            assert(false);
            return NULL;
    }
    primary_expression->variable_type = type;
    return type;
}

TypeDenoter * SemanticChecker::check_variable_access(VariableAccess * variable_access, bool allow_function_return_value)
{
    switch (variable_access->type) {
        case VariableAccess::IDENTIFIER:
        {
            // it's the type of the declaration
            // figure out what variable this is referencing
            ClassSymbolTable * class_symbols = m_symbol_table->get(m_class_id);
            FunctionSymbolTable * function_symbols = class_symbols->function_symbols->get(m_function_id);
            if (function_symbols->variables->has_key(variable_access->identifier->text)) {
                // local variable or parameter
                if (! allow_function_return_value) {
                    // if it's the function return value, we need explicit permission
                    if (Utils::insensitive_equals(function_symbols->function_declaration->identifier->text, variable_access->identifier->text)) {
                        std::cerr << err_header(variable_access->identifier->line_number) <<
                            "cannot read from \"" << variable_access->identifier->text <<
                            "\" because it is reserved for use as the function return value" << std::endl;
                        m_success = false;
                    }
                }
                return function_symbols->variables->get(variable_access->identifier->text)->type;
            }
            TypeDenoter * type = class_variable_type(m_class_id, variable_access->identifier);
            if (type != NULL) {
                // class variable
                // convert "a" to "this.a"
                variable_access->type = VariableAccess::ATTRIBUTE;
                variable_access->attribute = new AttributeDesignator(new VariableAccess(VariableAccess::THIS), variable_access->identifier);
                return type;
            }
            // undeclared variable
            std::cerr << err_header(variable_access->identifier->line_number) <<
                "variable \"" << variable_access->identifier->text << "\" not declared" << std::endl;
            m_success = false;
            return NULL;
        }
        case VariableAccess::INDEXED_VARIABLE:
            return check_indexed_variable(variable_access->indexed_variable);
        case VariableAccess::ATTRIBUTE:
            return check_attribute_designator(variable_access->attribute);
        case VariableAccess::THIS:
            return new TypeDenoter(m_symbol_table->get(m_class_id)->class_declaration->identifier);
        default:
            assert(false);
            return NULL;
    }
}

TypeDenoter * SemanticChecker::class_variable_type(std::string class_name, Identifier * variable)
{
    ClassSymbolTable * class_symbols = m_symbol_table->get(class_name);
    if (class_symbols->variables->has_key(variable->text)) {
        return class_symbols->variables->get(variable->text)->type;
    } else if (class_symbols->class_declaration->parent_identifier == NULL) {
        return NULL;
    } else {
        return class_variable_type(class_symbols->class_declaration->parent_identifier->text, variable);
    }
}

FunctionDeclaration * SemanticChecker::class_method(std::string class_name, FunctionDesignator * function_designator)
{
    ClassSymbolTable * class_symbols = m_symbol_table->get(class_name);
    if (class_symbols->function_symbols->has_key(function_designator->identifier->text)) {
        return class_symbols->function_symbols->get(function_designator->identifier->text)->function_declaration;
    } else if (class_symbols->class_declaration->parent_identifier == NULL) {
        return NULL;
    } else {
        return class_method(class_symbols->class_declaration->parent_identifier->text, function_designator);
    }
}

TypeDenoter * SemanticChecker::check_method_designator(MethodDesignator * method_designator)
{
    FunctionDesignator * function_designator = method_designator->function;

    TypeDenoter * owner_type = check_variable_access(method_designator->owner);
    assert(owner_type->type == TypeDenoter::CLASS);
    FunctionDeclaration * function_declaration = class_method(owner_type->class_identifier->text, function_designator);
    if (function_declaration == NULL) {
        std::cerr << err_header(function_designator->identifier->line_number) <<
            "class \"" << owner_type->class_identifier->text << "\" has no method \"" << function_designator->identifier->text << "\"" << std::endl;
        m_success = false;
        return NULL;
    }

    // check signature
    int parameter_index = 0;
    ExpressionList * actual_parameter_list = function_designator->parameter_list;
    VariableDeclarationList * formal_parameter_list = function_declaration->parameter_list;
    for (;actual_parameter_list != NULL || formal_parameter_list != NULL;
        actual_parameter_list = actual_parameter_list->next,
        formal_parameter_list = formal_parameter_list->next,
        ++parameter_index)
    {
        if (actual_parameter_list == NULL) {
            std::cerr << err_header(function_designator->identifier->line_number) <<
                "too few arguments to function \"" << function_designator->identifier->text << "\"" << std::endl;
            m_success = false;
            break;
        } else if (formal_parameter_list == NULL) {
            std::cerr << err_header(function_designator->identifier->line_number) <<
                "too many arguments to function \"" << function_designator->identifier->text << "\"" << std::endl;
            m_success = false;
            break;
        } else {
            TypeDenoter * formal_type = formal_parameter_list->item->type;
            TypeDenoter * actual_type = check_expression(actual_parameter_list->item);
            if (formal_type == NULL || actual_type == NULL)
                continue;
            if (! assignment_valid(formal_type, actual_type)) {
                std::cerr << err_header(function_designator->identifier->line_number) <<
                    "function \"" << function_designator->identifier->text << "\": " <<
                    "parameter index " << parameter_index << ": cannot convert \"" <<
                    type_to_string(actual_type) << "\" to \"" << type_to_string(formal_type) << "\"" << std::endl;
                m_success = false;
            }
        }
    }

    return function_declaration->type;
}

TypeDenoter * SemanticChecker::check_object_instantiation(ObjectInstantiation * object_instantiation)
{
    // look it up in the symbol table
    if (! m_symbol_table->has_key(object_instantiation->class_identifier->text)) {
        std::cerr << err_header(object_instantiation->class_identifier->line_number) <<
            "class \"" << object_instantiation->class_identifier->text << "\" not declared" << std::endl;
        m_success = false;
        return NULL;
    }

    for (ExpressionList * expression_list = object_instantiation->parameter_list; expression_list != NULL; expression_list = expression_list->next) {
        Expression * expression = expression_list->item;
        check_expression(expression);
    }

    return new TypeDenoter(object_instantiation->class_identifier);
}

LiteralInteger * SemanticChecker::constant_integer(Expression * expression)
{
    if (expression->right != NULL)
        return NULL;
    if (expression->left->left != NULL)
        return NULL;
    if (expression->left->right->left != NULL)
        return NULL;

    NegatableExpression * negatable_expression = expression->left->right->right;
    int sign = 1;
    while (negatable_expression->type == NegatableExpression::SIGN) {
        sign *= negatable_expression->sign;
        negatable_expression = negatable_expression->next;
    }
    if (negatable_expression->primary_expression->type != PrimaryExpression::INTEGER)
        return NULL;

    return negatable_expression->primary_expression->literal_integer;
}

Identifier * SemanticChecker::find_identifier(VariableAccess * variable_access)
{
    switch (variable_access->type) {
        case VariableAccess::IDENTIFIER:
            return variable_access->identifier;
        case VariableAccess::INDEXED_VARIABLE:
            return find_identifier(variable_access->indexed_variable->variable);
        case VariableAccess::ATTRIBUTE:
            return variable_access->attribute->identifier;
        default:
            assert(false);
    }
}

TypeDenoter * SemanticChecker::check_indexed_variable(IndexedVariable * indexed_variable)
{
    TypeDenoter * array_type = check_variable_access(indexed_variable->variable);
    if (array_type->type != TypeDenoter::ARRAY) {
        Identifier * id = find_identifier(indexed_variable->variable);
        std::cerr << err_header(id->line_number) << "indexed variable \"" << id->text << "\" is not an array" << std::endl;
        m_success = false;
        return NULL;
    }

    // the type that we keep iterating to get inner arrays
    TypeDenoter * array_type_iterator = array_type;

    // every expression in the list should be an integer
    for (ExpressionList * expression_list = indexed_variable->expression_list; expression_list != NULL; expression_list = expression_list->next) {
        Expression * expression = expression_list->item;
        TypeDenoter * index_type = check_expression(expression);
        if (index_type == NULL) {
            // semantic error occured while determining type
            continue;
        }
        if (index_type->type != TypeDenoter::INTEGER) {
            Identifier * identifier = find_identifier(indexed_variable->variable);
            std::cerr << err_header(identifier->line_number) <<
                "array index not an integer for variable \"" << identifier->text << "\"" << std::endl;
            m_success = false;
        } else {
            // if expression is constant, check bounds
            LiteralInteger * literal_int = constant_integer(expression);
            if (literal_int != NULL) {
                assert(array_type_iterator->type == TypeDenoter::ARRAY);
                if (! (literal_int->value >= array_type_iterator->array_type->min->value &&
                    literal_int->value <= array_type_iterator->array_type->max->value))
                {
                    std::cerr << err_header(literal_int->line_number) << "array index " << literal_int->value <<
                        " is out of the range [" << array_type_iterator->array_type->min->value << ".." <<
                        array_type_iterator->array_type->max->value << "]" << std::endl;
                    m_success = false;
                }
            }
        }
        array_type_iterator = array_type_iterator->array_type->type;
    }

    // it's the array type of the variable access type
    return array_type_iterator;
}

TypeDenoter * SemanticChecker::check_attribute_designator(AttributeDesignator * attribute_designator)
{
    TypeDenoter * owner_type = check_variable_access(attribute_designator->owner);
    assert(owner_type->type == TypeDenoter::CLASS);
    TypeDenoter * variable_type = class_variable_type(owner_type->class_identifier->text, attribute_designator->identifier);
    if (variable_type == NULL) {
        std::cerr << err_header(attribute_designator->identifier->line_number) <<
            "class \"" << owner_type->class_identifier->text << "\" has no attribute \"" <<
            attribute_designator->identifier->text << "\"" << std::endl;
        m_success = false;
        return NULL;
    } else {
        return variable_type;
    }
}
