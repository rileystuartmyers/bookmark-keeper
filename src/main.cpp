#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>

#include <assert.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <sqlite3.h>

#if defined(_WIN32)
    #include <windows.h>
#endif

static constexpr long unsigned int BUFFER_SIZE = 128;

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

        int MAX_LABEL_LENGTH = 14;
        int MAX_URL_LENGTH = 45;
        int MAX_DATE_LENGTH = 10;
        int INTER_STRING_SPACE_BUFFER = 2;

    public:

        std::string ShortenedStringIfLongerThanN(std::string str, size_t n) {

            if ( (n <= 3) || (str.length() <= n) ){
                return str;
            }

            std::string shortened_str = str.substr(0, n - 3);
            shortened_str += "...";
            return shortened_str;

        }

        std::string BackfilledStringWithSpacesToLengthOfN(std::string str, size_t n) {

            if (str.length() < n) {
                std::string filler_str(n - str.length(), ' ');
                return str + filler_str;
            } else{
                return str;
            }
        }

        SavedURLBookmark(int _id, std::string _label, std::string _url, std::string _date) {

            id = _id;
            label = _label;
            url = _url;
            date = _date;

            
            std::string label_substr = ShortenedStringIfLongerThanN(label, MAX_LABEL_LENGTH);
            std::string label_section = BackfilledStringWithSpacesToLengthOfN(label_substr, MAX_LABEL_LENGTH + INTER_STRING_SPACE_BUFFER);

            std::string url_substr = ShortenedStringIfLongerThanN(url, MAX_URL_LENGTH);
            std::string url_section = BackfilledStringWithSpacesToLengthOfN(url_substr, MAX_URL_LENGTH + INTER_STRING_SPACE_BUFFER);
            
            std::string date_section = ShortenedStringIfLongerThanN(date, MAX_DATE_LENGTH);

            row_display_content = label_section + url_section + date_section;
        
        }

        int getid() {
            return id;
        }

        void updateid(int _id) {
            id = _id;
        }

        std::string getlabel() {
            return label;
        }

        void setlabel(std::string _label) {
            label = _label;
        }

        void seturl(std::string _url) {
            url = _url;
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
    statement update_stmt;
    statement download_stmt;
    statement cleartable_stmt;
    statement lastindex_stmt;
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

        update_stmt(
            create_statement(
                db,
                "UPDATE BOOKMARKS SET url = ?, label = ? WHERE id = ?;"
            )
        ),
        
        download_stmt(
            create_statement(
                db,
                "SELECT url FROM BOOKMARKS;"
            )
        ),

        cleartable_stmt(
            create_statement(
                db,
                "DELETE FROM BOOKMARKS;"
            )
        ),

        lastindex_stmt(
            create_statement(
                db,
                "SELECT last_insert_rowid();"
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
    
        int return_code;

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
                std::cerr << "Insertion failed :::> " << sqlite3_errmsg(db_connection) << '\n';
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
                std::cerr << "Insertion failed :::> " << sqlite3_errmsg(db_connection) << '\n';
            }

            refresh_and_clear_bindings_of_statement(insert_statement);

        }

        void update_bookmark(int id, std::string NewURL, std::string NewLabel) {

            // "UPDATE BOOKMARKS SET url = ?, label = ? WHERE id = ?;"

            statement& update_statement = statements -> update_stmt;
            sqlite3_stmt* query = update_statement.get();
            sqlite3_bind_text(query, 1, NewURL.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(query, 2, NewLabel.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(query, 3, id);

            return_code = sqlite3_step(query);
            if (return_code != SQLITE_DONE) {
                std::cerr << "Update failed :::> " << sqlite3_errmsg(db_connection) << '\n';
            }

            refresh_and_clear_bindings_of_statement(update_statement);

        }

        void clear_table() {

            statement& cleartable_statement = statements -> cleartable_stmt;
            sqlite3_stmt* stmt = cleartable_statement.get();

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::runtime_error("Failed to clear table...");
            }

            refresh_statement(cleartable_statement);

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

        statement& get_update_statement() {
            refresh_and_clear_bindings_of_statement(statements -> update_stmt);
            return statements -> update_stmt;
        }

        statement& get_cleartable_statement() {
            refresh_statement(statements -> cleartable_stmt);
            return statements -> cleartable_stmt;
        }

        statement& get_lastindex_statement() {
            refresh_statement(statements -> lastindex_stmt);
            return statements -> lastindex_stmt;
        }
        
        SQLiteInterface(const char* _directory) : directory(_directory) {
            initialize_connection();
            statements = std::make_unique<precompiled_sqliteStatements>(db_connection);
        };


};

class SavedURLBookmarkContainer {

    private:

    SQLiteInterface& SQLiteIntrfc;
    int size = 0;
    
    public:
    
        std::vector<SavedURLBookmark> SavedURLBookmarks;
        int current_index = 0;

        void ClearList() {

            SavedURLBookmarks.clear();
            current_index = 0;
            size = 0;

        }

        int get_return_code() {
            return SQLiteIntrfc.return_code;
        }

        int get_current_book_id() {
            return SavedURLBookmarks[current_index].getid();
        }

        std::string get_current_label() {
            return SavedURLBookmarks[current_index].getlabel();
        }

        std::string get_current_url() {
            return SavedURLBookmarks[current_index].geturl();
        }

        std::string get_url_at_index(int idx) {

            if (IndexOutOfRange(idx, size)) {
                throw std::runtime_error("Failed to get URL at index... out of range.");
            }

            return SavedURLBookmarks[idx].geturl();
           
        }

        bool StringLengthIsInvalid(std::string InputString) {
            if (InputString.length() <= 0 || InputString.length() >= BUFFER_SIZE) {
                return true;
            } else {
                return false;
            }
        }

        void insert_bookmark_into_db(SavedURLBookmark Bookmark) {
            SQLiteIntrfc.insert_bookmark(Bookmark);
        }

        void update_bookmark_in_db(int id, const std::string& NewURL, const std::string& NewLabel) {

            if (StringLengthIsInvalid(NewURL) || StringLengthIsInvalid(NewLabel)) {
                return;
            }

            SQLiteIntrfc.update_bookmark(id, NewURL, NewLabel);
            
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

            int current_index_temp = current_index;

            ClearList();
            ParseDBQueryContentsToList();
            
            if (current_index_temp >= size) {
                current_index_temp = size - 1;
            } else {
                current_index = current_index_temp;
            }
            
        }

        void DeleteDBRowById(int id) {
            
            statement& delete_statement = SQLiteIntrfc.get_delete_statement();
            sqlite3_stmt* stmt = delete_statement.get();
            sqlite3_bind_int64(stmt, 1, id);

            
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Error deleting bookmark." << '\n';
            }

            SQLiteIntrfc.refresh_and_clear_bindings_of_statement(delete_statement);

        }

        void DeleteBookFromListByIndex(int idx) {

            if ( (size <= 0) || IndexOutOfRange(idx, size) ){
                return;
            }

            SavedURLBookmarks.erase(SavedURLBookmarks.begin() + idx);

            current_index = std::max(0, current_index - 1);
            size--;

        }

        void UpdateCurrentBookListEntry(std::string NewURL, std::string NewLabel) {

            if (StringLengthIsInvalid(NewURL) || StringLengthIsInvalid(NewLabel)) {
                return;
            }
            
            SavedURLBookmark* CurrentBookmark = &SavedURLBookmarks[current_index];
    
            CurrentBookmark -> setlabel(NewLabel);
            CurrentBookmark -> seturl(NewURL);

        }
        
        void DeleteCurrentBookListEntry() {
            DeleteBookFromListByIndex(current_index);
        }

        void DeleteHoveredRowFromListAndDB() {
            int id = SavedURLBookmarks[current_index].getid();
            DeleteCurrentBookListEntry();
            DeleteDBRowById(id);
        }

        void DeleteRowAtIndexFromListAndDB(int idx) {

            if (IndexOutOfRange(idx, size)) {
                throw std::runtime_error("Failed to delete row... index out of range.");
            }

            int id = SavedURLBookmarks[idx].getid();
            DeleteCurrentBookListEntry();
            DeleteDBRowById(id);

        }

        int get_current_index() {
            return current_index;
        }

        int get_lastindex() {

            statement& insert_statement = SQLiteIntrfc.get_lastindex_statement();
            SQLiteIntrfc.refresh_statement(insert_statement);

            sqlite3_stmt* stmt = insert_statement.get();

            if (sqlite3_step(stmt) != SQLITE_ROW) {
                throw std::runtime_error("Failed to retrieve last index from DB...");
            }


            int last_index = sqlite3_column_int(stmt, 0);

            return last_index;

        }

        void push_back(SavedURLBookmark Bookmark) {
            SavedURLBookmarks.push_back(Bookmark);
            size++;
        }

        void print() {

            for (int i = 0; i < size; ++i) {
                std::cout << SavedURLBookmarks[i].getid() << ", " << SavedURLBookmarks[i].getlabel() << '\n';
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
            return (Window.GLFWWindow != nullptr);
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
    std::cout << "Clicked Position At: [" << pos.x << "," << pos.y << "]" << '\n';

}

void ImGuiLayout_TextHeadersChild() {

    ImGui::SetCursorPos(ImVec2(12,10));
	ImGui::BeginChild(8, ImVec2(543,28), true);

	ImGui::SetCursorPos(ImVec2(7.5,6));
	ImGui::Text("Label:");

	ImGui::SetCursorPos(ImVec2(116,6));
	ImGui::Text("URL:");

	ImGui::SetCursorPos(ImVec2(445,6));
	ImGui::Text("Date Added:");

	ImGui::EndChild();

}

bool SelectedEqualToCurrent(int first, int second) {
    return first == second;
}

std::string GetMDYDateAsString() {

    char date_str[11];
    tm local_time;
    time_t now = time(0);

    #if defined(_WIN32)
        localtime_s(&local_time, &now);
    #else
        localtime_r(&now, &local_time);
    #endif

    strftime(date_str, sizeof(date_str), "%m/%d/%Y", &local_time);

    return date_str;

}

std::string GetHostNameFromURL(std::string url) {

    if (url.length() <= 3) {
        return url;
    }

    std::string url_hostname;
    int start_index = 0;
    int end_index = (url.length() - 1);

    for (size_t i = 0; i < (size_t)end_index; ++i) {
        if ( (url[i] == '/') && (url[i + 1] == '/') ) {
            start_index = i + 2;
            break;
        }
    }

    for (size_t i = 0; i < url.length() - 3; ++i) {

        if (
            (url[i] == 'w'     || url[i] == 'W') && 
            (url[i + 1] == 'w' || url[i + 1] == 'W') && 
            (url[i + 2] == 'w' || url[i + 2] == 'W') && 
            (url[i + 3] == '.')
        ) {
            start_index = i + 4;
            break;
        }
    }
        
    for (size_t i = start_index; i < url.length(); ++i) {
        if (url[i] == '.') {
            end_index = i - 1;
        }
    }
    
    url_hostname = url.substr(start_index, end_index - start_index + 1);
    return url_hostname;

}

void AddBufferContentsToList(SavedURLBookmarkContainer& ListContainer, char* url_text_buffer) {

    //Determine last index 
    if (url_text_buffer[0] == '\0') {
        return;
    }

    std::string url_hostname = GetHostNameFromURL(url_text_buffer);
    std::string url_buffer_copy = url_text_buffer;
    std::string date_str = GetMDYDateAsString();

    SavedURLBookmark NewBookmark(
        -1,
        url_hostname,
        url_buffer_copy,
        date_str
    );

    ListContainer.insert_bookmark_into_db(NewBookmark);
    int DBAutoIncrementedID = ListContainer.get_lastindex();

    NewBookmark.updateid(DBAutoIncrementedID);

    ListContainer.push_back(NewBookmark);

    url_text_buffer[0] = '\0';

}

void DeleteAllInstancesOfCurrentRow(SavedURLBookmarkContainer& ListContainer) {

    if (ListContainer.get_size() <= 0) {
        return;
    }

    ListContainer.DeleteHoveredRowFromListAndDB();

}

void DeleteAllInstancesOfRowWithIndex(SavedURLBookmarkContainer& ListContainer, int idx) {

    if ( (ListContainer.get_size() <= 0) || IndexOutOfRange(idx, ListContainer.get_size()) ){
        return;
    }

    ListContainer.DeleteHoveredRowFromListAndDB();

}

void CopyStringToClipboard(GLFWwindow* Window, const std::string& str) {
    glfwSetClipboardString(Window, str.c_str());
}

void LINUXSYSTEM_PasteStringToClipboard(std::string str) {
    std::string cmd = "echo \"" + str + "\"| xclip -selection clipboard";
    system(cmd.c_str());
}

struct ImGuiWindow_EditWindowInstance {

    bool is_active = false;

    int CurrentID = 0;
    char CurrentLabelArr[BUFFER_SIZE];
    char CurrentURLArr[BUFFER_SIZE];
    
    SavedURLBookmarkContainer& ListContainerReference;

    ImVec2 Dimensions = {300, 140};
    ImVec2 Origin;

    int TextChildBox_Width = 280;
    int TextChildBox_Height = 34;
    int TextChildBox_LeftXValue = 0;
    
    int InputText_LeftChild_Padding = 100;
    int Inputtext_UpperChild_Padding = 7;
    int InputText_Width_Value = 175;

    int EnterBoxWidth = 46;
    int EnterBoxHeight = 20;
    int EnterBox_CornerMargins_Value = 10;

    ImGuiWindow_EditWindowInstance(SavedURLBookmarkContainer& ListContainer, ImVec2& Window_Dimensions) : ListContainerReference(ListContainer) {

        Origin = {
            (Window_Dimensions.x - Dimensions.x) / 2, 
            (Window_Dimensions.y - Dimensions.y) / 2
        };

    };

    bool IsActive() {
        return is_active;
    }

    void Activate() {

        
        is_active = true;
        
        CurrentID = ListContainerReference.get_current_book_id();

        std::string CurrentURLString = ListContainerReference.get_current_url();
        std::string CurrentLabelString = ListContainerReference.get_current_label();
        strcpy(CurrentURLArr, CurrentURLString.c_str());
        strcpy(CurrentLabelArr, CurrentLabelString.c_str());

    }

    void Deactivate() {

        is_active = false;

        CurrentID = 0;

        memset(CurrentURLArr, 0, sizeof(CurrentURLArr));
        memset(CurrentLabelArr, 0, sizeof(CurrentLabelArr));

    }

    void UpdateURLInContainerAndDB() {

        ListContainerReference.update_bookmark_in_db(CurrentID, CurrentURLArr, CurrentLabelArr);
        ListContainerReference.UpdateCurrentBookListEntry(CurrentURLArr, CurrentLabelArr);
        ListContainerReference.RefreshList();

    }
    
    void Display() {
        
        ImGui::SetNextWindowPos(Origin);
        ImGui::SetNextWindowSize(Dimensions);

        if (ImGui::Begin(
            "###EditWindow", 
            &is_active,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar
        )) {    

            int TitleWidth = ImGui::CalcTextSize("Edit an entry:").x;
            int Title_LeftXValue = (Dimensions.x - TitleWidth) / 2;

            //@begin EditWindowTitleChildBox
            ImGui::SetCursorPos(ImVec2(Title_LeftXValue, 10));
            ImGui::Text("Edit an entry:");


            //@begin SavedURLChildBox
            ImGui::SetCursorPos(ImVec2(TextChildBox_LeftXValue, 30));
            ImGui::BeginChild(4, ImVec2(TextChildBox_Width, TextChildBox_Height), false);

            ImGui::SetCursorPos(ImVec2(18,11));
            ImGui::Text("Saved URL:");

            ImGui::SetCursorPos(ImVec2(InputText_LeftChild_Padding, Inputtext_UpperChild_Padding));
            ImGui::PushItemWidth(InputText_Width_Value); //NOTE: (Push/Pop)ItemWidth is optional
            if (ImGui::InputText("##SavedURL_Label", CurrentLabelArr, IM_ARRAYSIZE(CurrentLabelArr), ImGuiInputTextFlags_EnterReturnsTrue)) {

                UpdateURLInContainerAndDB();
                Deactivate();

            }
            ImGui::PopItemWidth();

            ImGui::EndChild();

            //@begin LabelChildBox
            ImGui::SetCursorPos(ImVec2(TextChildBox_LeftXValue, 70));
            ImGui::BeginChild(8, ImVec2(TextChildBox_Width, TextChildBox_Height), false);

            ImGui::SetCursorPos(ImVec2(47,10));
            ImGui::Text("Label:");

            ImGui::SetCursorPos(ImVec2(InputText_LeftChild_Padding, Inputtext_UpperChild_Padding));
            ImGui::PushItemWidth(InputText_Width_Value); //NOTE: (Push/Pop)ItemWidth is optional
            if (ImGui::InputText("##URLLabel_Label", CurrentURLArr, IM_ARRAYSIZE(CurrentURLArr), ImGuiInputTextFlags_EnterReturnsTrue)) {
                
                UpdateURLInContainerAndDB();
                Deactivate();

            }
            ImGui::PopItemWidth();

            ImGui::EndChild();


            //@begin EnterChildBox
            ImGui::SetCursorPos(
                ImVec2(
                    Dimensions.x - EnterBoxWidth - EnterBox_CornerMargins_Value, 
                    Dimensions.y - EnterBoxHeight - EnterBox_CornerMargins_Value
                )
            );
            ImGui::BeginChild(2, ImVec2(EnterBoxWidth, EnterBoxHeight), false, ImGuiWindowFlags_NoScrollbar);
            ImGui::SetCursorPos(ImVec2(0, 0));
            if (ImGui::Button("Enter", ImVec2(EnterBoxWidth, EnterBoxHeight))) {

                UpdateURLInContainerAndDB();
                Deactivate();

            }

            ImGui::EndChild();


        }

        ImGui::End();

    }
};

struct ImGuiWindow_MainWindowInstance {

    bool is_active = true;
    char url_text_buffer[BUFFER_SIZE] = "";
    
    ImVec2 Dimensions = {568, 220};
    ImVec2 Origin = {0, 0};
    GLFWInterface& GLFWInterfaceReference;
    SavedURLBookmarkContainer& ListContainerReference;
    ImGuiWindow_EditWindowInstance EditWindow;



    ImGuiWindow_MainWindowInstance(

        GLFWInterface& GLFWInterface, SavedURLBookmarkContainer& ListContainer
    
    ) : GLFWInterfaceReference(GLFWInterface), 
        ListContainerReference(ListContainer),
        EditWindow(ListContainerReference, Dimensions) 
        
        {

    }

    void Display() {

        ImGui::SetNextWindowSize(Dimensions);
        ImGui::SetNextWindowPos(Origin);

        if (ImGui::Begin(
            "window_name", 
            &is_active,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoBringToFrontOnFocus
        )) {

                //@begin TextHeadersChild
                ImGui::SetCursorPos(ImVec2(12,10));
                ImGui::BeginChild(8, ImVec2(543,28), true);

                ImGui::SetCursorPos(ImVec2(7.5,6));
                ImGui::Text("Label:");

                ImGui::SetCursorPos(ImVec2(116,6));
                ImGui::Text("URL:");

                ImGui::SetCursorPos(ImVec2(445,6));
                ImGui::Text("Date Added:");

                ImGui::EndChild();

                //@begin URLListBoxChild
                ImGui::SetCursorPos(ImVec2(12,45));
                ImGui::BeginChild(13, ImVec2(550,-13), false);

                ImGui::SetCursorPos(ImVec2(0,0));

                if (ImGui::BeginListBox("##", ImVec2(542, 130))) {
                    for (size_t i = 0; i < ListContainerReference.get_size(); ++i) {
                        
                        if (ImGui::Selectable(
                            ListContainerReference.SavedURLBookmarks[i].formatted_display_string(), 
                            (ListContainerReference.current_index == (int)i), 
                            ImGuiSelectableFlags_AllowDoubleClick)) {

                                ListContainerReference.current_index = i;
                        
                        }

                        if (ImGui::IsItemHovered()) {

                            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                EditWindow.Activate();

                            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                CopyStringToClipboard(GLFWInterfaceReference.GetGLFWWindow(), ListContainerReference.get_url_at_index(i));

                            } else if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                                DeleteAllInstancesOfCurrentRow(ListContainerReference);
                                ListContainerReference.RefreshList();

                            }
                        }
                    }
                    ImGui::EndListBox();

                }
                ImGui::EndChild();


                //@begin URLButtonsAndInputChild
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
                    ImGuiInputTextFlags_EnterReturnsTrue)) 
                    {
                        AddBufferContentsToList(ListContainerReference, url_text_buffer);
                        ImGui::SetKeyboardFocusHere(-1);
                }   

                ImVec2 AddButtonOrigin(458 - origin.x, 187 - origin.y);
                ImGui::SetCursorPos(ImVec2(458 - origin.x, 187 - origin.y));
                if (ImGui::Button(
                    "Add", 
                    ImVec2(490 - AddButtonOrigin.x - origin.x, 205 - AddButtonOrigin.y - origin.y))) 
                    {
                        AddBufferContentsToList(ListContainerReference, url_text_buffer);
                }

                ImVec2 DeleteButtonOrigin(497 - origin.x, 187 - origin.y);
                ImGui::SetCursorPos(ImVec2(497 - origin.x, 187 - origin.y));
                if (ImGui::Button(
                    "Delete", 
                    ImVec2(547 - DeleteButtonOrigin.x - origin.x, 205 - DeleteButtonOrigin.y - origin.y))) 
                    {
                        DeleteAllInstancesOfCurrentRow(ListContainerReference);
                        ListContainerReference.RefreshList();

                }

                ImGui::EndChild();


                if (EditWindow.IsActive()) {
                    EditWindow.Display();
                }

        }

        ImGui::End();
    }
};

