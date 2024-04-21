
#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <cmath>



bool running = false;

float windowWidth = 800;
float windowHeight = 600;
// bounding box inside our display window
float width = 800;
float height = 600;
Camera2D cam = {0};
Rectangle bounds = {};
Font f;
float collisionDamping = 1.0f;
float dropRadius = 5.0f;
float dropSpacing = dropRadius;
float smoothingRadius = 1.0f;

float NUM_DROPS = 50;
const int MAX_DROPS = 1000;
Vector2 start_position[MAX_DROPS] = {
  {width, height},
  {0, 0},
  {width/2, height},
  {0, height},
  {width, 0}
};

Vector2 position[MAX_DROPS] = {};
Vector2 accel[MAX_DROPS] = {};
Vector2 velocity[MAX_DROPS] = {};

void Drop(Vector2 coords, float radius, Color color);
void Text(const char *text, int x, int y, float textsize, Color color);
void TextLeft(const char *text, int x, int y, float textsize, Color color);
void RemoveCollisions(int i);
void UpdateBounds();
// constant downward force
float gravity = -9.81f;
// max speed anything can travel
const float terminal_vel = 50.0f;

Vector2 controlsSize;
Rectangle controlsRect;


void InitMainWindow() {
  InitWindow(windowWidth, windowHeight, "Raylib water");
  SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_BORDERLESS_WINDOWED_MODE);
  Vector2 dpi = GetWindowScaleDPI();
  int monitor = GetCurrentMonitor();
  SetTargetFPS(GetMonitorRefreshRate(monitor));

  windowWidth = GetMonitorWidth(monitor) * dpi.x;
  windowHeight = GetMonitorHeight(monitor) * dpi.y;

  // update camera with new position
  cam.target = {windowWidth / 2, windowHeight / 2};
  cam.offset = {windowWidth / 2, windowHeight / 2};
  cam.rotation = 0.0f;
  cam.zoom = 1.0f;

  UpdateBounds();

  SetWindowSize(windowWidth, windowHeight);
  // enable raygui
  GuiEnable();
  f = LoadFontEx("./fonts/JetBrainsMonoNerdFont-Regular.ttf", 20, nullptr, 0);
  if (IsFontReady(f)) {
    GuiSetFont(f);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
  }

  controlsSize = {400, 700};
  controlsRect = { windowWidth/2 + controlsSize.x + 150, 10, controlsSize.x, controlsSize.y };
}


void InitDrops() {
  int dropsPerRow = sqrt(NUM_DROPS);
  int dropsPerCol = (NUM_DROPS - 1) / dropsPerRow + 1;
  float spacing = dropRadius * 2 + dropSpacing;
  for (int i=0;i<NUM_DROPS;i++) { 
    // don't use predetermined locations
    //position[i] = start_position[i];
    float x = width/2 + (i % dropsPerRow - dropsPerRow / 2.0f + 0.5f) * spacing;
    float y = height/2 + (i / (float)dropsPerRow - dropsPerCol / 2.0f + 0.5f) * spacing;
    position[i] = {x, y};
    velocity[i] = {0, 0};
  }
}

void DrawControls(Rectangle rect) {
  int line = 0;
  int lineSize = 20;
  int indent = 90;
  int outdent = 150;
  GuiWindowBox(rect, "Controls"); 
  float prevWidth = width;
  float prevHeight = height;
  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Width", TextFormat("%2.0f", width), &width, 100.0f, windowWidth);
  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Height", TextFormat("%2.0f", height), &height, 100.0f, windowHeight);
  if (width != prevWidth || height != prevHeight) {
    UpdateBounds();
  }
  float prevDrops = NUM_DROPS;
  float prevSpacing = dropSpacing;
  float prevRadius = dropRadius;
  float prevSmoothing = smoothingRadius;
  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Drops", TextFormat("%2.0f", NUM_DROPS), &NUM_DROPS, 1.0f, MAX_DROPS);
  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Spacing", TextFormat("%2.2f", dropSpacing), &dropSpacing, 0.0f, 100.0f);
  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Radius", TextFormat("%2.2f", dropRadius), &dropRadius, 1.0f, 100.0f);
  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Smoothing", TextFormat("%2.2f", smoothingRadius), &smoothingRadius, 1.0f, 100.0f);

  if (NUM_DROPS != prevDrops || dropSpacing != prevSpacing || dropRadius != prevRadius || smoothingRadius != prevSmoothing) {
    InitDrops();
  }

  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Gravity", TextFormat("%2.2f", gravity), &gravity, -50.0f, 0.0f);
  GuiSlider({rect.x + indent, rect.y + 10 + (lineSize * ++line), rect.width - outdent, 5}, "Damping", TextFormat("%2.2f", collisionDamping), &collisionDamping, 0.0f, 1.0f);


}

