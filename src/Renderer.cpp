#include "../inc/so_long.h"

Renderer::~Renderer()
{
	auto destroy = [](SDL_Texture *&t) { if (t) { SDL_DestroyTexture(t); t = nullptr; } };
	destroy(_texWall);
	destroy(_texFloor);
	destroy(_texPlayer);
	destroy(_texCollect);
	destroy(_texExit);
	destroy(_texExitOpen);
	if (_mapTex)   { SDL_DestroyTexture(_mapTex);   _mapTex = nullptr; }
	if (_renderer) { SDL_DestroyRenderer(_renderer); _renderer = nullptr; }
	if (_window)   { SDL_DestroyWindow(_window);     _window = nullptr; }
	IMG_Quit();
	SDL_Quit();
}

void Renderer::init(const Map &map)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		std::cerr << "Warning: IMG_Init PNG failed: " << IMG_GetError() << " (fallback colors will be used)" << std::endl;

	_window = SDL_CreateWindow("so_long", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (!_window)
		throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());

	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!_renderer)
		throw std::runtime_error(std::string("SDL_CreateRenderer: ") + SDL_GetError());

	_loadTextures();
	Vec2 pp = map.playerPos();
	Vec2f startPx = { (float)(pp.x * TILE_SIZE), (float)(pp.y * TILE_SIZE) };
	updateCamera(map, startPx);
}

SDL_Texture *Renderer::_makeColor(Uint8 r, Uint8 g, Uint8 b)
{
	SDL_Surface *s = SDL_CreateRGBSurface(0, TILE_SIZE, TILE_SIZE, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!s)
		throw std::runtime_error("Failed to create fallback surface");
	SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, r, g, b));
	SDL_Texture *t = SDL_CreateTextureFromSurface(_renderer, s);
	SDL_FreeSurface(s);
	if (!t)
		throw std::runtime_error("Failed to create fallback texture");
	return t;
}

SDL_Texture *Renderer::_loadOrFallback(const std::string &path, Uint8 r, Uint8 g, Uint8 b)
{
	SDL_Surface *surface = IMG_Load(path.c_str());
	if (!surface)
	{
		std::cerr << "Note: " << path << " not found — using fallback color" << std::endl;
		return _makeColor(r, g, b);
	}
	SDL_Texture *tex = SDL_CreateTextureFromSurface(_renderer, surface);
	SDL_FreeSurface(surface);
	if (!tex)
		throw std::runtime_error("Texture creation failed for " + path);
	return tex;
}

void Renderer::_loadTextures()
{
	_texWall     = _loadOrFallback("textures/wall.png",        100, 100, 100);
	_texFloor    = _loadOrFallback("textures/floor.png",        50,  50,  50);
	_texPlayer   = _loadOrFallback("textures/player.png",        0, 150, 255);
	_texCollect  = _loadOrFallback("textures/collectible.png", 255, 215,   0);
	_texExit     = _loadOrFallback("textures/exit.png",        180,   0,   0);
	_texExitOpen = _loadOrFallback("textures/open_exit.png",     0, 200,   0);
}

void Renderer::updateCamera(const Map &map, Vec2f playerPx)
{
	int winW, winH;
	SDL_GetWindowSize(_window, &winW, &winH);

	int mapPxW = map.cols() * TILE_SIZE;
	int mapPxH = map.rows() * TILE_SIZE;

	if (mapPxW <= winW)
		_camX = -(winW - mapPxW) / 2;
	else
	{
		_camX = (int)(playerPx.x + TILE_SIZE / 2.0f - winW / 2.0f);
		_camX = std::clamp(_camX, 0, mapPxW - winW);
	}

	if (mapPxH <= winH)
		_camY = -(winH - mapPxH) / 2;
	else
	{
		_camY = (int)(playerPx.y + TILE_SIZE / 2.0f - winH / 2.0f);
		_camY = std::clamp(_camY, 0, mapPxH - winH);
	}
}

void Renderer::_drawTile(SDL_Texture *tex, int col, int row)
{
	SDL_Rect dst = {
		col * TILE_SIZE - _camX,
		row * TILE_SIZE - _camY,
		TILE_SIZE, TILE_SIZE
	};
	SDL_RenderCopy(_renderer, tex, nullptr, &dst);
}

void Renderer::_drawTileAt(SDL_Texture *tex, float px, float py)
{
	SDL_Rect dst = {
		(int)(px) - _camX,
		(int)(py) - _camY,
		TILE_SIZE, TILE_SIZE
	};
	SDL_RenderCopy(_renderer, tex, nullptr, &dst);
}

void Renderer::_rebuildMapTex(const Map &map)
{
	_mapTexW = map.cols() * TILE_SIZE;
	_mapTexH = map.rows() * TILE_SIZE;

	if (_mapTex)
		SDL_DestroyTexture(_mapTex);
	_mapTex = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET, _mapTexW, _mapTexH);
	if (!_mapTex)
		throw std::runtime_error(std::string("Map texture: ") + SDL_GetError());

	SDL_SetRenderTarget(_renderer, _mapTex);
	SDL_RenderClear(_renderer);

	int savedCamX = _camX;
	int savedCamY = _camY;
	_camX = 0;
	_camY = 0;

	for (int r = 0; r < map.rows(); r++)
	{
		for (int c = 0; c < map.cols(); c++)
		{
			_drawTile(_texFloor, c, r);
			switch (map.at(r, c))
			{
				case WALL:    _drawTile(_texWall, c, r);    break;
				case COLLECT: _drawTile(_texCollect, c, r); break;
				case EXIT:
					_drawTile(map.collectibles() == 0 ? _texExitOpen : _texExit, c, r);
					break;
				default: break;
			}
		}
	}

	SDL_SetRenderTarget(_renderer, nullptr);
	_camX = savedCamX;
	_camY = savedCamY;
	_mapDirty = false;
}

void Renderer::markMapDirty()
{
	_mapDirty = true;
}

void Renderer::render(const Map &map, Vec2f playerPx)
{
	if (_mapDirty)
		_rebuildMapTex(map);

	int winW, winH;
	SDL_GetWindowSize(_window, &winW, &winH);

	SDL_Rect src = { std::max(0, _camX), std::max(0, _camY), winW, winH };
	SDL_Rect dst = { 0, 0, winW, winH };
	if (_camX < 0)
	{
		src.x = 0;
		src.w = _mapTexW;
		dst.x = -_camX;
		dst.w = _mapTexW;
	}
	if (_camY < 0)
	{
		src.y = 0;
		src.h = _mapTexH;
		dst.y = -_camY;
		dst.h = _mapTexH;
	}

	SDL_RenderClear(_renderer);
	SDL_RenderCopy(_renderer, _mapTex, &src, &dst);
	_drawTileAt(_texPlayer, playerPx.x, playerPx.y);
	SDL_RenderPresent(_renderer);
}

SDL_Window *Renderer::window() const
{
	return _window;
}
