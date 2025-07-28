# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: alanty <alanty@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/02/05 15:24:34 by smasse            #+#    #+#              #
#    Updated: 2025/07/28 16:42:36 by alanty           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME      = webserv
CXX       = clang++ 
STD       = -std=c++98
CXXFLAGS  = -Wall -Wextra -O2 $(STD)
INCLUDES  = -Isrc -Isrc/serverConfig -Isrc/core -Isrc/config -Isrc/utils -Isrc/routes -I./src/core
DEBUG_FLAGS = -O0 -g3 $(STD)
OBJ_DIR   = ./objs

SRCS      = src/main.cpp \
            src/config/Parser.cpp \
            src/config/Location.cpp \
            src/config/Server.cpp \
            src/config/ServerNameHandler.cpp \
            src/core/EpollClasse.cpp \
            src/core/TimeoutManager.cpp \
            src/serverConfig/ServerConfig.cpp \
            src/utils/Utils.cpp \
            src/utils/Logger.cpp \
            src/routes/RouteHandler.cpp \
            src/routes/RedirectionHandler.cpp \
            src/routes/AutoIndex.cpp \
            src/http/RequestBufferManager.cpp \
            src/http/Cookie.cpp \
#             src/cgi/CgiHandler.cpp

OBJS      = $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

ERASE	:=	\033[2K\r
BLUE    :=  \033[34m
YELLOW  :=  \033[33m
GREEN   :=  \033[32m
END     :=  \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@printf "$(ERASE)$(BLUE)> Compiling $(NAME)... <$(END)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)
	@printf "$(ERASE)$(BLUE)> $(NAME) created <$(END)\n"

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@printf "$(ERASE)$(BLUE)> Compiling: $< <$(END)"

debug: CXXFLAGS = $(DEBUG_FLAGS)
debug:
	@printf "$(BLUE)> Debug mode <$(END)\n"
	@$(MAKE) all

clean:
	@printf "$(BLUE)> Removing object files... <$(END)"
	@rm -rf $(OBJ_DIR)
	@printf "$(ERASE)$(BLUE)> Object files removed <$(END)\n"

fclean: clean
	@printf "$(BLUE)> Removing executable $(NAME)... <$(END)"
	@rm -f $(NAME)
	@printf "$(ERASE)$(BLUE)> $(NAME) removed <$(END)\n"

re: fclean all

.PHONY: all clean fclean re debug