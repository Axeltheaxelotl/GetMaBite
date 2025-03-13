CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I./include  # Ajouter -I./include pour inclure le dossier include

SRC = src/main.cpp src/server.cpp src/request.cpp src/response.cpp
OBJ = $(SRC:.cpp=.o)
EXEC = webserv

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(OBJ) -o $(EXEC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
