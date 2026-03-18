#pragma once

#include <maxql/scan/row_source.h>
#include <maxql/schema/schema.h>

RowSource* table_scan_create(TableSchema*);
