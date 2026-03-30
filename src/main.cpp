#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>

#include <assert.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>
#include <sqlite3.h>

bool IndexOutOfRange(int index, int size) {

    if ( (index < 0) || (index >= size) ) {
        return true;
    } else {
        return false;
    }

}

using statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
statement create_statement(sqlite3* db, std::string sqlString) {

    sqlite3_stmt* stmt;
    int return_code = sqlite3_prepare_v2(
        db,
        sqlString.c_str(),
        -1,
        &stmt,
        nullptr
    );

    assert((return_code == SQLITE_OK) && "Error creating SQL statement.");    
    return statement(stmt, sqlite3_finalize);

}

class SavedURLBookmark {

    private:

        int id;
        std::string label;
        std::string url;
        std::string date;
        std::string row_display_content;

    public:

        SavedURLBookmark(int _id, std::string _label, std::string _url, std::string _date) {

            std::string intervalue_buffer(20, ' ');

            id = _id;
            label = _label;
            url = _url;
            date = _date;

            row_display_content = label + intervalue_buffer + url + intervalue_buffer + date;
            
            std::cout << label << " " << url << " " << date << std::endl;
            std::cout << row_display_content << std::endl;
        }

        int getid() {
            return id;
        }

        std::string getlabel() {
            return label;
        }

        void setlabel(std::string _label) {
            label = _label;
        }

        std::string geturl() {
            return url;
        }

        std::string getdate() {
            return date;
        }

        std::string getrow_display_content() {
            return row_display_content;
        }

        const char* formatted_display_string() {
            return row_display_content.c_str();
        }

        void clear() {

            id = 0;
            label = "";
            url = "";
            date = "";
            row_display_content = "";

        }

};

struct precompiled_sqliteStatements {
    
    sqlite3* db;
    statement fetch_stmt;
    statement insert_stmt;
    statement delete_stmt;
    statement download_stmt;
    statement vacuum_stmt;

    precompiled_sqliteStatements(sqlite3* db_ptr) :
    
        db(db_ptr),
        
        fetch_stmt(
            create_statement(
                db,
                "SELECT id, label, url, date FROM BOOKMARKS;"
            )
        ),

        insert_stmt(
            create_statement(
                db,
                "INSERT INTO BOOKMARKS(label, url, date) VALUES (?, ?, ?);"
            )
        ),
        
        delete_stmt(
            create_statement(
                db,
                "DELETE FROM BOOKMARKS WHERE id = ?;"
            )
        ),
        
        download_stmt(
            create_statement(
                db,
                "SELECT url FROM BOOKMARKS"
            )
        ),

        vacuum_stmt(
            create_statement(
                db,
                "VACUUM;"
            )
        )
        
    {}

    ~precompiled_sqliteStatements() = default;
    
};

class SQLiteInterface {

    private:

        const char* directory;
        sqlite3* db_connection;
        std::unique_ptr<precompiled_sqliteStatements> statements;
        int return_code;

        void initialize_connection() {
            
            return_code = sqlite3_open(
                directory,
                &db_connection
            );
            
            assert(return_code == SQLITE_OK);
            
        };
        
        void setup() {

            return_code = sqlite3_initialize();
            
            if (return_code != SQLITE_OK) {
                throw std::runtime_error("sqlite3 Lib Initialization Error. Exiting...");
            }

        }

    public:
    
        bool is_valid_connection() {
            return (db_connection != nullptr);
        }

        void refresh_statement(statement& stmt) {
            sqlite3_reset(stmt.get());
        }

        void refresh_and_clear_bindings_of_statement(statement& stmt) {
            refresh_statement(stmt);
            sqlite3_clear_bindings(stmt.get());
        }

