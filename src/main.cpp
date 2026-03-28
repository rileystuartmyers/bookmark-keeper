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

struct int_pair {
    int x;
    int y;
};

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
                "SELECT id, url, date FROM BOOKMARKS;"
            )
        ),

        insert_stmt(
            create_statement(
                db,
                "INSERT INTO BOOKMARKS(url, date) VALUES (?, ?);"
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
    

        SQLiteInterface(const char* _directory) : directory(_directory) {

            initialize_connection();
            statements = std::make_unique<precompiled_sqliteStatements>(db_connection);
            printf("return_code == %d\n", return_code);

        };

};

struct WindowDimensions {
    
    int width;
    int height;
    int virtual_width;
    int virtual_height;

    void SetDimensions(int _width, int _height) {
        
        width = _width;
        height = _height;

        virtual_width = _width;
        virtual_height = _height;

    }

    void SetDimensions(int_pair _Dimensions) {
        SetDimensions(_Dimensions.x, _Dimensions.y);
    }
    
    WindowDimensions(int_pair _Dimensions) {
        SetDimensions(_Dimensions.x, _Dimensions.y);
    }

    WindowDimensions(int _width, int _height) {
        SetDimensions(_width, _height);    
    }   

    WindowDimensions() {};


};

struct GLFWWindowFactory {

    GLFWwindow* GLFWWindow = nullptr;
    WindowDimensions WindowDims;
    ImVec4 WindowColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    const char* WindowName;

    void InitializeGLFWWindow() {

        GLFWWindow = glfwCreateWindow(WindowDims.width, WindowDims.height, WindowName, nullptr, nullptr);

        if (GLFWWindow == nullptr) {
            throw std::runtime_error("Failed to initialize GLFWwindow. Exiting...");
        }

        glfwSetWindowSizeLimits(GLFWWindow, WindowDims.width, WindowDims.height, WindowDims.width, WindowDims.height);
        glfwMakeContextCurrent(GLFWWindow);
        glfwSwapInterval(1);

    }

    bool exists() {
        return (GLFWWindow != nullptr);
    }

    GLFWWindowFactory(const char* _WindowName, int_pair _Dimensions) {

        WindowName = _WindowName;
        WindowDims.SetDimensions(_Dimensions);

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
        
        
    public:
        
        bool exists() {
            return (Window.GLFWWindow == nullptr);
        }
        
        void GLFWRenderFrameProcess() {

            glfwGetFramebufferSize(Window.GLFWWindow, &Window.WindowDims.width, &Window.WindowDims.height);
            glViewport(0, 0, Window.WindowDims.width, Window.WindowDims.height);
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
        
        GLFWInterface(const char* _WindowName, int_pair Dimensions) : Window(_WindowName, Dimensions) {

            setup();
            assert(Dimensions.x && Dimensions.y);
            
        }

};

class ImGuiInterface {

    private:

        ImGuiIO* io;
        GLFWInterface GLFWInterface_Reference;
        
        void setup() { 
            
            io -> IniFilename = "../build/imgui.ini";
            io -> ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            
            ImGui_ImplGlfw_InitForOpenGL(GLFWInterface_Reference.GetGLFWWindow(), true);          
            ImGui_ImplOpenGL3_Init();
            
            IMGUI_CHECKVERSION();
    
        };
        
        
    public:
        
        ImGuiInterface(GLFWInterface _GLFWInterface) : GLFWInterface_Reference(_GLFWInterface) {
    
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

            GLFWInterface_Reference.GLFWRenderFrameProcess();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            GLFWInterface_Reference.GLFWSwapBuffers();

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
    std::cout << pos.x << "," << pos.y << std::endl;

}

void ImGuiLayout_TextHeadersChild() {

    int ChildWidth, ChildHeight;

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
void ImGuiLayout_URLButtonsAndInputChild(std::vector<std::string>& strings) {

    ImVec2 origin(12, 180);
    int child_width = 542;
    int child_height = 34;
    

    ImGui::SetCursorPos(ImVec2(origin.x,origin.y));
	ImGui::BeginChild(14, ImVec2(child_width, child_height), true);

    ImGui::SetCursorPos(ImVec2(6,8));
	ImGui::Text("Add String:");

    ImGui::SetCursorPos(ImVec2(85,6));
	ImGui::InputText("##url_input", url_text_buffer, IM_ARRAYSIZE(url_text_buffer));
    if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {

        std::string buffer_copy = url_text_buffer;
        strings.push_back(buffer_copy);

        url_text_buffer[0] = '\0';

    }   

    ImGui::EndChild();

}

std::vector<std::string> strings = {"Never", "Gonna", "Give", "You", "Up"};
void ImGuiLayout_URLListBoxChild() {

    ImGui::SetCursorPos(ImVec2(12,45));
	ImGui::BeginChild(13, ImVec2(550,-13), false);

	ImGui::SetCursorPos(ImVec2(0,0));
    
	static int current_item = 0;
    if (ImGui::BeginListBox("##", ImVec2(542, 130))) {
        for (int i = 0; i < strings.size(); ++i) {
            if (ImGui::Selectable(strings[i].c_str(), SelectedEqualToCurrent(i, current_item), ImGuiSelectableFlags_AllowDoubleClick)) {
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
void ImGuiWindow_MainWindow(int_pair dims) {

    ImGui::SetNextWindowSize(ImVec2(dims.x,dims.y));
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    if (ImGui::Begin(
        "window_name", 
        &ImGuiWindow_MainWindow_Status,
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar
    ))
    {
        ImGuiLayout_TextHeadersChild();
        ImGuiLayout_URLListBoxChild();
        ImGuiLayout_URLButtonsAndInputChild(strings);
    }

    ImGui::End();

}



int main(int argc, char** argv) {

    int_pair dims = {568, 220};
    SQLiteInterface SQLiteIntrfc("BOOKMARKS.db");
    GLFWInterface GLFWIntrfc("URL Keeper", dims);
    ImGuiInterface ImGuiIntrfc(GLFWIntrfc);

    while (GLFWIntrfc.is_still_open()) {
        
        glfwPollEvents();
        ImGuiIntrfc.ImGuiNewFrameSetup();
        
        ImGuiWindow_MainWindow(dims);
        
        ImGuiIntrfc.ImGuiRenderWithGLFWProcess();
        
        PrintMousePos();

    }

    ImGuiIntrfc.ImGuiShutdownProcess();

    iterTextFile("iter_count.txt");

    return 0;

};