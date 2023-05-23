#include "raylib.h"
#include "raymath.h"

#include <forward_list>
#include <tuple>

namespace Game {
    struct Pawn {
        // Position
        // Man I don't like floats at all
        float x = 0, y = 0;
        // Direction
        enum Direction {
            Up = 0, Right = 1, Down = 2, Left = 3
        };
        Direction direction = Direction::Right;
        // Instead of having a global game grid, we just record where the pawn was
        // Direction isn't stored, but could be computed easily
        std::forward_list<std::pair<int, int>> path;
        // Goes from 0 to 100
        int progress = 0;
        std::pair<int, int> crash_point = { -1,-1 };
    };

    struct State {
        int window_width = 800;
        int window_height = 800;

        int grid_spacing = 40;
        int grid_tolerance = 5;

        int num_pawns = 1;
        Pawn pawns[1];
        int pawnspeed = 4; // Keep this a multiple of two
    };

    std::pair<int, int> GetPawnWorldSpace(Pawn& pawn, int grid_spacing)
    {
        int pawn_x = pawn.x * grid_spacing;
        int pawn_y = pawn.y * grid_spacing;

        switch (pawn.direction) {
        case Pawn::Up:
            pawn_y -= (grid_spacing * pawn.progress) / 100;
            break;
        case Pawn::Down:
            pawn_y += (grid_spacing * pawn.progress) / 100;
            break;
        case Pawn::Right:
            pawn_x += (grid_spacing * pawn.progress) / 100;
            break;
        case Pawn::Left:
            pawn_x -= (grid_spacing * pawn.progress) / 100;
            break;
        default:
            break;
        }
        return { pawn_x, pawn_y };
    }

    // Probably my least favorite function dude
    void DrawPawnPath(Pawn& pawn, State& state)
    {
        if (pawn.path.empty()) return; // bail on empty

        auto pawn_world = GetPawnWorldSpace(pawn, state.grid_spacing);
        auto it = pawn.path.cbegin();
        // Render partial line trail too
        DrawLineEx({ (float)it->first * state.grid_spacing, (float)it->second * state.grid_spacing }, { (float)pawn_world.first, (float)pawn_world.second }, 5, ORANGE);

        while (it != pawn.path.cend()) {
            std::pair<int, int> point = *(it);

            auto next_it = std::next(it, 1);
            if (next_it == pawn.path.cend()) break;
            std::pair<int, int> next_point = *(next_it);

            DrawLineEx({ (float)state.grid_spacing * point.first,
                (float)state.grid_spacing * point.second },
                { (float)state.grid_spacing * next_point.first,
                  (float)state.grid_spacing * next_point.second }, 5, ORANGE);
            ++it;
        }

#ifdef _DEBUG
        for (auto point : pawn.path) {
            DrawCircle(point.first * state.grid_spacing, point.second * state.grid_spacing, 3, GREEN);
        }
#endif
    }

    void DrawPawn(Pawn& pawn, int grid_spacing)
    {
        auto loc = GetPawnWorldSpace(pawn, grid_spacing);
        DrawRectangle(loc.first - 10, loc.second - 10, 20, 20, BLUE);
#ifdef _DEBUG
        if (pawn.crash_point.first != -1) {
            DrawCircle(pawn.crash_point.first * grid_spacing, pawn.crash_point.second * grid_spacing, 5, RED);
        }
#endif

        return;
        float rotation_LUT[4] = { 180, 90, 0, -90 };
        Vector2 v1{ 0, 16 };
        Vector2 v2{ 10, 10 };
        Vector2 v3{ -10, 10 };
        float angle = DEG2RAD * -rotation_LUT[pawn.direction];
        v1 = Vector2Rotate(v1, angle);
        v2 = Vector2Rotate(v2, angle);
        v3 = Vector2Rotate(v3, angle);
        v1 = Vector2Add(v1, { pawn.x, pawn.y });
        v2 = Vector2Add(v2, { pawn.x, pawn.y });
        v3 = Vector2Add(v3, { pawn.x, pawn.y });
        DrawTriangle(v1, v2, v3, ORANGE);
    }

    void TranslatePawn(Pawn& pawn, Pawn::Direction dir)
    {
        switch (dir) {
        case Pawn::Up:
            pawn.y -= 1;
            break;
        case Pawn::Down:
            pawn.y += 1;
            break;
        case Pawn::Right:
            pawn.x += 1;
            break;
        case Pawn::Left:
            pawn.x -= 1;
            break;
        default:
            break;
        }
#ifdef _DEBUG
        TraceLog(LOG_DEBUG, "Translated!");
#endif
    }