        void insert_bookmark(std::string label, std::string url, std::string date) {

            // "INSERT INTO BOOKMARKS(label, url, date) VALUES (?, ?, ?);"

            statement& insert_statement = statements -> insert_stmt;
            sqlite3_stmt* query = insert_statement.get();
            sqlite3_bind_text(query, 1, label.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(query, 2, url.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(query, 3, date.c_str(), -1, SQLITE_TRANSIENT);

            return_code = sqlite3_step(query);
            if (return_code != SQLITE_DONE) {
                std::cerr << "Insertion failed :::> " << sqlite3_errmsg(db_connection) << std::endl;
            } else {
                std::cout << "Successful insertion..." << std::endl;
            }

            refresh_and_clear_bindings_of_statement(insert_statement);

        }

        void insert_bookmark(SavedURLBookmark Bookmark) {

            statement& insert_statement = statements -> insert_stmt;
            sqlite3_stmt* query = insert_statement.get();
            sqlite3_bind_text(query, 1, Bookmark.getlabel().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(query, 2, Bookmark.geturl().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(query, 3, Bookmark.getdate().c_str(), -1, SQLITE_TRANSIENT);

            return_code = sqlite3_step(query);
            if (return_code != SQLITE_DONE) {
                std::cerr << "Insertion failed :::> " << sqlite3_errmsg(db_connection) << std::endl;
            } else {
                std::cout << "Successful insertion..." << std::endl;
            }

            refresh_and_clear_bindings_of_statement(insert_statement);

        }

        statement& get_fetch_statement() {
            refresh_statement(statements -> fetch_stmt);
            return statements -> fetch_stmt;
        };

        statement& get_insert_statement() {
            refresh_and_clear_bindings_of_statement(statements -> insert_stmt);
            return statements -> insert_stmt;
        }

        statement& get_delete_statement() {
            refresh_and_clear_bindings_of_statement(statements -> delete_stmt);
            return statements -> delete_stmt;
        }
        
        SQLiteInterface(const char* _directory) : directory(_directory) {
            initialize_connection();
            statements = std::make_unique<precompiled_sqliteStatements>(db_connection);
        };


};

class SavedURLBookmarkContainer {

    private:

        std::vector<SavedURLBookmark> SavedURLBookmarks;
        SQLiteInterface& SQLiteIntrfc;

        int current_index = 0;
        int size = 0;

    public:
    
        void ClearList() {

            SavedURLBookmarks.clear();
            current_index = 0;
            size = 0;

        }

        void insert_bookmark_into_db(SavedURLBookmark Bookmark) {
            SQLiteIntrfc.insert_bookmark(Bookmark);
        }
    
        std::string CharArrayToStringWithNullCheck(const char* str) {
            return (str == nullptr) ? "" : (std::string)(str);
        }

        void ParseDBQueryContentsToList() {

            statement& fetch_statement = SQLiteIntrfc.get_fetch_statement();
            sqlite3_stmt* stmt = fetch_statement.get();
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {

                int id = sqlite3_column_int(stmt, 0);
                const char* label = reinterpret_cast<const char*> (sqlite3_column_text(stmt, 1));
                const char* url = reinterpret_cast<const char*> (sqlite3_column_text(stmt, 2));
                const char* date = reinterpret_cast<const char*> (sqlite3_column_text(stmt, 3));
                
                SavedURLBookmark NewBookmark(
                    
                    id,
                    CharArrayToStringWithNullCheck(label),
                    CharArrayToStringWithNullCheck(url),
                    CharArrayToStringWithNullCheck(date)
                    
                );

                SavedURLBookmarks.push_back(NewBookmark);

            }

            size = SavedURLBookmarks.size();

        }

        void RefreshList() {

            ClearList();
            ParseDBQueryContentsToList();

        }

        void DeleteDBRowById(int id) {
            
            statement& delete_statement = SQLiteIntrfc.get_delete_statement();
            sqlite3_stmt* stmt = delete_statement.get();
            sqlite3_bind_int64(stmt, 1, id);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Error deleting bookmark." << std::endl;
            }

        }

        void DeleteBookFromListByIndex(int idx) {

            if ( (size <= 0) || IndexOutOfRange(idx, size) ){
                return;
            }

            SavedURLBookmarks.erase(SavedURLBookmarks.begin() + idx);
            current_index = std::max(0, current_index - 1);
            size--;

        }
        
        void DeleteCurrentBookListEntry() {
            DeleteBookFromListByIndex(current_index);
        }

        void DeleteHoveredRowFromListAndDB() {

            int id = SavedURLBookmarks[current_index].getid();
            DeleteDBRowById(id);
            DeleteCurrentBookListEntry();

        }

        int get_current_index() {
            return current_index;
        }

        void push_back(SavedURLBookmark Bookmark) {
            SavedURLBookmarks.push_back(Bookmark);
            size++;
        }

        void print() {

            for (int i = 0; i < size; ++i) {
                std::cout << SavedURLBookmarks[i].getid() << ", " << SavedURLBookmarks[i].getlabel() << std::endl;
            }

        }

        std::vector<SavedURLBookmark>& get_bookmarks_list() {
            return SavedURLBookmarks;
        }

        size_t get_size() {
            return size;
        }

        SavedURLBookmarkContainer(SQLiteInterface& _SQLiteIntrfc) : SQLiteIntrfc(_SQLiteIntrfc) {

            assert(SQLiteIntrfc.is_valid_connection());

            RefreshList();

        }


};

struct GLFWWindowFactory {

    GLFWwindow* GLFWWindow = nullptr;
    ImVec2 Dimensions;
    ImVec4 WindowColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    const char* WindowName;

    void InitializeGLFWWindow() {

        GLFWWindow = glfwCreateWindow(Dimensions.x, Dimensions.y, WindowName, nullptr, nullptr);

        if (GLFWWindow == nullptr) {
            throw std::runtime_error("Failed to initialize GLFWwindow. Exiting...");
        }

        glfwSetWindowSizeLimits(GLFWWindow, Dimensions.x, Dimensions.y, Dimensions.x, Dimensions.y);
        glfwMakeContextCurrent(GLFWWindow);
        glfwSwapInterval(1);

    }

    bool exists() {
        return (GLFWWindow != nullptr);
    }

    GLFWWindowFactory(const char* _WindowName, ImVec2 _Dimensions) {

        WindowName = _WindowName;
        Dimensions = _Dimensions;

    };



};

class GLFWInterface {

    private:

        bool is_active = false;
        GLFWWindowFactory Window;
        
        void setup() {
            
            if (!glfwInit()) {
                throw std::runtime_error("GLFW Lib Initialization Error. Exiting...");
            }

            Window.InitializeGLFWWindow();
            
            assert(Window.exists());

        }
        
        void GetFramebufferSize() {

            int width, height;

            glfwGetFramebufferSize(Window.GLFWWindow, &width, &height);

            Window.Dimensions = ImVec2(
                (float)width,
                (float)height
            );

        }
        
    public:
        
        bool exists() {
            return (Window.GLFWWindow == nullptr);
        }
        
        void GLFWRenderFrameProcess() {

            GetFramebufferSize();
            glViewport(0, 0, Window.Dimensions.x, Window.Dimensions.y);
            glClearColor(
                Window.WindowColor.x * Window.WindowColor.w, 
                Window.WindowColor.y * Window.WindowColor.w, 
                Window.WindowColor.z * Window.WindowColor.w, 
                Window.WindowColor.w
            );
            glClear(GL_COLOR_BUFFER_BIT);

        }

        bool is_still_open() {
            return (!glfwWindowShouldClose(Window.GLFWWindow));
        }

        void GLFWSwapBuffers() {
            glfwSwapBuffers(Window.GLFWWindow);
        }

        GLFWwindow* GetGLFWWindow() {
            return Window.GLFWWindow;
        }
        
        GLFWInterface(const char* _WindowName, ImVec2 Dimensions) : Window(_WindowName, Dimensions) {

            setup();
            assert(Dimensions.x && Dimensions.y);
            
        }

};

class ImGuiInterface {

    private:

        ImGuiIO* io;
        GLFWInterface* GLFWInterface_Reference;
        
        void setup() { 
            
            io -> IniFilename = "../build/imgui.ini";
            io -> ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            
            ImGui_ImplGlfw_InitForOpenGL(GLFWInterface_Reference -> GetGLFWWindow(), true);          
            ImGui_ImplOpenGL3_Init();
            
            IMGUI_CHECKVERSION();
    
        };
        
        
    public:
        
        ImGuiInterface(GLFWInterface &_GLFWInterface) : GLFWInterface_Reference(&_GLFWInterface) {
    
            ImGui::CreateContext();
            io = &ImGui::GetIO();
            setup();
    
        }
        
        void ImGuiNewFrameSetup() {

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

        }

        void ImGuiRenderWithGLFWProcess() {

            ImGui::Render();

            GLFWInterface_Reference -> GLFWRenderFrameProcess();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            GLFWInterface_Reference -> GLFWSwapBuffers();

        }

        void ImGuiShutdownProcess() {

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            
        }

    };

void iterTextFile(const char* Path) {

    std::fstream fs(Path);
    std::string buffer;
    std::getline(fs, buffer);

    int bufferToInt = std::stoi(buffer);
    bufferToInt += 1;
    
    fs.clear();
    fs.seekp(0);

    fs << bufferToInt;
    fs.close();

}

void PrintMousePos() {

    ImVec2 pos = ImGui::GetMousePos();
    std::cout << "Clicked Position At: [" << pos.x << "," << pos.y << "]" << std::endl;

}

void ImGuiLayout_TextHeadersChild() {

    ImGui::SetCursorPos(ImVec2(12,10));
	ImGui::BeginChild(8, ImVec2(543,28), true);

	ImGui::SetCursorPos(ImVec2(7.5,6));
	ImGui::Text("Label:");

	ImGui::SetCursorPos(ImVec2(108,6));
	ImGui::Text("URL:");

	ImGui::SetCursorPos(ImVec2(420,6));
	ImGui::Text("Date Added:");

	ImGui::EndChild();

}

bool SelectedEqualToCurrent(int first, int second) {
    return first == second;
}


const int URL_BUFFER_SIZE = 128;
char url_text_buffer[URL_BUFFER_SIZE] = "";
void ImGuiLayout_URLButtonsAndInputChild(SavedURLBookmarkContainer& ListContainer) {

    ImVec2 origin(12, 180);
    int child_width = 542;
    int child_height = 34;

    ImGui::SetCursorPos(ImVec2(origin.x,origin.y));
	ImGui::BeginChild(14, ImVec2(child_width, child_height), true);

    ImGui::SetCursorPos(ImVec2(6,8));
	ImGui::Text("Add String:");

    ImGui::SetCursorPos(ImVec2(85,6));
	if (ImGui::InputText(
        "##url_input", 
        url_text_buffer, 
        IM_ARRAYSIZE(url_text_buffer), 
        ImGuiInputTextFlags_EnterReturnsTrue)) {

            std::string buffer_copy = url_text_buffer;

            SavedURLBookmark NewBookmark(
                10,
                "blarg",
                buffer_copy,
                "12/12/2024"
            );

            ListContainer.insert_bookmark_into_db(NewBookmark);
            ListContainer.push_back(NewBookmark);
            std::cout << "pushed back" << buffer_copy << std::endl;
            url_text_buffer[0] = '\0';

    }   

    ImGui::EndChild();

}

void ImGuiLayout_URLListBoxChild(SavedURLBookmarkContainer ListContainer) {

    ImGui::SetCursorPos(ImVec2(12,45));
	ImGui::BeginChild(13, ImVec2(550,-13), false);

	ImGui::SetCursorPos(ImVec2(0,0));

    std::vector<SavedURLBookmark> List = ListContainer.get_bookmarks_list();
    static int current_item = 0;
    if (ImGui::BeginListBox("##", ImVec2(542, 130))) {
        for (size_t i = 0; i < ListContainer.get_size(); ++i) {
            
            if (ImGui::Selectable(
                List[i].formatted_display_string(), 
                SelectedEqualToCurrent(i, current_item), 
                ImGuiSelectableFlags_AllowDoubleClick)) {

                    current_item = i;
            
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                std::cout << "Double clicked: " << i << std::endl;
            }
            
        }
        ImGui::EndListBox();
    }

	ImGui::EndChild();

}

static bool ImGuiWindow_MainWindow_Status = true;
void ImGuiWindow_MainWindow(SavedURLBookmarkContainer& ListContainer, ImVec2 dims) {

    ImGui::SetNextWindowSize(ImVec2(dims.x,dims.y));
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    if (ImGui::Begin(
        "window_name", 
        &ImGuiWindow_MainWindow_Status,
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar
    )) {
            ImGuiLayout_TextHeadersChild();
            ImGuiLayout_URLListBoxChild(ListContainer);
            ImGuiLayout_URLButtonsAndInputChild(ListContainer);
    }

    ImGui::End();

}



int main(int argc, char** argv) {

    ImVec2 dims(568, 220);
    SQLiteInterface SQLiteIntrfc("BOOKMARKS.db");
    GLFWInterface GLFWIntrfc(
        "URL Keeper", 
        dims
    );
    ImGuiInterface ImGuiIntrfc(GLFWIntrfc);
    SavedURLBookmarkContainer List(SQLiteIntrfc);

    while (GLFWIntrfc.is_still_open()) {
        
        glfwPollEvents();
        ImGuiIntrfc.ImGuiNewFrameSetup();
        
        ImGuiWindow_MainWindow(List, dims);
        
        ImGuiIntrfc.ImGuiRenderWithGLFWProcess();
        


        if (ImGui::IsMouseClicked(0)) {

            List.RefreshList();
            List.print();

            PrintMousePos();
        }

    }

    ImGuiIntrfc.ImGuiShutdownProcess();

    iterTextFile("iter_count.txt");

    return 0;

};