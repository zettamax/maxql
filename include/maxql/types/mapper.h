#pragma once

#include <maxql/types.h>
#include <stdint.h>
#include <maxql/types/types.h>

const TypeInfo* type_by_name(TypeName type_name);
TypeResolveResult resolve_type(TypeName type_name, bool has_len, uint8_t len, ResolvedType* out);
const char* resolve_result_msg(TypeResolveResult result);
TypeValidateResult resolved_type_validate(const ResolvedType*, const DataValue*);
const char* validate_type_result_msg(TypeValidateResult result);
