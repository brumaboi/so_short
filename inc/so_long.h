#ifndef SO_LONG_H
# define SO_LONG_H

# include <SDL2/SDL.h>
# include <SDL2/SDL_image.h>
# include <iostream>
# include <fstream>
# include <string>
# include <vector>
# include <algorithm>
# include <stdexcept>
# include <cmath>

constexpr char	FLOOR   = '0';
constexpr char	WALL    = '1';
constexpr char	PLAYER  = 'P';
constexpr char	EXIT    = 'E';
constexpr char	COLLECT = 'C';
constexpr int	TILE_SIZE   = 64;
constexpr int	VIEWPORT_W  = 800;
constexpr int	VIEWPORT_H  = 600;
constexpr int	MOVE_FRAMES = 4;

struct Vec2 {
	int x = 0;
	int y = 0;
};

struct Vec2f {
	float x = 0.0f;
	float y = 0.0f;
};

class Map {

	public:
		Map() = default;
		void	load(const std::string &path);
		void	validate();

		char	at(int row, int col) const;
		void	set(int row, int col, char c);
		int		rows() const;
		int		cols() const;
		int		collectibles() const;
		Vec2	playerPos() const;
		Vec2	exitPos() const;

		void	removeCollectible(int row, int col);
		void	movePlayer(int new_row, int new_col);

	private:
		std::vector<std::string>	_grid;
		int							_rows = 0;
		int							_cols = 0;
		int							_collectibles = 0;
		int							_exits = 0;
		int							_players = 0;
		Vec2						_playerPos;
		Vec2						_exitPos;

		void	_readFile(const std::string &path);
		void	_trimRows();
		void	_checkFormat(const std::string &path);
		void	_checkEmptyLines();
		void	_checkWalls();
		void	_countElements();
		void	_checkValidChars();
		void	_checkAccess();
		void	_flood(int col, int row, std::vector<std::vector<bool>> &visited, bool passExit, int &collectFound, bool &exitFound);
};

class Renderer {
	public:
		Renderer() = default;
		~Renderer();
		Renderer(const Renderer &)            = delete;
		Renderer &operator=(const Renderer &) = delete;

		void	init(const Map &map);
		void	render(const Map &map, Vec2f playerPx);
		void	updateCamera(const Map &map, Vec2f playerPx);
		void	markMapDirty();
		SDL_Window	*window() const;

	private:
		SDL_Window *_window = nullptr;
		SDL_Renderer *_renderer = nullptr;
		int	_camX = 0;
		int _camY = 0;

		SDL_Texture	*_texWall = nullptr;
		SDL_Texture	*_texFloor = nullptr;
		SDL_Texture	*_texPlayer = nullptr;
		SDL_Texture	*_texCollect = nullptr;
		SDL_Texture	*_texExit = nullptr;
		SDL_Texture	*_texExitOpen = nullptr;
		SDL_Texture	*_mapTex = nullptr;
		bool		_mapDirty = true;
		int			_mapTexW = 0;
		int			_mapTexH = 0;

		void			_loadTextures();
		void			_rebuildMapTex(const Map &map);
		SDL_Texture		*_loadOrFallback(const std::string &path, Uint8 r, Uint8 g, Uint8 b);
		SDL_Texture		*_makeColor(Uint8 r, Uint8 g, Uint8 b);
		void			_drawTile(SDL_Texture *tex, int col, int row);
		void			_drawTileAt(SDL_Texture *tex, float px, float py);
};

class Game {
	public:
		Game(const std::string &mapPath);
		void	run();

	private:
		Map			_map;
		Renderer	_renderer;
		bool		_running = false;
		bool		_animating = false;
		int			_animFrame = 0;
		Vec2f		_playerPx;
		Vec2f		_targetPx;
		Vec2f		_startPx;

		void	_handleEvents();
		void	_movePlayer(int dx, int dy);
		void	_update();
};

#endif