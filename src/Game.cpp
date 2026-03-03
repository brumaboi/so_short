#include "../inc/so_long.h"

Game::Game(const std::string &mapPath)
{
	_map.load(mapPath);
	_map.validate();
	_renderer.init(_map);
	Vec2 pp = _map.playerPos();
	_playerPx = { (float)(pp.x * TILE_SIZE), (float)(pp.y * TILE_SIZE) };
	_targetPx = _playerPx;
	_startPx = _playerPx;
}

void Game::run()
{
	_running = true;
	while (_running)
	{
		_handleEvents();
		_update();
		_renderer.updateCamera(_map, _playerPx);
		_renderer.render(_map, _playerPx);
	}
}

void Game::_update()
{
	if (!_animating)
		return;

	_animFrame++;
	if (_animFrame >= MOVE_FRAMES)
	{
		_playerPx = _targetPx;
		_animating = false;
		_animFrame = 0;
	}
	else
	{
		float t = (float)_animFrame / (float)MOVE_FRAMES;
		_playerPx.x = _startPx.x + (_targetPx.x - _startPx.x) * t;
		_playerPx.y = _startPx.y + (_targetPx.y - _startPx.y) * t;
	}
}

void Game::_handleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			_running = false;
			return;
		}
		if (event.type != SDL_KEYDOWN)
			continue;
		switch (event.key.keysym.sym)
		{
			case SDLK_ESCAPE:              _running = false;   break;
			case SDLK_UP:    case SDLK_w:  _movePlayer( 0, -1); break;
			case SDLK_DOWN:  case SDLK_s:  _movePlayer( 0,  1); break;
			case SDLK_LEFT:  case SDLK_a:  _movePlayer(-1,  0); break;
			case SDLK_RIGHT: case SDLK_d:  _movePlayer( 1,  0); break;
			default: break;
		}
	}
}

void Game::_movePlayer(int dx, int dy)
{
	if (_animating)
		return;

	Vec2 pos  = _map.playerPos();
	int  newX = pos.x + dx;
	int  newY = pos.y + dy;

	if (newX < 0 || newX >= _map.cols() || newY < 0 || newY >= _map.rows())
		return;

	char tile = _map.at(newY, newX);
	if (tile == WALL)
		return;
	if (tile == EXIT && _map.collectibles() > 0)
		return;

	if (tile == COLLECT)
	{
		_map.removeCollectible(newY, newX);
		_renderer.markMapDirty();
		if (_map.collectibles() == 0)
			_renderer.markMapDirty();
	}

	if (tile == EXIT && _map.collectibles() == 0)
	{
		_running = false;
		return;
	}

	_map.movePlayer(newY, newX);
	_startPx = _playerPx;
	_targetPx = { (float)(newX * TILE_SIZE), (float)(newY * TILE_SIZE) };
	_animating = true;
	_animFrame = 0;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./so_long <map.ber>" << std::endl;
		return 1;
	}
	try
	{
		Game game(argv[1]);
		game.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
