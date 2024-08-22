#include <raylib.h>
#include "controllers.h"
#include "main.h"
#include "motor.h"
#include "pid.h"
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include "solver.h"

#define TS 1.0f /* motor simulation time */


void DrawCar(int x, int y)
{
  DrawRectangle(x-5,y-50,50,20, BLACK);
}

int MotorSelect(int *motor_type)  {
  int motor_selected = 0;
  motor_selected = GuiButton((Rectangle){600, 70, 120, 20},"Motor 1");
  if(motor_selected)
  {
    *motor_type = 0;
    return 1;
  }
  motor_selected = GuiButton((Rectangle){600, 90, 120, 20},"Motor 2");
  if(motor_selected)
  {
    *motor_type = 1;
    return 1;
  }

  motor_selected = GuiButton((Rectangle){600, 110, 120, 20},"Motor 3");
  if(motor_selected)
  {
    *motor_type = 2;
    return 1;
  }
  return 0;
}

int main(void)
{
  // Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth = 800;
  const int screenHeight = 450;
  /* motor parameters */
  float t = 0.0; /* time */
  int counter = 0;
  int ctrl_run = 0;
  float motor_state[N] = {0.0}; /* buffer for output */
  float w_ref = 0.0;
  int motor_type = 0;
  /* should be a button */
  motor_turn_on(motor_state);

  int posX = screenWidth / 2;
  int posY = screenHeight / 2;
  Vector2 touchPosition = {0, 0};
  int motor_selected = 0; 
  Vector2 center = {80, 80};
  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
  set_motor(motor_type);

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose())    // Detect window close button or ESC key
  {
    
    /* plant motor stuff */
    step(t, t+TS, motor_state);
    t = t+TS;
    
    /* controller stuff */
    control_runner(&w_ref, &motor_state[WR], &motor_state[ID], &motor_state[IQ],&motor_state[VD], &motor_state[VQ]);
    
    posX += motor_state[WR]; 
    if(posX > screenWidth)
    {
      posX = 0;
    }
    else if (posX < 0)
    {
      posX = screenWidth - 1;
    }
    
    BeginDrawing();
    ClearBackground(RAYWHITE);

    
    GuiSlider((Rectangle){600,40,120,20}, "Reference", "M/S", &w_ref, -20.0f, 20.0f);
    GuiTextBox((Rectangle){600, 20, 120, 20}, TextFormat("%lf", w_ref), 12, 0);
    if(MotorSelect(&motor_type))
    {
      set_motor(motor_type);
      motor_turn_on(motor_state);
    }
    DrawText(TextFormat("Speed:\t\t %d", (int)motor_state[WR]), 600, 250, 16, GRAY);
    DrawText(TextFormat("D Current: %d", (int)motor_state[ID]), 600, 300, 16, GRAY);
    DrawText(TextFormat("Q Current: %d", (int)motor_state[IQ]), 600, 350, 16, GRAY);
    DrawCircleSector(center, 60, 0, 360, 40, GRAY);
    DrawCircleSector(center, 60, motor_state[THETA], motor_state[THETA] + 10, 40, BLUE);
    DrawCar(posX,posY);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow();        // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
