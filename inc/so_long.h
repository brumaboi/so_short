#ifndef SO_LONG_H
# define SO_LONG_H

# include <SDL2/SDL.h>
# include <SDL2/SDL_image.h>
# include <iostream>
# include <string>
# include <vector>
# include <algorithm>
# include <stdexcept>
# include <cmath>
#include <random>
#include <stack>
#include <queue>
#include <utility>

constexpr char	FLOOR   = '0';
constexpr char	WALL    = '1';
constexpr char	PLAYER  = 'P';
constexpr char	EXIT    = 'E';
constexpr char	COLLECT = 'C';

constexpr int	TILE_SIZE   = 64;
constexpr float	PLAYER_SPEED = 300.0f;

struct Vec2 {
	int x = 0;
	int y = 0;
};

struct Vec2f {
	float x = 0.0f;
	float y = 0.0f;
};

enum class EntityKind { Collectible, Exit };

struct Entity {
	EntityKind	kind;
	Vec2		tile;
	bool		active = true;
};

struct PlayerObj {
	Vec2f	pos;
	Vec2f	vel;
	int		collected = 0;
};

std::vector<std::string> generateMaze(int level);

class Map {

	public:
		Map() = default;
		void	loadFromGrid(std::vector<std::string> grid);
		void	validate();

		void	extractEntities(PlayerObj &player, std::vector<Entity> &entities);

		char	at(int row, int col) const;
		int		rows() const;
		int		cols() const;
		bool	isWall(int row, int col) const;

	private:
		std::vector<std::string>	_grid;
		int							_rows = 0;
		int							_cols = 0;
		int							_collectibles = 0;
		int							_exits = 0;
		int							_players = 0;
		Vec2						_playerPos;

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

		void	init(const Map &map, Vec2f playerPx);
		void	render(const Map &map, const PlayerObj &player,
					   const std::vector<Entity> &entities, int totalCollectibles);
		void	renderPauseOverlay(int selectedItem, int level);
		void	present();
		void	updateCamera(const Map &map, Vec2f playerPx);
		void	rebuildMapTex(const Map &map);

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
		int			_mapTexW = 0;
		int			_mapTexH = 0;

		SDL_Texture	*_pauseTex = nullptr;
		int			_cachedPauseSel = -1;
		int			_cachedPauseLvl = -1;

		void			_loadTextures();
		void			_buildMapTex(const Map &map);
		SDL_Texture		*_loadOrFallback(const std::string &path, Uint8 r, Uint8 g, Uint8 b);
		SDL_Texture		*_makeColor(Uint8 r, Uint8 g, Uint8 b);
		void			_drawTile(SDL_Texture *tex, int col, int row);
		void			_drawTileAt(SDL_Texture *tex, float px, float py);
		void			_drawRect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
		void			_drawFilledRect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
		void			_buildPauseOverlay(int selectedItem, int level, int winW, int winH);
};

class Game {
	public:
		Game();
		void	run();

	private:
		Map					_map;
		Renderer			_renderer;
		PlayerObj			_player;
		std::vector<Entity>	_entities;
		int					_totalCollectibles = 0;
		int					_level = 1;
		bool				_running = false;
		bool				_paused = false;
		int					_pauseSelection = 0;

		Uint32				_lastTick = 0;
		bool				_rendererReady = false;

		void	_loadLevel();
		void	_handleEvents();
		void	_handlePauseEvents();
		void	_update(float dt);
		bool	_canMoveTo(float px, float py) const;
		void	_checkEntityCollisions();
};

#endif