void ProcessInput() {
  float dt = GetFrameTime();
  float move_speed = 50.0f;
  float zoom_speed = 2.0f;
  float rotation_speed = 100.0f;
  if (IsKeyDown(KEY_A)) {
    cam.offset.x -= move_speed * dt;
  }
  if (IsKeyDown(KEY_D)) {
    cam.offset.x += move_speed * dt;
  }
  if (IsKeyDown(KEY_W)) {
    cam.offset.y -= move_speed * dt;
  }
  if (IsKeyDown(KEY_S)) {
    cam.offset.y += move_speed * dt;
  }
  if (IsKeyDown(KEY_J)) {
    cam.target.x += move_speed * dt;
  }
  if (IsKeyDown(KEY_L)) {
    cam.target.x -= move_speed * dt;
  }
  if (IsKeyDown(KEY_I)) {
    cam.target.y += move_speed * dt;
  }
  if (IsKeyDown(KEY_K)) {
    cam.target.y -= move_speed * dt;
  }
  if (IsKeyDown(KEY_Z)) {
    cam.zoom += zoom_speed * dt;
  }
  if (IsKeyDown(KEY_X)) {
    cam.zoom -= zoom_speed * dt;
  }
  if (IsKeyDown(KEY_Q)) {
    cam.rotation -= rotation_speed * dt;
    if (cam.rotation < 0) {
      cam.rotation += 360;
    }
  }
  if (IsKeyDown(KEY_E)) {
    cam.rotation += rotation_speed * dt;
    if (cam.rotation > 360) {
      cam.rotation -= 360;
    }
  }
  if (IsKeyPressed(KEY_R)) {
    InitDrops();
  }
  if (IsKeyPressed(KEY_SPACE)) {
    running = !running;
  }
}

void UpdateBounds() {
  bounds = {
    windowWidth / 2 - width / 2,
    windowHeight / 2 - height / 2,
    width,
    height
  };
}

float SmoothingKernel(float radius, float dst) {
  float volume = PI * pow(radius, 8) / 4;
  float value = fmax(0, radius * radius - dst * dst);
  return value * value * value / volume;
}

float CalculateDensity(Vector2 point) {
  float density = 0;
  const float mass = 1;

  // loop over all drops
  // TODO: optimize to only look at particles within smoothing radius
  for (int i=0;i<NUM_DROPS;i++) {
    float dst = Vector2Distance(position[i], point);
    float influence = SmoothingKernel(smoothingRadius, dst);
    density += mass * influence;
  }

  return density;
}

void UpdateDrops() {
  float dt = GetFrameTime();
  Vector2 direction = Vector2Zero();
  float radians = (cam.rotation * PI / 180.0f);

  // calculate down force of gravity with rotation
  direction.y += cosf(radians);
  direction.x -= sinf(radians);

  if (!Vector2Equals(direction, Vector2Zero())) {
    // normalize directional movement
    direction = Vector2Normalize(direction);
  }

    // scale movement with gravity

  for (int i = 0;i<NUM_DROPS;i++) {
    velocity[i].y += direction.y * gravity * dt * CalculateDensity(position[i]);
    velocity[i].x += direction.x * gravity * dt * CalculateDensity(position[i]);
    // artifically limit objects to moving at terminal velocity
    velocity[i].y = fminf(terminal_vel, velocity[i].y);
    velocity[i].x = fminf(terminal_vel, velocity[i].x);
    // move object in world due to acceleration
    position[i].y += velocity[i].y;
    position[i].x += velocity[i].x;
    // bounce off ground
    RemoveCollisions(i);

  }
}
void RemoveCollisions(int i) {
  if (position[i].x <= 0+dropRadius || position[i].x >= width-dropRadius) {
    velocity[i].x *= -1 * collisionDamping;
    position[i].x = fmaxf(0+dropRadius, fminf(width-dropRadius, position[i].x));
  }
  if (position[i].y <= 0+dropRadius || position[i].y >= height-dropRadius) {
    velocity[i].y *= -1 * collisionDamping;
    position[i].y = fmaxf(0+dropRadius, fminf(height-dropRadius, position[i].y));
  }
}

