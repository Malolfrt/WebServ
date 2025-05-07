NAME        = webserv

CXX         = c++
CPPFLAGS    = -Wall -Wextra -Werror -std=c++98
HEADERS		=	include/Parsing.hpp	\
				include/WebServ.hpp	\
				include/Server.hpp	\
				include/Request.hpp

OBJ_PATH	= obj/

SRC_PATH	= src/
SRC_FILES   =	main.cpp			\
				Parsing.cpp			\
				Server.cpp			\
				Request.cpp			\
				# WebServ.cpp			\

SRC			= $(addprefix $(SRC_PATH), $(SRC_FILES))
OBJ_FILES   = $(SRC_FILES:.cpp=.o)
OBJS		= $(addprefix $(OBJ_PATH), $(OBJ_FILES))

all: $(OBJ_PATH) $(NAME)

$(OBJ_PATH)%.o: $(SRC_PATH)%.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(OBJ_PATH):
	mkdir -p $(OBJ_PATH)

$(NAME): $(OBJS)
	$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -rf $(OBJ_PATH)

fclean: clean
	rm -f ${NAME}

re: fclean all

run: all
	./webserv ./config/default.conf

v: all
	valgrind --leak-check=full --track-origins=yes -s ./webserv ./config/default.conf

.PHONY: all clean fclean re

