# EdgyDB

A lightweight, in-memory graph database with JSON serialization support, written in modern C++23.


---

<p align = "center">
  <b> <i> Show your support by giving a :star: </b> </i>
</p>

---

## Features

- Graph-based data structure with nodes and edges
- Support for both primitive and complex (JSON-like) data types
- Query system with conditional filtering
- JSON serialization/deserialization for persistence
- REPL interface with command-line support
- Colored logging system with debug levels

> Note: Some of the code was written fast, to meet the deadline, so it is far from ideal. The support for JSON serialization/deserialization and query parsing
> was build purely to support my specific needs, so it is not generic as it should be. Hopefully I will find some time to refactor it.

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

> EdgyDB is a C++ project developed as part of the "Programowanie w C++" (C++ Programming) course at the Polish-Japanese Academy of Information Technology, Computer Science Major, during the 2024/2025 academic year.