void DropSmoothing(Vector2 coords, Color color) {
  DrawCircleGradient(bounds.x + coords.x, bounds.y + bounds.height - coords.y, smoothingRadius*50.0f, color, Fade(color, 0.0f));
}

void Drop(Vector2 coords, float radius, Color color) {
  DrawCircleV({ bounds.x + coords.x, bounds.y + bounds.height - coords.y }, radius, color);
  //TextLeft(TextFormat("%2.2f, %2.2f", coords.x, coords.y), coords.x, coords.y, 20, RAYWHITE);
}

// Draws text at the world coordinates given after camera shifting
void Text(const char *text, int x, int y, float textsize, Color color) {
  DrawTextEx(f, text, {bounds.x + x, bounds.y + bounds.height - y}, textsize, 1.0f, color);
}
// Draws text at the world coordinates given after camera shifting, with the 
// rightmost edge as the given coordinates (text is pushed to the left)
void TextLeft(const char *text, int x, int y, float textsize, Color color) {
  Vector2 textdimensions = MeasureTextEx(GetFontDefault(), text, textsize, 2.0f);
  DrawTextEx(f, text, {bounds.x + x - textdimensions.x, bounds.y + bounds.height - y}, textsize, 1.0f, color);
}

void DrawAxes() {
  TextLeft(TextFormat("(%0.0f, %0.0f)", 0, 0), 0, 0, 20, RAYWHITE);
  Text(TextFormat("(%0.0f, %0.0f)", width, 0), width, 0, 20, RAYWHITE);
  TextLeft(TextFormat("(%0.0f, %0.0f)", 0, height), 0, height, 20, RAYWHITE);
  Text(TextFormat("(%0.0f, %0.0f)", width, height), width, height, 20, RAYWHITE);
}
void DrawDrops() {
  for (int i=0;i<NUM_DROPS;i++) {
    DropSmoothing(position[i], BLUE);
  }
  for (int i=0;i<NUM_DROPS;i++) {
    Drop(position[i], dropRadius, RAYWHITE);
  }
}

int main() {
  InitMainWindow();
  InitDrops();


  while (!WindowShouldClose()) {
    ProcessInput();
    UpdateBounds();
    if (running) {
      UpdateDrops();
    }
    BeginDrawing();
      ClearBackground(BLACK);
      // outer window outline
      DrawRectangleLinesEx(Rectangle { 0, 0, windowWidth, windowHeight }, 1.0f, RAYWHITE);
      DrawTextEx(f, TextFormat("Window: %2.2f, %2.2f Frame: %2.2f, %2.2f", windowWidth, windowHeight, width, height), {10, 10}, 20.0f, 1.0f, RAYWHITE);
      DrawTextEx(f, TextFormat("Target: %2.2f, %2.2f Offset: %2.2f, %2.2f Zoom: %2.2f Rotation: %2.2f", cam.target.x, cam.target.y, cam.offset.x, cam.offset.y, cam.zoom, cam.rotation), {10, 30}, 20.0f, 1.0f, RAYWHITE);
      DrawControls(controlsRect);
      BeginMode2D(cam);
        DrawAxes();
        // inner bounding box outline
        DrawRectangleLinesEx(bounds, 1.0f, GRAY);
        BeginBlendMode(BLEND_ADDITIVE);

          DrawDrops();
        EndBlendMode();

      EndMode2D();
    EndDrawing();
  }
  CloseWindow();
}
