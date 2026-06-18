# Makefile dla FrameLab Hub (C++ + Dear ImGui)

EXE = framelab_hub
SOURCES = main.cpp
SOURCES += imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
SOURCES += imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(SOURCES)))

UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++17 -Iimgui -Iimgui/backends -O2 -Wall
LIBS =

ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl
	# Spróbuj użyć pkg-config dla glfw3, z fallbackiem na standardowe -lglfw i -lX11
	GLFW_LIBS := $(shell pkg-config --libs glfw3 2>/dev/null || echo "-lglfw -lX11 -lpthread")
	GLFW_CFLAGS := $(shell pkg-config --cflags glfw3 2>/dev/null || echo "")
	LIBS += $(GLFW_LIBS)
	CXXFLAGS += $(GLFW_CFLAGS)
endif

all: $(EXE)
	@echo "Kompilacja zakończona pomyślnie dla $(ECHO_MESSAGE)!"

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

# Reguła kompilacji plików cpp w podfolderach
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(EXE) $(OBJS)

.PHONY: all clean
