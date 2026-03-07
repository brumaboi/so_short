#include "so_long.h"

Game::Game()
{
	_level = 1;
	_loadLevel();
	_renderer.init(_map, _player.pos);
	_rendererReady = true;
	_lastTick = SDL_GetTicks();
}

void Game::_loadLevel()
{
	auto grid = generateMaze(_level);
	_map = Map();
	_map.loadFromGrid(std::move(grid));
	_map.validate();
	_map.extractEntities(_player, _entities);
	_spawnPos = _player.pos;

	_totalCollectibles = 0;
	for (auto &e : _entities)
		if (e.kind == EntityKind::Collectible)
			_totalCollectibles++;

	if (_rendererReady)
		_renderer.rebuildMapTex(_map);
}

void Game::run()
{
	_running = true;
	constexpr Uint32 FRAME_MS = 16;
	while (_running)
	{
		Uint32 frameStart = SDL_GetTicks();
		Uint32 elapsed = frameStart - _lastTick;
		if (elapsed == 0) elapsed = 1;
		float dt = elapsed / 1000.0f;
		if (dt > 0.05f) dt = 0.05f;
		_lastTick = frameStart;

		if (_paused)
		{
			_handlePauseEvents();
			_renderer.render(_map, _player, _entities, _totalCollectibles);
			_renderer.renderPauseOverlay(_pauseSelection, _level);
			_renderer.present();
			SDL_WaitEvent(nullptr);
		}
		else
		{
			_handleEvents();
			_update(dt);
			_renderer.updateCamera(_map, _player.pos);
			_renderer.render(_map, _player, _entities, _totalCollectibles);
			_renderer.present();
			Uint32 frameTime = SDL_GetTicks() - frameStart;
			if (frameTime < FRAME_MS)
				SDL_Delay(FRAME_MS - frameTime);
		}
	}
}

bool Game::_canMoveTo(float px, float py) const
{
	const float margin = 2.0f;
	float left   = px + margin;
	float right  = px + TILE_SIZE - margin;
	float top    = py + margin;
	float bottom = py + TILE_SIZE - margin;

	auto blocked = [&](float fx, float fy) -> bool {
		int col = (int)(fx) / TILE_SIZE;
		int row = (int)(fy) / TILE_SIZE;
		return _map.isWall(row, col);
	};

	return !blocked(left, top)    && !blocked(right, top) &&
	       !blocked(left, bottom) && !blocked(right, bottom);
}

void Game::_checkEntityCollisions()
{
	float cx = _player.pos.x + TILE_SIZE / 2.0f;
	float cy = _player.pos.y + TILE_SIZE / 2.0f;
	int pcol = (int)(cx) / TILE_SIZE;
	int prow = (int)(cy) / TILE_SIZE;

	for (auto &e : _entities)
	{
		if (!e.active)
			continue;
		if (e.tile.x != pcol || e.tile.y != prow)
			continue;

		if (e.kind == EntityKind::Collectible)
		{
			e.active = false;
			_player.collected++;
		}
		else if (e.kind == EntityKind::Exit)
		{
			if (_player.collected >= _totalCollectibles)
			{
				_level++;
				_loadLevel();
				_lastTick = SDL_GetTicks();
				return;
			}
		}
	}
}

void Game::_update(float dt)
{
	if (_player.isFalling)
	{
		_player.jumpZ -= 300.0f * dt;
		if (_player.jumpZ <= -TILE_SIZE)
		{
			_player.isFalling = false;
			_player.jumpZ = 0.0f;
			_player.pos = _spawnPos;
		}
		return;
	}

	const Uint8 *keys = SDL_GetKeyboardState(nullptr);
	_player.vel = { 0, 0 };

	if (keys[SDL_SCANCODE_SPACE] && !_player.isJumping)
	{
		_player.isJumping = true;
		_player.jumpVelZ = JUMP_VELOCITY;
	}

	if (_player.isJumping)
	{
		_player.jumpVelZ -= JUMP_GRAVITY * dt;
		_player.jumpZ += _player.jumpVelZ * dt;
		if (_player.jumpZ <= 0.0f)
		{
			_player.jumpZ = 0.0f;
			_player.jumpVelZ = 0.0f;
			_player.isJumping = false;
		}
	}

	if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    _player.vel.y = -PLAYER_SPEED;
	if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  _player.vel.y =  PLAYER_SPEED;
	if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  _player.vel.x = -PLAYER_SPEED;
	if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) _player.vel.x =  PLAYER_SPEED;

	if (_player.vel.x != 0 && _player.vel.y != 0)
	{
		constexpr float inv = 0.70710678f;
		_player.vel.x *= inv;
		_player.vel.y *= inv;
	}

	float newX = _player.pos.x + _player.vel.x * dt;
	float newY = _player.pos.y + _player.vel.y * dt;

	if (_canMoveTo(newX, _player.pos.y))
		_player.pos.x = newX;
	if (_canMoveTo(_player.pos.x, newY))
		_player.pos.y = newY;

	_checkEntityCollisions();

	if (!_player.isJumping)
		_checkHoleFall();
}

void Game::_checkHoleFall()
{
	float cx = _player.pos.x + TILE_SIZE / 2.0f;
	float cy = _player.pos.y + TILE_SIZE / 2.0f;
	int col = (int)(cx) / TILE_SIZE;
	int row = (int)(cy) / TILE_SIZE;

	if (row >= 0 && row < _map.rows() && col >= 0 && col < _map.cols())
	{
		if (_map.at(row, col) == HOLE)
		{
			_player.isFalling = true;
			_player.jumpZ = 0.0f;
		}
	}
}

void Game::_handlePauseEvents()
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
			case SDLK_ESCAPE:
				_paused = false;
				_lastTick = SDL_GetTicks();
				break;
			case SDLK_UP:   case SDLK_w:
				_pauseSelection = (_pauseSelection + 1) % 2;
				break;
			case SDLK_DOWN: case SDLK_s:
				_pauseSelection = (_pauseSelection + 1) % 2;
				break;
			case SDLK_RETURN: case SDLK_SPACE:
				if (_pauseSelection == 0)
				{
					_paused = false;
					_lastTick = SDL_GetTicks();
				}
				else
					_running = false;
				break;
			default: break;
		}
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
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
		{
			_paused = true;
			_pauseSelection = 0;
			return;
		}
	}
}

int main()
{
	try
	{
		Game game;
		game.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
