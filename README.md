# EdgyDB

A lightweight, in-memory graph database with JSON serialization support, written in modern C++23.

## Features

- Graph-based data structure with nodes and edges
- Support for both primitive and complex (JSON-like) data types
- Query system with conditional filtering
- JSON serialization/deserialization for persistence
- REPL interface with command-line support
- Colored logging system with debug levels

## Building

Requires:
- CMake 3.30 or higher
- C++23 compatible compiler
- fmt library (automatically fetched via CMake)

## Usage

```sql
CREATE GRAPH employees
USE employees
INSERT NODE COMPLEX {"name": "John", "position": "manager"}
INSERT EDGE FROM 1 TO 2
SELECT NODE WHERE "position" EQ "manager"
```

Run with debug logging:
```bash
./edgydb --log-level=1
```

