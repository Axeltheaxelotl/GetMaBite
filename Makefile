# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: smasse <smasse@student.42luxembourg.lu>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/02/05 15:24:34 by smasse            #+#    #+#              #
#    Updated: 2025/03/14 11:24:05 by smasse           ###   ########.fr       #
#                                                                              #
# **************************************************************************** #

NAME      = webserv
CXX       = clang++ 
STD       = -std=c++98
CXXFLAGS  = -Wall -Wextra -Ofast -march=native -ffast-math -flto $(STD)
INCLUDES  = -I./include
DEBUG_FLAGS = -O0 -g3 -flto $(STD)
OBJ_DIR   = ./objs

SRCS      = ./srcs/main.cpp ./srcs/parser/Parser.cpp ./srcs/runner/Runner.cpp ./srcs/parser/Location.cpp ./srcs/parser/Server.cpp

OBJS      = $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

ERASE	:=	\033[2K\r
BLUE    :=  \033[34m
YELLOW  :=  \033[33m
GREEN   :=  \033[32m
END     :=  \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@printf "$(BLUE)> Compilation de $(NAME)... <$(END)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)
	@printf "$(ERASE)$(BLUE)> $(NAME) créé <$(END)\n"

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@printf "$(ERASE)$(BLUE)> Compilation: $< <$(END)"

debug: CXXFLAGS = $(DEBUG_FLAGS)
debug:
	@printf "$(BLUE)> Mode debug <$(END)\n"
	@$(MAKE) all

clean:
	@printf "$(BLUE)> Suppression des fichiers objets... <$(END)"
	@rm -rf $(OBJ_DIR)
	@printf "$(ERASE)$(BLUE)> Fichiers objets supprimés <$(END)\n"

fclean: clean
	@printf "$(BLUE)> Suppression de l'exécutable $(NAME)... <$(END)"
	@rm -f $(NAME)
	@printf "$(ERASE)$(BLUE)> $(NAME) supprimé <$(END)\n"

re: fclean all

.PHONY: all clean fclean re debug