#pragma once

#include <maxql/scan/row_source.h>
#include <maxql/scan/scan_types.h>
#include <maxql/schema/schema.h>

RowSource* index_scan_create(TableSchema*, IndexDecision*);