int main(int argc, char** argv) {

    ImVec2 dims(568, 220);
    SQLiteInterface SQLiteIntrfc("BOOKMARKS.db");
    GLFWInterface GLFWIntrfc(
        "URL Keeper", 
        dims
    );
    ImGuiInterface ImGuiIntrfc(GLFWIntrfc);
    SavedURLBookmarkContainer ListContainer(SQLiteIntrfc);

    ImGuiWindow_MainWindowInstance MainWindow(GLFWIntrfc, ListContainer);

    if (argc > 1) {
        if (strcmp(argv[1], "clear") == 0) {
            SQLiteIntrfc.clear_table();
        } else if (strcmp(argv[1], "url") == 0) {

            for (int i = 2; i < argc; ++i) {
                std::cout << argv[i] << ":      " << GetHostNameFromURL(argv[i]) << '\n';
            }
        }
    }

    ListContainer.RefreshList();
    while (GLFWIntrfc.is_still_open()) {
        
        glfwPollEvents();
        ImGuiIntrfc.ImGuiNewFrameSetup();
        
        MainWindow.Display();
        

        ImGuiIntrfc.ImGuiRenderWithGLFWProcess();
        
    }

    ImGuiIntrfc.ImGuiShutdownProcess();

    iterTextFile("iter_count.txt");

    return 0;

};