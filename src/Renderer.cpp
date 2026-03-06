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
	if (_pauseTex) { SDL_DestroyTexture(_pauseTex); _pauseTex = nullptr; }
	if (_renderer) { SDL_DestroyRenderer(_renderer); _renderer = nullptr; }
	if (_window)   { SDL_DestroyWindow(_window);     _window = nullptr; }
	IMG_Quit();
	SDL_Quit();
}

void Renderer::init(const Map &map, Vec2f playerPx)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		std::cerr << "Warning: IMG_Init PNG failed: " << IMG_GetError()
		          << " (fallback colors will be used)" << std::endl;

	_window = SDL_CreateWindow("so_short",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (!_window)
		throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());

	_renderer = SDL_CreateRenderer(_window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!_renderer)
		throw std::runtime_error(std::string("SDL_CreateRenderer: ") + SDL_GetError());

	_loadTextures();
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	_buildMapTex(map);
	updateCamera(map, playerPx);
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

void Renderer::_buildMapTex(const Map &map)
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
			if (map.at(r, c) == WALL)
				_drawTile(_texWall, c, r);
		}
	}

	SDL_SetRenderTarget(_renderer, nullptr);
	_camX = savedCamX;
	_camY = savedCamY;
}

void Renderer::rebuildMapTex(const Map &map)
{
	_buildMapTex(map);
}

void Renderer::render(const Map &, const PlayerObj &player,
                      const std::vector<Entity> &entities, int totalCollectibles)
{
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

	SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
	SDL_RenderClear(_renderer);

	SDL_RenderCopy(_renderer, _mapTex, &src, &dst);

	bool allCollected = (player.collected >= totalCollectibles);
	for (auto &e : entities)
	{
		if (!e.active)
			continue;
		if (e.kind == EntityKind::Collectible)
			_drawTile(_texCollect, e.tile.x, e.tile.y);
		else if (e.kind == EntityKind::Exit)
			_drawTile(allCollected ? _texExitOpen : _texExit, e.tile.x, e.tile.y);
	}

	_drawTileAt(_texPlayer, player.pos.x, player.pos.y);
}

void Renderer::_drawFilledRect(int x, int y, int w, int h,
                               Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_SetRenderDrawColor(_renderer, r, g, b, a);
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderFillRect(_renderer, &rect);
}

void Renderer::_drawRect(int x, int y, int w, int h,
                         Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_SetRenderDrawColor(_renderer, r, g, b, a);
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderDrawRect(_renderer, &rect);
}

