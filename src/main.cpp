#include "raylib.h"
#include "raymath.h"

#include <map>
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
        // store direction when passing over
        // list of tuple of (x,y,direction)
        std::map<std::pair<int, int>, Direction> path;
    };

    struct State {
        int window_width = 800;
        int window_height = 800;

        int grid_spacing = 40;
        int grid_tolerance = 5;

        int num_pawns = 1;
        Pawn pawns[1];
        int pawnspeed = 3;
    };

    void DrawPawnPath(Pawn &pawn, State &state) {
        auto shift_coord = [](int &x, int &y, Pawn::Direction dir, bool flip = false) {
            if (dir == Pawn::Right)
                x -= flip ? -1 : 1;
            else if (dir == Pawn::Left)
                x += flip ? -1 : 1;
            else if (dir == Pawn::Up)
                y += flip ? -1 : 1;
            else if (dir == Pawn::Down)
                y -= flip ? -1 : 1;
        };

        auto shift_half = [](float x, float y, Pawn::Direction dir, bool flip = false) -> Vector2 {
            // Flip dir
            if (flip) dir = (Pawn::Direction) ((dir + 2) % 4);
            if (dir == Pawn::Right)
                x -= 0.5f;
            else if (dir == Pawn::Left)
                x += 0.5f;
            else if (dir == Pawn::Up)
                y += 0.5f;
            else if (dir == Pawn::Down)
                y -= 0.5f;
            return {x, y};
        };

        //Draw pawn paths too
        for (auto &point: pawn.path) {
            auto &loc = point.first;
            Pawn::Direction dir = point.second;
            int x = loc.first;
            int y = loc.second;

            // step back to get control point
            int endx = loc.first;
            int endy = loc.second;
            shift_coord(endx, endy, dir);

            // copy control
            int controlx = endx;
            int controly = endy;
            // Attempt to find end point if it exists
            Pawn::Direction other_dir = dir;
            if (pawn.path.find({endx, endy}) != pawn.path.end()) {
                other_dir = pawn.path.find({endx, endy})->second;
                shift_coord(endx, endy, other_dir);
            };

            // Scale grid coords to world coords
            Vector2 start = Vector2Scale(shift_half(x, y, dir), state.grid_spacing);
            Vector2 end = Vector2Scale(shift_half(endx, endy, other_dir, true), state.grid_spacing);
            Vector2 control = Vector2Scale({(float) controlx, (float) controly}, state.grid_spacing);
            DrawLineBezierQuad(start, end, control, 5.f, BLUE);
        }
        
        // Render partial line trail too
        {
            // Get source point
            int lastx = lrint(pawn.x / state.grid_spacing);
            int lasty = lrint(pawn.y / state.grid_spacing);
            Vector2 lastpoint = {(float)lastx, (float)lasty};

            int controlx = lastx;
            int controly = lasty;

            auto prev_point = pawn.path.find({lastx, lasty});
            if (prev_point != pawn.path.end()) {
                shift_coord(lastx, lasty, prev_point->second);
                lastpoint = {(float) lastx, (float) lasty};
                lastpoint = shift_half(lastpoint.x, lastpoint.y, prev_point->second, true);
            }

            lastpoint = Vector2Scale(lastpoint, state.grid_spacing);
            Vector2 control = Vector2Scale({(float) controlx, (float) controly}, state.grid_spacing);
            DrawLineBezierQuad(lastpoint,
                               {(float) pawn.x, (float) pawn.y}, // Render to this point
                               control,
                               5.f, BLUE);
        }
    }

    void DrawPawn(Pawn &pawn) {

        float rotation_LUT[4] = {180, 90, 0, -90};
        // Draws a square with a triangle pointing in direction
        // Should be moved to a nicer function
        DrawRectangle(pawn.x - 10, pawn.y - 10, 20, 20, BLUE);
        Vector2 v1{0, 16};
        Vector2 v2{10, 10};
        Vector2 v3{-10, 10};
        float angle = DEG2RAD * -rotation_LUT[pawn.direction];
        v1 = Vector2Rotate(v1, angle);
        v2 = Vector2Rotate(v2, angle);
        v3 = Vector2Rotate(v3, angle);
        v1 = Vector2Add(v1, {pawn.x, pawn.y});
        v2 = Vector2Add(v2, {pawn.x, pawn.y});
        v3 = Vector2Add(v3, {pawn.x, pawn.y});
        DrawTriangle(v1, v2, v3, ORANGE);
    }

    void Tick(State &state) {
        Pawn &pawn = state.pawns[0];
        auto align = [](float val, int grid) -> float {
            return (int) (val / grid + 0.5) * grid;
        };

        auto CanTurn = [](Pawn &p, int grid_spacing, int grid_tolerance) -> bool {
            int actual_x = (int) ((p.x) + 0.5f);
            int actual_y = (int) ((p.y) + 0.5f);
            actual_x = abs(actual_x) % grid_spacing;
            actual_y = abs(actual_y) % grid_spacing;
            // try negating actual_x based on direction
            return (actual_x) < grid_tolerance && (actual_y) < grid_tolerance;
        };

        if (CanTurn(pawn, state.grid_spacing, state.grid_tolerance)) {
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
            // If we moved, correct grid snap
            if (prev_dir != pawn.direction) {
                pawn.x = align(pawn.x, state.grid_spacing);
                pawn.y = align(pawn.y, state.grid_spacing);
            }
        }

        // Move pawn in direction
        if (pawn.direction == Game::Pawn::Down) {
            pawn.y += state.pawnspeed;
        }
        if (pawn.direction == Game::Pawn::Right) {
            pawn.x += state.pawnspeed;
        }
        if (pawn.direction == Game::Pawn::Up) {
            pawn.y -= state.pawnspeed;
        }
        if (pawn.direction == Game::Pawn::Left) {
            pawn.x -= state.pawnspeed;
        }

        auto grid_pos = [](float x, int grid_spacing) -> int {
            // multiple of grid spacing
            int pos = (int) ((x / grid_spacing) + 0.5f);
            return pos;
        };

        int actual_x = grid_pos(pawn.x, state.grid_spacing);
        int actual_y = grid_pos(pawn.y, state.grid_spacing);
        // Path was never crossed before
        auto coord = pawn.path.find({actual_x, actual_y});
        if (coord == pawn.path.end()) {
            pawn.path[{actual_x, actual_y}] = pawn.direction;
        } else {
            // We have crossed this point before, check if it's against us
            if (coord->second != pawn.direction) {
                TraceLog(LOG_DEBUG, "Collide!");
            }
        }
    }


    void Draw(State &state) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        const char *debug_text = TextFormat("FPS %d/60 ", GetFPS());
        DrawText(debug_text, 190, 200, 20, LIGHTGRAY);
        // Draw grid lines
        for (int i = 0; i < state.window_width; i += state.grid_spacing) {
            DrawLine(i, 0, i, state.window_height, BLACK);
        }
        for (int j = 0; j < state.window_height; j += state.grid_spacing) {
            DrawLine(0, j, state.window_width, j, BLACK);
        }

        for (int i = 0; i < state.num_pawns; i++) {
            DrawPawnPath(state.pawns[i], state);
            DrawPawn(state.pawns[i]);
        }

        EndDrawing();
    }
}

int main() {
    Game::State state;
    for (int i = 0; i < state.num_pawns; i++) {
        state.pawns[i].x = state.grid_spacing + state.grid_spacing * i;
        state.pawns[i].y = state.grid_spacing + state.grid_spacing * i;
    }
    SetTraceLogLevel(LOG_DEBUG);
    InitWindow(state.window_width, state.window_height, "raylib [core] example - basic window");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        Game::Tick(state);
        Game::Draw(state);
    }

    CloseWindow();

    return 0;
}