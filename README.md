# maxQL

A SQL database engine built from scratch in C23.

maxQL implements the full query processing pipeline — lexing, parsing, validation, analysis, planning, and execution — along with B-tree indexing, binary row storage, and an interactive REPL.

## Features

- **DDL**: `create table`, `drop table`, `create index`, `drop index`, `truncate`
- **DML**: `select`, `insert`, `update`, `delete` with `where` clauses
- **Introspection**: `show tables`, `show table`, `show index`
- **Types**: `int`, `varchar(n)`, `char(n)`, `binary(n)`, `float`
- **B-tree indexes** with pluggable index type registry
- **Full table scan** and **index-guided scan** via a unified `RowSource` interface
- **Interactive REPL** with line editing, command history, and reverse search

## Build

Requires CMake 3.21+ and a C23-capable compiler.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target maxql
./build/maxql
```

## Example

```sql
create table users (id int, name varchar(64), score float);
create index idx_id on users (id);
insert into users set id = 1, name = 'alice', score = 95.5;
insert into users set id = 2, name = 'bob', score = 87.0;
select * from users where id = 1;
```

## Architecture

```
SQL input
  --> Lexer (tokenization)
    --> Parser (AST)
      --> Validator (limits & constraint checks)
        --> Analyzer (schema resolution, type checking)
          --> Planner (execution plan)
            --> Executor (row I/O + index ops)
```

Storage uses binary `.data` files for rows and `.idx` files for B-tree indexes. Schema metadata is persisted in `.schema` files and the table registry in a `.catalog` file.

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