void Renderer::_buildPauseOverlay(int selectedItem, int level, int winW, int winH)
{
	if (_pauseTex)
		SDL_DestroyTexture(_pauseTex);
	_pauseTex = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET, winW, winH);
	SDL_SetTextureBlendMode(_pauseTex, SDL_BLENDMODE_BLEND);

	SDL_SetRenderTarget(_renderer, _pauseTex);
	SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0);
	SDL_RenderClear(_renderer);

	_drawFilledRect(0, 0, winW, winH, 0, 0, 0, 160);

	int panelW = 300;
	int panelH = 240;
	int px = (winW - panelW) / 2;
	int py = (winH - panelH) / 2;
	_drawFilledRect(px, py, panelW, panelH, 30, 30, 30, 220);
	_drawRect(px, py, panelW, panelH, 180, 180, 180, 255);

	_drawFilledRect(px, py, panelW, 40, 60, 60, 60, 255);
	for (int i = 0; i < 6; i++)
		_drawFilledRect(px + panelW / 2 - 30 + i * 10, py + 14, 8, 12,
		                200, 200, 200, 255);
	int lvY = py + 50;
	int lvX = px + panelW / 2;

	std::string numStr = std::to_string(level);
	int digitW = 14;
	int totalW = (int)numStr.size() * (digitW + 4) + 20;
	int startX = lvX - totalW / 2;

	_drawFilledRect(startX, lvY, 3, 14, 180, 180, 180, 255);
	_drawFilledRect(startX, lvY + 11, 10, 3, 180, 180, 180, 255);
	for (int d = 0; d < 7; d++)
	{
		_drawFilledRect(startX + 14 + d, lvY + d * 2, 3, 2, 180, 180, 180, 255);
		_drawFilledRect(startX + 26 - d, lvY + d * 2, 3, 2, 180, 180, 180, 255);
	}

	int dX = startX + 36;

	auto drawSeg = [&](int sx, int sy, bool segs[7]) {
		int sw = 10, sh = 2, vw = 2, vh = 6;
		if (segs[0]) _drawFilledRect(sx + 1, sy, sw, sh, 0, 200, 255, 255);
		if (segs[1]) _drawFilledRect(sx, sy + 1, vw, vh, 0, 200, 255, 255);
		if (segs[2]) _drawFilledRect(sx + sw, sy + 1, vw, vh, 0, 200, 255, 255);
		if (segs[3]) _drawFilledRect(sx + 1, sy + vh + 1, sw, sh, 0, 200, 255, 255);
		if (segs[4]) _drawFilledRect(sx, sy + vh + 2, vw, vh, 0, 200, 255, 255);
		if (segs[5]) _drawFilledRect(sx + sw, sy + vh + 2, vw, vh, 0, 200, 255, 255);
		if (segs[6]) _drawFilledRect(sx + 1, sy + 2*vh + 2, sw, sh, 0, 200, 255, 255);
	};

	bool segTable[10][7] = {
		{ true,  true,  true,  false, true,  true,  true  },
		{ false, false, true,  false, false, true,  false },
		{ true,  false, true,  true,  true,  false, true  },
		{ true,  false, true,  true,  false, true,  true  },
		{ false, true,  true,  true,  false, true,  false },
		{ true,  true,  false, true,  false, true,  true  },
		{ true,  true,  false, true,  true,  true,  true  },
		{ true,  false, true,  false, false, true,  false },
		{ true,  true,  true,  true,  true,  true,  true  },
		{ true,  true,  true,  true,  false, true,  true  },
	};

	for (char ch : numStr)
	{
		int d = ch - '0';
		if (d >= 0 && d <= 9)
			drawSeg(dX, lvY, segTable[d]);
		dX += digitW + 4;
	}

	int btnW = 220;
	int btnH = 44;
	int btnX = px + (panelW - btnW) / 2;

	for (int i = 0; i < 2; i++)
	{
		int btnY = py + 100 + i * 64;
		bool sel = (i == selectedItem);

		if (sel)
		{
			_drawFilledRect(btnX, btnY, btnW, btnH, 0, 120, 215, 255);
			_drawRect(btnX, btnY, btnW, btnH, 255, 255, 255, 255);
		}
		else
		{
			_drawFilledRect(btnX, btnY, btnW, btnH, 60, 60, 60, 200);
			_drawRect(btnX, btnY, btnW, btnH, 120, 120, 120, 200);
		}

		if (i == 0)
		{
			int cx = btnX + btnW / 2;
			int cy = btnY + btnH / 2;
			Uint8 ic = sel ? 255 : 160;
			for (int row = -8; row <= 8; row++)
			{
				int half = 8 - std::abs(row);
				_drawFilledRect(cx - 4, cy + row, half + 4, 1, ic, ic, ic, 255);
			}
		}
		else
		{
			int cx = btnX + btnW / 2;
			int cy = btnY + btnH / 2;
			Uint8 ic = sel ? 255 : 160;
			for (int d = -7; d <= 7; d++)
			{
				_drawFilledRect(cx + d - 1, cy + d - 1, 3, 3, ic, ic, ic, 255);
				_drawFilledRect(cx + d - 1, cy - d - 1, 3, 3, ic, ic, ic, 255);
			}
		}
	}

	SDL_SetRenderTarget(_renderer, nullptr);
}

void Renderer::renderPauseOverlay(int selectedItem, int level)
{
	int winW, winH;
	SDL_GetWindowSize(_window, &winW, &winH);

	if (!_pauseTex || _cachedPauseSel != selectedItem || _cachedPauseLvl != level)
	{
		_buildPauseOverlay(selectedItem, level, winW, winH);
		_cachedPauseSel = selectedItem;
		_cachedPauseLvl = level;
	}

	SDL_RenderCopy(_renderer, _pauseTex, nullptr, nullptr);
}

void Renderer::present()
{
	SDL_RenderPresent(_renderer);
}
