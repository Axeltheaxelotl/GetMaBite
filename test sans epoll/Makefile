NAME		= webserv2
CXX			= c++
STD			= -std=c++98
CXXFLAGS	= -Wall -Wextra -Werror $(STD)
SRCS		= main.cpp ../srcs/parser/Parser.cpp ../srcs/parser/Server.cpp ../srcs/parser/Location.cpp
OBJS		= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	@printf "$(BLUE)> Compiling $(NAME)... <$(END)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)
	@printf "$(ERASE)$(BLUE)> $(NAME) created <$(END)\n"

%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@printf "$(ERASE)$(BLUE)> Compiling: $< <$(END)"

clean:
	rm -rf $(OBJS)
	@echo "Object files removed"

fclean: clean
	rm -f $(NAME)
	@echo "$(NAME) removed"

re: fclean all
