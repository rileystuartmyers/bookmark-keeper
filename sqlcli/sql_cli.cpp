#include <iostream>
#include <cstdlib>

#include <sqlite3.h>

const char* SQL_TABLE_NAMES = "SELECT name FROM sqlite_master WHERE type='table'";

void ClearScreen() {
    
    #if defined(_WIN32) 
        system("cls");
    #else 
        system("clear");
    #endif

}

void PrintRows(sqlite3_stmt* stmt) {

    int nCol = sqlite3_column_count(stmt);
    int rowCount = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        
        printf("Row %d:\n", rowCount++);
    
        for (int i = 0; i < nCol; ++i) {

            printf("    %s = %s\n",
                sqlite3_column_name(stmt, i),
                sqlite3_column_text(stmt, i)
            );

        }

    }
    
    std::cout << std::endl;
    return;

}

void PrintTableNames(sqlite3_stmt* stmt) {

    std::cout << "| ACTIVE TABLES: ";
    int nCols = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        
        for (int i = 0; i < nCols; ++i) {

            printf("%s ",
                sqlite3_column_text(stmt, i)
            );

        }

    }

    std::cout << "|" << std::endl;
    return;

}

int main(int argc, char** argv) {

    if (argc < 2) {

        std::cout << "--- Usage: {your_database}.db ---" << std::endl;
        std::cout << "--- After providing a database file, you will be prompted for SQL queries. ---" << std::endl;
        std::cout << "--- When finished, type 'exit' --- " << std::endl;
        return 0;

    }

    ClearScreen();

    sqlite3* db;
    sqlite3_stmt* table_name_stmt;
    int rc;
    
    rc = sqlite3_open(
        argv[1],
        &db
    );
    
    if (rc != SQLITE_OK) {
        std::cerr << "Error initializing/opening table." << std::endl; 
        return 1;
    } else {
        std::cout << "***Successfully connected to database file.  ";

        rc = sqlite3_prepare(
            db, 
            SQL_TABLE_NAMES,
            -1, 
            &table_name_stmt, 
            nullptr
        );

        PrintTableNames(table_name_stmt);

    }
    
    while (true) {
        
        sqlite3_stmt* stmt;
        std::string sql;
        std::cout << "Enter a query..." << std::endl;
        std::getline(std::cin, sql);

        if (sql == "exit" || sql == "EXIT" || sql == "Exit" || sql == "quit" || sql == "QUIT") { 
            break; 
        } else if (sql == "clear" || sql == "CLEAR" || sql == "Clear") {
            ClearScreen();
            PrintTableNames(table_name_stmt);
            continue;
        }
        
        rc = sqlite3_prepare(
            db, 
            sql.c_str(),
            sql.size(), 
            &stmt, 
            nullptr
        );

        if (rc != SQLITE_OK) {
            std::cerr << "Error issuing statement." << std::endl << std::endl; 
            continue;
        } else {
            std::cout << "Successfully issued query." << std::endl << std::endl;
        }
        
        PrintRows(stmt);
        sqlite3_finalize(stmt);

    }

    sqlite3_close(db);

    return 0;

}
