#include "so_long.h"

void Map::loadFromGrid(std::vector<std::string> grid)
{
	_grid = std::move(grid);
	_rows = static_cast<int>(_grid.size());
	if (_rows == 0)
		throw std::runtime_error("Generated maze is empty");
	_cols = static_cast<int>(_grid[0].size());
}

void Map::_checkWalls()
{
	for (int i = 0; i < _rows; i++)
	{
		size_t len = _grid[i].length();
		if (len == 0 || _grid[i].front() != WALL || _grid[i].back() != WALL)
			throw std::runtime_error("Map is not surrounded by walls (row " + std::to_string(i) + ")");
	}
	for (int j = 0; j < _cols; j++)
		if (_grid[0][j] != WALL || _grid[_rows - 1][j] != WALL)
			throw std::runtime_error("Map is not surrounded by walls (col " + std::to_string(j) + ")");
}

void Map::_checkValidChars()
{
	for (int i = 0; i < _rows; i++)
		for (int j = 0; j < _cols; j++)
		{
			char c = _grid[i][j];
			if (c != FLOOR && c != WALL && c != PLAYER && c != EXIT && c != COLLECT && c != HOLE)
				throw std::runtime_error(std::string("Invalid character '") + c + "' at (" + std::to_string(i) + "," + std::to_string(j) + ")");
		}
}

void Map::_countElements()
{
	_collectibles = 0;
	_exits = 0;
	_players = 0;
	for (int i = 0; i < _rows; i++)
		for (int j = 0; j < _cols; j++)
		{
			char c = _grid[i][j];
			if (c == PLAYER) { _players++; _playerPos = {j, i}; }
			if (c == COLLECT) _collectibles++;
			if (c == EXIT)   _exits++;
		}
	if (_players != 1)
		throw std::runtime_error("Map must have exactly 1 player (found " + std::to_string(_players) + ")");
	if (_exits != 1)
		throw std::runtime_error("Map must have exactly 1 exit (found " + std::to_string(_exits) + ")");
	if (_collectibles < 1)
		throw std::runtime_error("Map must have at least 1 collectible");
}

void Map::_flood(int col, int row,
	std::vector<std::vector<bool>> &visited,
	bool passExit, int &collectFound, bool &exitFound)
{
	if (col < 0 || row < 0 || col >= _cols || row >= _rows)
		return;
	if (visited[row][col])
		return ;
	char c = _grid[row][col];
	if (c == WALL)
		return ;
	if (c == EXIT && !passExit)
	{
		exitFound = true;
		return ;
	}
	visited[row][col] = true;
	if (c == COLLECT)
		collectFound++;
	if (c == EXIT)
		exitFound = true;
	_flood(col + 1, row, visited, passExit, collectFound, exitFound);
	_flood(col - 1, row, visited, passExit, collectFound, exitFound);
	_flood(col, row + 1, visited, passExit, collectFound, exitFound);
	_flood(col, row - 1, visited, passExit, collectFound, exitFound);
}

void Map::_checkAccess()
{
	std::vector<std::vector<bool>> visited(_rows, std::vector<bool>(_cols, false));
	int collectFound = 0;
	bool exitFound = false;

	_flood(_playerPos.x, _playerPos.y, visited, false, collectFound, exitFound);
	if (collectFound != _collectibles)
		throw std::runtime_error("Not all collectibles are reachable (" + std::to_string(collectFound) + "/" + std::to_string(_collectibles) + ")");

	for (auto &row : visited)
		std::fill(row.begin(), row.end(), false);
	collectFound = 0;
	exitFound = false;
	_flood(_playerPos.x, _playerPos.y, visited, true, collectFound, exitFound);
	if (!exitFound)
		throw std::runtime_error("Exit is not reachable");
}

void Map::validate()
{
	_checkWalls();
	_checkValidChars();
	_countElements();
	_checkAccess();
}


char Map::at(int row, int col) const
{
	return _grid[row][col];
}

int  Map::rows() const
{
	return _rows;
}

int  Map::cols() const
{
	return _cols;
}

bool Map::isWall(int row, int col) const
{
	if (row < 0 || row >= _rows || col < 0 || col >= _cols)
		return true;
	return _grid[row][col] == WALL;
}

void Map::extractEntities(PlayerObj &player, std::vector<Entity> &entities)
{
	entities.clear();
	for (int r = 0; r < _rows; r++)
	{
		for (int c = 0; c < _cols; c++)
		{
			char ch = _grid[r][c];
			if (ch == PLAYER)
			{
				player.pos = { (float)(c * TILE_SIZE), (float)(r * TILE_SIZE) };
				player.vel = { 0, 0 };
				player.collected = 0;
				player.jumpZ = 0.0f;
				player.jumpVelZ = 0.0f;
				player.isJumping = false;
				player.isFalling = false;
				_grid[r][c] = FLOOR;
			}
			else if (ch == COLLECT)
			{
				entities.push_back({ EntityKind::Collectible, {c, r}, true });
				_grid[r][c] = FLOOR;
			}
			else if (ch == EXIT)
			{
				entities.push_back({ EntityKind::Exit, {c, r}, true });
				_grid[r][c] = FLOOR;
			}
		}
	}
}
