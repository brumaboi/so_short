NAME			=	so_short
CXX				=	g++
CXXFLAGS		=	-Wall -Wextra -Werror -std=c++17
RM				=	rm -f

SDL_CFLAGS		=	$(shell sdl2-config --cflags)
SDL_LIBS		=	$(shell sdl2-config --libs) -lSDL2_image

INC				= 	inc/
SRC_DIR			= 	src/
OBJ_DIR			= 	obj/
	   
SRC 			= $(SRC_DIR)Game.cpp \
				  $(SRC_DIR)Map.cpp \
				  $(SRC_DIR)MazeGenerator.cpp \
				  $(SRC_DIR)Renderer.cpp
				  
		
OBJ = $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRC))

all : $(NAME)

$(NAME) : $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ) $(SDL_LIBS)

$(OBJ_DIR)%.o:	$(SRC_DIR)%.cpp
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -I $(INC) -c $< -o $@

clean :
	@$(RM) -f $(OBJ)
	@$(RM) -rf $(OBJ_DIR)

fclean : clean
	@$(RM) $(NAME)

re : fclean all
.PHONY : all clean fclean re