    bool TestPointPathIntersect(int pawn_x, int pawn_y, std::forward_list<std::pair<int, int>>& path)
    {
        // Test collision with each segment
        // I don't like C++ iterators man
        // but forward_list has no size
        // Also skip first segment, thats still in progress/incomplete and we never collide with that
        for (auto& it = std::next(path.begin()); it != path.end(); it++) {
            std::pair<int, int>& point = *it;
            // or std::advance for C++17
            auto& next_it = std::next(it, 1);
            if (next_it == path.end()) break;
            std::pair<int, int>& next = *next_it;

            int dx = (point.first - next.first);
            int dy = (point.second - next.second);

            // Right now we're using >= and <=  to detect collision on corners.
            // Maybe we want some sick corner turns instead?
            if (dx == 0) {
                int x = point.first; // constant
                int min_y = std::min(point.second, next.second);
                int max_y = std::max(point.second, next.second);
                // Test vertical
                if (pawn_y <= max_y && pawn_y >= min_y && pawn_x == x)
                    return true;
            } else if (dy == 0) {
                // Test horizontal
                int y = point.second;
                int min_x = std::min(point.first, next.first);
                int max_x = std::max(point.first, next.first);
                if (pawn_x <= max_x && pawn_x >= min_x && pawn_y == y)
                    return true;
            }
        }
        return false;
    }

    void Tick(State& state)
    {
        Pawn& pawn = state.pawns[0];

        // We want to be able to move within 5 pixels of the corner.
        // Because precise inputs are impossible, we allow some tolerance
        int dist_to_corner = 100 - pawn.progress;
        if (abs(dist_to_corner) < state.grid_tolerance) {
            // Don't do a 180 turn and hit the wall
            Pawn::Direction prev_dir = pawn.direction;
            if (IsKeyDown(KEY_W) && pawn.direction != Game::Pawn::Down) {
                pawn.direction = Game::Pawn::Up;
            } else if (IsKeyDown(KEY_A) && pawn.direction != Game::Pawn::Right) {
                pawn.direction = Game::Pawn::Left;
            } else if (IsKeyDown(KEY_S) && pawn.direction != Game::Pawn::Up) {
                pawn.direction = Game::Pawn::Down;
            } else if (IsKeyDown(KEY_D) && pawn.direction != Game::Pawn::Left) {
                pawn.direction = Game::Pawn::Right;
            };

            // Did we turn around?
            if (prev_dir != pawn.direction) {
                // If we moved before we actually hit the corner, adjust
                if (dist_to_corner >= 0) { // value is positive
                    Game::TranslatePawn(pawn, prev_dir);
                    // Round progress to zero
                    pawn.progress = 0;
                }
                // Push current position to stack
                pawn.path.push_front({ pawn.x, pawn.y });
            }
        }

        // Move pawn
        pawn.progress += state.pawnspeed;
        if (pawn.progress >= 100) {
            pawn.progress -= 100;
            Game::TranslatePawn(pawn, pawn.direction);
        }

        bool already_crashed = (pawn.crash_point.first != -1);
        if (!already_crashed && TestPointPathIntersect(pawn.x, pawn.y, pawn.path)) {
            TraceLog(LOG_INFO, "COLLIDE!!!");
            pawn.crash_point = { pawn.x, pawn.y };
        }
    }

    void Draw(State& state)
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
#ifdef _DEBUG
        const char* debug_text = TextFormat("FPS %d/60 ", GetFPS());
        DrawText(debug_text, 190, 200, 20, LIGHTGRAY);
#endif
        // Draw grid lines
        for (int i = 0; i < state.window_width; i += state.grid_spacing) {
            DrawLine(i, 0, i, state.window_height, BLACK);
        }
        for (int j = 0; j < state.window_height; j += state.grid_spacing) {
            DrawLine(0, j, state.window_width, j, BLACK);
        }
        // Draw pawns
        for (int i = 0; i < state.num_pawns; i++) {
            DrawPawnPath(state.pawns[i], state);
            DrawPawn(state.pawns[i], state.grid_spacing);
        }
        EndDrawing();
    }
}

int main()
{
    Game::State state;
    // Create pawns
    for (int i = 0; i < state.num_pawns; i++) {
        state.pawns[i].x = 1 + i;
        state.pawns[i].y = 1 + i;
        state.pawns[i].path.push_front({ 1 + i, 1 + i });
    }
    SetTraceLogLevel(LOG_DEBUG);
    InitWindow(state.window_width, state.window_height, "Bikes");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        Game::Tick(state);
        Game::Draw(state);
    }

    CloseWindow();
    return 0;
}