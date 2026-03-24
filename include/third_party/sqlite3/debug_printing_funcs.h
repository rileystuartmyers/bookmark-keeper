#ifndef DEBUG_PRINTING_FUNCS_H
#define DEBUG_PRINTING_FUNCS_H

#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

#include <sqlite3.h>

const bool ENABLE_CLOSURE = true;
unsigned short print_title_flag = 0;
unsigned long id_counter = 1;

char *pErrorMessage = nullptr;
extern std::vector<std::string> ColNames;
extern std::vector<std::vector<std::string>> RowValues;

static int callback(void *NotUsed, int argc, char **argv, char **ColName) {

    std::stringstream ss;

    if (print_title_flag == 0) {
        std::stringstream ssCol;
        for (int i = 0; i < argc; ++i) {
            ssCol << ColName[i] << " ";            
        }
        std::cout << ssCol.str() << std::endl;
        print_title_flag = 1;
    }

    for (int i = 0; i < argc; ++i) {
        std::string rstStr = argv[i] ? argv[i] : "NULL";
        ss << rstStr << " ";
    }
    std::cout << ss.str() << std::endl;

    return 0;

}

static int callback_vector(void *NotUsed, int argc, char **argv, char **ColName) {
    
    std::vector<std::string> row;
    if (print_title_flag == 0) {

        for (int i = 0; i < argc; ++i) {
            ColNames.push_back((ColName[i]));
        }    

        print_title_flag = 1;

    } 
    
    for (int i = 0; i < argc; ++i) {
        std::string rowValue = argv[i] ? argv[i] : "NULL";
        row.push_back(rowValue);
    }

    RowValues.push_back(row);

    return 0;

}

void CLEAR_CALLBACK_VALUES() {

    ColNames.clear();
    RowValues.clear();
    print_title_flag = 0;

    return;

}


void PRAGMA_ENABLE_WAL(sqlite3* db) {
    sqlite3_exec(db, "PRAGMA journal_mode = WAL", nullptr, nullptr, nullptr);
    return;
}

void PRAGMA_DISABLE_WAL(sqlite3* db) {
    sqlite3_exec(db, "PRAGMA journal_mode = DELETE", nullptr, nullptr, nullptr);
    return;
}

int ERROR_CHECK(int return_code, std::stringstream& ss) {

    if (return_code != SQLITE_OK) {
        std::cerr << "Error with statement: { " << ss.str() << " }.  RETURN CODE " << return_code << ".    ERR " << pErrorMessage << std::endl;
        return -1;
    }

    return 0;

}

int ERROR_CHECK(int return_code, std::string query) {

    if (return_code != SQLITE_OK) {
        std::cerr << "Error with statement: { " << query << " }.  RETURN CODE " << return_code << ".    ERR " << pErrorMessage << std::endl;
        return -1;
    }

    return 0;

}


using statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
statement create_statement(sqlite3* db, std::string sql) {

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(
        db,
        sql.c_str(),
        sql.length(),
        &stmt,
        nullptr
    );

    std::cout << rc << std::endl;

    if (rc != SQLITE_OK) {

        std::cerr << "Unable to create statement '" << sql << std::endl;
        std::exit(EXIT_FAILURE);

    }

    return statement(stmt, sqlite3_finalize);

}

#endif