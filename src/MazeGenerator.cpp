#include "so_long.h"

static void carveMaze(std::vector<std::string> &grid, int rows, int cols,
                      std::mt19937 &rng)
{
	constexpr int dx[] = { 0, 2, 0, -2 };
	constexpr int dy[] = { 2, 0, -2,  0 };

	auto inBounds = [&](int r, int c) {
		return r > 0 && r < rows - 1 && c > 0 && c < cols - 1;
	};

	std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
	std::stack<std::pair<int,int>> stk;

	int startR = 1, startC = 1;
	visited[startR][startC] = true;
	grid[startR][startC] = FLOOR;
	stk.push({startR, startC});

	while (!stk.empty())
	{
		auto [cr, cc] = stk.top();

		int order[] = {0, 1, 2, 3};
		for (int i = 3; i > 0; --i)
		{
			std::uniform_int_distribution<int> dist(0, i);
			std::swap(order[i], order[dist(rng)]);
		}

		bool pushed = false;
		for (int i = 0; i < 4; i++)
		{
			int nr = cr + dx[order[i]];
			int nc = cc + dy[order[i]];
			if (inBounds(nr, nc) && !visited[nr][nc])
			{
				visited[nr][nc] = true;
				grid[nr][nc] = FLOOR;
				grid[cr + dx[order[i]] / 2][cc + dy[order[i]] / 2] = FLOOR;
				stk.push({nr, nc});
				pushed = true;
				break;
			}
		}
		if (!pushed)
			stk.pop();
	}
}

static std::vector<std::vector<int>>
bfsDistances(const std::vector<std::string> &grid, int rows, int cols,
             int sr, int sc)
{
	std::vector<std::vector<int>> dist(rows, std::vector<int>(cols, -1));
	std::queue<std::pair<int,int>> q;
	dist[sr][sc] = 0;
	q.push({sr, sc});

	constexpr int dr[] = {0, 0, 1, -1};
	constexpr int dc[] = {1, -1, 0, 0};

	while (!q.empty())
	{
		auto [r, c] = q.front();
		q.pop();
		for (int d = 0; d < 4; d++)
		{
			int nr = r + dr[d];
			int nc = c + dc[d];
			if (nr <= 0 || nr >= rows - 1 || nc <= 0 || nc >= cols - 1)
				continue;
			if (dist[nr][nc] != -1)
				continue;
			if (grid[nr][nc] == WALL)
				continue;
			dist[nr][nc] = dist[r][c] + 1;
			q.push({nr, nc});
		}
	}
	return dist;
}

static std::vector<std::pair<int,int>>
reachableFloors(const std::vector<std::string> &grid, int rows, int cols,
                int sr, int sc, int exitR, int exitC)
{
	std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
	std::stack<std::pair<int,int>> stk;
	std::vector<std::pair<int,int>> result;

	visited[sr][sc] = true;
	stk.push({sr, sc});

	while (!stk.empty())
	{
		auto [r, c] = stk.top();
		stk.pop();

		if (grid[r][c] == FLOOR)
			result.push_back({r, c});

		constexpr int dr[] = {0, 0, 1, -1};
		constexpr int dc[] = {1, -1, 0, 0};
		for (int d = 0; d < 4; d++)
		{
			int nr = r + dr[d];
			int nc = c + dc[d];
			if (nr <= 0 || nr >= rows - 1 || nc <= 0 || nc >= cols - 1)
				continue;
			if (visited[nr][nc])
				continue;
			if (grid[nr][nc] == WALL || grid[nr][nc] == HOLE)
				continue;
			if (nr == exitR && nc == exitC)
				continue;
			visited[nr][nc] = true;
			stk.push({nr, nc});
		}
	}
	return result;
}

static void placeHoles(std::vector<std::string> &grid, int rows, int cols,
                       int count, int playerR, int playerC,
                       int exitR, int exitC, std::mt19937 &rng)
{
	std::vector<std::pair<int,int>> candidates;
	for (int r = 1; r < rows - 1; r++)
		for (int c = 1; c < cols - 1; c++)
			if (grid[r][c] == FLOOR)
				candidates.push_back({r, c});

	for (int i = (int)candidates.size() - 1; i > 0; --i)
	{
		std::uniform_int_distribution<int> dist(0, i);
		std::swap(candidates[i], candidates[dist(rng)]);
	}

	auto isAdjacentHole = [&](int r, int c) -> bool {
		constexpr int dr[] = {0, 0, 1, -1};
		constexpr int dc[] = {1, -1, 0, 0};
		for (int d = 0; d < 4; d++)
		{
			int nr = r + dr[d];
			int nc = c + dc[d];
			if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && grid[nr][nc] == HOLE)
				return true;
		}
		return false;
	};

	int placed = 0;
	for (auto &[r, c] : candidates)
	{
		if (placed >= count)
			break;
		if (r == playerR && c == playerC)
			continue;
		if (r == exitR && c == exitC)
			continue;
		if (std::abs(r - playerR) + std::abs(c - playerC) <= 2)
			continue;
		if (std::abs(r - exitR) + std::abs(c - exitC) <= 1)
			continue;
		if (isAdjacentHole(r, c))
			continue;
		grid[r][c] = HOLE;
		placed++;
	}
}

