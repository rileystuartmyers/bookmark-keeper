#include <iostream>
#include <vector>
#include <string>

#include <assert.h>

#include <sqlite3.h>

class SQLiteInterface {

    private:

        const char* directory;
        sqlite3* db_connection;
        int return_code;

    public:
    
    void setup(const char* _directory) {

        directory = _directory;

        return_code = sqlite3_open(
            directory,
            &db_connection
        );

        assert(return_code == SQLITE3_OK);

    };
    
};

class ImGuiInterface {

    private:

    public:

        int setup() {};

};

class GLFWInterface {

    private:

        bool is_active = false;

    public:

        bool isactive() { return is_active; }

        int setup() {};

};




int main(int argc, char** argv) {

    SQLiteInterface SQLiteIntrfc;
    ImGuiInterface ImGuiIntrfc;
    GLFWInterface GLFWIntrfc;

    while (GLFWIntrfc.isactive()) {


    }

    return 0;
};