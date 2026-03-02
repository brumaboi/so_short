#include "../inc/so_long.h"

void Map::_checkFormat(const std::string &path)
{
	auto dot = path.rfind('.');
	if (dot == std::string::npos || path.substr(dot) != ".ber")
		throw std::runtime_error("Invalid file format (expected .ber)");
}

void Map::_readFile(const std::string &path)
{
	std::ifstream file(path);
	if (!file.is_open())
		throw std::runtime_error("Failed to open file: " + path);

	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		_grid.push_back(line);
	}
	_rows = static_cast<int>(_grid.size());
	if (_rows == 0)
		throw std::runtime_error("Map file is empty");
}

void Map::_trimRows()
{
	size_t maxLen = 0;
	for (auto &row : _grid)
	{
		while (!row.empty() && (row.back() == '\n' || row.back() == '\r'))
			row.pop_back();
		maxLen = std::max(maxLen, row.length());
	}
	_cols = static_cast<int>(maxLen);
}

void Map::_checkEmptyLines()
{
	if (_grid[0].empty() || _grid[_rows - 1].empty())
		throw std::runtime_error("Map has empty border lines");
	for (int i = 0; i < _rows - 1; i++)
		if (_grid[i].empty() && _grid[i + 1].empty())
			throw std::runtime_error("Map has consecutive empty lines");
}

void Map::load(const std::string &path)
{
	_checkFormat(path);
	_readFile(path);
	_trimRows();
	_checkEmptyLines();
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
			if (c != FLOOR && c != WALL && c != PLAYER && c != EXIT && c != COLLECT)
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
			if (c == EXIT)   { _exits++; _exitPos = {j, i}; }
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

void Map::set(int row, int col, char c)
{
	_grid[row][col] = c;

}

int  Map::rows() const
{
	return _rows;
}

int  Map::cols() const
{
	return _cols;
}

int  Map::collectibles() const
{
	return _collectibles;
}

Vec2 Map::playerPos() const
{
	return _playerPos;
}

Vec2 Map::exitPos() const
{
	return _exitPos;
}

void Map::removeCollectible(int row, int col)
{
	_grid[row][col] = FLOOR;
	_collectibles--;
}

void Map::movePlayer(int new_row, int new_col)
{
	_grid[_playerPos.y][_playerPos.x] = FLOOR;
	_playerPos = {new_col, new_row};
	_grid[new_row][new_col] = PLAYER;
}
