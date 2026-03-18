#pragma once

#include <maxql/parser/parser_state.h>
#include <maxql/parser/ast.h>
#include <maxql/core/error.h>

void parser_set_error(Parser* parser, ErrorCode code, const char* msg);
void parser_next(Parser* parser);
bool parser_check(Parser* parser, TokenType type);
bool parser_accept(Parser* parser, TokenType type);
void parser_expect(Parser* parser, TokenType type);
bool parser_accept_keyword(Parser* parser, const char* keyword);
void parser_expect_keyword(Parser* parser, const char* keyword);
Ident parse_ident(Parser* parser);
IdentArray parse_ident_list(Parser* parser);
StrView parse_number(Parser* parser);
StrView parse_string(Parser* parser);
StrView parse_hex(Parser* parser);
RawValue parse_value(Parser* parser);
IndexKeyPart parse_index_key_part(Parser* parser);
IndexKeyPartArray parse_index_key_part_list(Parser* parser);
Assignment parse_assignment(Parser* parser);
AssignmentArray parse_assignment_list(Parser* parser);
ConditionOp parse_comparison_op(Parser* parser);
Condition parse_condition(Parser* parser);
ConditionArray parse_condition_list(Parser* parser);
ColumnTypeDef parse_column_type(Parser* parser);
ColumnDef parse_column_def(Parser* parser);
ColumnDefArray parse_column_def_list(Parser* parser);
