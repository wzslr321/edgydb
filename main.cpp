#include <iostream>
#include <fmt/core.h>
#include <algorithm>

#include "database.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

void repl(Database &db);

int Logger::trace_level = 0;

// Baza danych przechowywana jest w postaci pliku `.json`
// Plik ten będzie znajdywał się pod nazwą 'database_snapshot.json` w folderze,
// z którego uruchamiany jest program. W przypadku uruchamiania poprzez IDE Clion,
// będzie to folder 'cmake-build-debug' (zakładając uruchamianie w profilu debug)
//
// Struktura bazy składa się z dowolnej ilości grafów, przechowywanych w pliku .json
// jako "graph". Każdy graf posiada swoją nazwę przechowywaną w polu "name".
// Dodatkowo każdy graf zawiera pola "nodes" i "edges" (będące tablicami),
// gdzie pojedyńczy node odpowiada za przechowywanie danych, a edge za
// przechowywanie połączeń między nimi.
//
// Edge zawiera jedynie pola "from" oraz "to", więc jest to skierowany graf nieważony.
// Node zawiera pole id przypisywane automatycznie (poprzez inkrementację) oraz pole
// data, które może zawierać typ prymitywny - int/double/boolean/string, lub
// strukturę zdefiniowaną poprzez użytkownika, która składa się z par string:(int/double/boolean/string)
// czyli klucz - wartość.
//
// Przykładowa baza danych:
// {
//    "graphs": [
//      {
//        "name": "Policemen",
//        "nodes": [
//          {
//            "id": 1,
//            "data": "Mariusz"
//          },
//          {
//            "id": 2,
//            "data": {
//              "name": "Szeregowy",
//              "imie": "Marek"
//              "emeryt": false,
//              "wiek": 42
//            }
//          }
//        ]
//      }
//    ]
//  }
//
// Baza operuje na jednym z grafów, który na starcie programu trzeba wybrać.
// Potem w trakcie trwania programu można go także oczywiście zmienić.
//
// Wspierane zapytania:
//
// Wybiera graf, na którym będą operować kolejne komendy, o ile taki istnieje. Przykład: USE firefighters
// USE [nazwa]
//
// Tworzy graf o nazwie [nazwa]. Przykład: CREATE firefighters
// CREATE GRAPH [nazwa]
//
// Dodaje node zawierający prymitywne dane. Przykład: INSERT NODE "Mariusz"
// INSERT NODE [data]
//
// Dodaje node zawierającego dane o strukturze zdefiniowanej przez użytkownika, gdzie pole
// "name" jest polem wymaganym. Struktura musi być poprawnym JSON'em
// Przykład: INSERT NODE COMPLEX {"name":"pracownik", "wiek":40, "pensja": 1000, "imię":"Marcin"}
// INSERT NODE COMPLEX {"name":"[nazwa]", "[pole1]":[wartość1], "[pole2]":[wartość2]}
// Każde pole oprócz name, może być kolejnym typem danych o strukturze
// zdefiniowanej przez użytkownika!
// Przykład: INSERT NODE COMPLEX {"name":"pracownik", "wiek":40, "pensja": 1000, "imię":"Marcin", "przyjaciel": {"name":"pracownik", "wiek":42, "pensja": 1200, "imię":"Paweł"}}
//
// Dodaje połączenie między node'ami, bazując na przekazanym ich id. Przykład: INSERT EDGE FROM 1 to 2
// INSERT EDGE FROM [node.id] TO [node.id]
//
// Aktualizuje dane przechowywane w node o danym id na pryumitywne dane. Przykład: UPDATE NODE 1 TO "Krzysztof"
// UPDATE NODE [node.id] TO [data]
//
// Aktualizuje dane przechowywane w node o danym id na dane zdefiniowane przez użytkownika.
// Dane te muszą być poprawnym JSON'em, z obowiązkowym polem "name" o typie string. Przykład: UPDATE NODE 1 TO COMPLEX {"name":"pracownik", "szef":true}
// UPDATE NODE [node.id] TO COMPLEX [data]
//
// Wyświetla dane zawarte w node o danym id. Przykład: SELECT NODE 1
// SELECT NODE [node.id]
//
// Wyświetla wszystkie node wraz z danymi, które spełniają dany warunek.
// Przykład: SELECT NODE WHERE "pozycja" EQ "menadżer"
// SELECT NODE WHERE [nazwa_pola] EQ/NEQ [wartość]
// Powyższa komenda może zawierać dodatkowe warunki oddzielone poprzez AND lub OR
// Przykład:
// SELECT NODE WHERE "pozycja" EQ "menadżer" AND "wiek" NEQ 40 AND "profesja" EQ "informatyk"
//
// Zwraca true/false w zależności od tego czy istnieje połączenie (niebezpośrednie) między node'ami o podanych id
// Przykład: IS 2 CONNECTED TO 3
// IS [node.id] CONNECTED TO [node.id]
//
// Zwraca true/false w zależności od tego czy istnieje połączenie pośrednie między node'ami o podanych id
// Przykład: IS 2 CONNECTED DIRECTLY TO 3
// IS [node.id] CONNECTED DIRECTLY TO [node.id]
//
//
// Baza dancyh zapisuje swój stan przy zamknięciu programu oraz po wykonaniu N zapytań
// gdzie N jest wartością przekazywaną do konfiguracji bazy danych - klasa DatabaseConfig.
auto main(const int argc, char *argv[]) -> int {
    // Zczytywanie argumentu, który pomagał mi w pracy nad projektem.
    // Gdy trace_level >= 1 to załączane są logi na poziomie DEBUG
    int trace_level = 0;
    for (int i = 1; i < argc; ++i) {
        if (std::string arg = argv[i]; arg.rfind("--trace-level=", 0) == 0) {
            try {
                std::string level_str = arg.substr(14);
                trace_level = std::stoi(level_str);
                if (trace_level < 0) {
                    throw std::invalid_argument("Trace level cannot be negative.");
                }
            } catch (const std::exception &e) {
                std::cerr << "Invalid trace level value. It should be either 0 or 1. Instead it is: " << arg <<
                        std::endl;
                return 1;
            }
        }
    }
    Logger::set_trace_level(trace_level);

    const auto db_config = DatabaseConfig(100);
    auto db = Database(db_config);
    repl(db);

    return EXIT_SUCCESS;
}

void display_help();


void repl(Database &db) {
    namespace rg = std::ranges;

    fmt::println("EdgyDB v1.0.0");
    fmt::println("Type 'help' for list of options.");

    auto const exit_commands = std::vector<std::string>{"exit", "quit"};

    while (true) {
        fmt::print("> ");
        std::string command;
        std::getline(std::cin, command);
        command = Utils::trim_leading_spaces(command);
        if (rg::contains(exit_commands, command)) break;
        if (command == "help") {
            display_help();
            continue;
        }
        if (auto query = Query::from_string(command); query.has_value()) {
            db.execute_query(query.value());
        }
    }
}

void display_help() {
    fmt::println("To exit REPL type 'exit' or 'quit'");
    fmt::println("Available DDL commands:");
    fmt::println("CREATE [TYPE/EDGE/NODE]");
    fmt::println("Available DML commands:");
    fmt::println("To be documented...");
    fmt::println("Available DQL commands:");
    fmt::println("To be documented...");
}