static void placeCollectibles(std::vector<std::string> &grid, int rows, int cols,
                              int count, int exitR, int exitC,
                              std::mt19937 &rng)
{
	auto floors = reachableFloors(grid, rows, cols, 1, 1, exitR, exitC);

	int minSpacing = std::max(2, std::min(rows, cols) / 5);

	for (int i = (int)floors.size() - 1; i > 0; --i)
	{
		std::uniform_int_distribution<int> dist(0, i);
		std::swap(floors[i], floors[dist(rng)]);
	}

	std::vector<std::pair<int,int>> placed;

	auto tooClose = [&](int r, int c) -> bool {
		int de = std::abs(r - exitR) + std::abs(c - exitC);
		if (de < minSpacing)
			return true;
		for (auto &[pr, pc] : placed)
		{
			int dp = std::abs(r - pr) + std::abs(c - pc);
			if (dp < minSpacing)
				return true;
		}
		return false;
	};

	for (auto &[r, c] : floors)
	{
		if ((int)placed.size() >= count)
			break;
		if (tooClose(r, c))
			continue;
		placed.push_back({r, c});
		grid[r][c] = COLLECT;
	}

	if ((int)placed.size() < count)
	{
		for (auto &[r, c] : floors)
		{
			if ((int)placed.size() >= count)
				break;
			if (grid[r][c] != FLOOR)
				continue;
			placed.push_back({r, c});
			grid[r][c] = COLLECT;
		}
	}
}

static void breakLongWalls(std::vector<std::string> &grid, int rows, int cols,
                           int maxLen)
{
	for (int r = 1; r < rows - 1; r++)
	{
		int count = 0;
		for (int c = 1; c < cols - 1; c++)
		{
			if (grid[r][c] == WALL)
			{
				count++;
				if (count > maxLen)
				{
					grid[r][c] = FLOOR;
					count = 0;
				}
			}
			else
				count = 0;
		}
	}

	for (int c = 1; c < cols - 1; c++)
	{
		int count = 0;
		for (int r = 1; r < rows - 1; r++)
		{
			if (grid[r][c] == WALL)
			{
				count++;
				if (count > maxLen)
				{
					grid[r][c] = FLOOR;
					count = 0;
				}
			}
			else
				count = 0;
		}
	}
}

std::vector<std::string> generateMaze(int level)
{
	int innerW = 5 + level;
	int innerH = 5 + level;
	if (innerW % 2 == 0) innerW++;
	if (innerH % 2 == 0) innerH++;

	int cols = innerW + 2;
	int rows = innerH + 2;
	if (cols % 2 == 0) cols++;
	if (rows % 2 == 0) rows++;

	std::vector<std::string> grid(rows, std::string(cols, WALL));

	std::random_device rd;
	std::mt19937 rng(rd());

	carveMaze(grid, rows, cols, rng);
	breakLongWalls(grid, rows, cols, 4);

	grid[1][1] = PLAYER;

	auto dist = bfsDistances(grid, rows, cols, 1, 1);
	int exitR = 1, exitC = 1;
	int bestDist = -1;
	for (int r = 1; r < rows - 1; r++)
		for (int c = 1; c < cols - 1; c++)
			if (grid[r][c] == FLOOR && dist[r][c] > bestDist)
			{
				bestDist = dist[r][c];
				exitR = r;
				exitC = c;
			}
	grid[exitR][exitC] = EXIT;

	int nCollect = 1;
	if (level >= 4)
		nCollect = 2 + (level - 4) / 10;
	placeCollectibles(grid, rows, cols, nCollect, exitR, exitC, rng);

	int nHoles = 0;
	if (level >= 2)
		nHoles = 1 + (level - 2) / 2;
	placeHoles(grid, rows, cols, nHoles, 1, 1, exitR, exitC, rng);

	return grid;
}
