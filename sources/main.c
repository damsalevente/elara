#include <raylib.h>
#include "controllers.h"
#include "main.h"
#include "motor.h"
#include "pid.h"
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include "solver.h"

#define SWC_NUM 3
#define TS 1.0f

typedef struct swc_t { 
  char name[64];
  int ID;
  int provides[10];
  int requires[10];
} swc_t;


swc_t swcs[] = {
  {.name = "swc1", .ID = 1},
  {.name = "swc2", .ID = 2},
  {.name = "swc3", .ID = 3},
};

void swc_draw(swc_t *swc, int posX, int posY)
{
  DrawRectangle(posX,posY,100,100,RED);
  DrawRectangle(posX+5,posY+5,100-10,100-10,LIGHTGRAY);
  DrawText(swc->name, posX, posY-20, 20, BLUE);
}

void drawDick(int x, int y)
{
  DrawCircle(x+10, y+10, 10.0f, BLACK);
  DrawCircle(x-10, y+10, 10.0f, BLACK);
  DrawRectangle(x-5,y-50,10,50, BLACK);
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
  float u[N] = {0.0}; /* buffer for output */
  int duration;
  float w_ref = 0.0;
  int motor_type = 0;
  u[TI] = 4.5;
  /* should be a button */
  motor_turn_on(u);

  int dickPosX =  screenWidth / 2;
  int dickPosY =  screenHeight / 2;
  int amountX = 5;
  int amountY = 5;
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
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    /* plant motor stuff */
    step(t, t+TS, u);
    t = t+TS;
    GuiSlider((Rectangle){600,40,120,20}, "Reference", "M/S", &w_ref, -20.0f, 20.0f);
    GuiTextBox((Rectangle){600, 20, 120, 20}, TextFormat("%lf", w_ref), 12, 0);
    
    motor_selected = GuiButton((Rectangle){600, 70, 120, 20},"Motor1");
    if(motor_selected)
    {
      motor_type = 0;
      set_motor(motor_type);
      motor_turn_on(u);
    }
    motor_selected = GuiButton((Rectangle){600, 90, 120, 20},"Motor1");
    if(motor_selected)
    {
      motor_type = 1;
      set_motor(motor_type);
      motor_turn_on(u);
    }
    
    motor_selected = GuiButton((Rectangle){600, 110, 120, 20},"Motor1");
    if(motor_selected)
    {
      motor_type = 2;
      set_motor(motor_type);
      motor_turn_on(u);
    }
    /* controller stuff */
    control_runner(&w_ref, &u[WR], &u[ID], &u[IQ],&u[VD], &u[VQ]);

    dickPosX += u[WR];
    //dickPosY += 0.01 * u[THETA];

    
    ClearBackground(RAYWHITE);
    if( GESTURE_HOLD == GetGestureDetected()){
      touchPosition = GetTouchPosition(0);
      //dickPosX = touchPosition.x;
      //dickPosY = touchPosition.y;
    }
    if(dickPosX > screenWidth)
    {
      dickPosX = 0;
    }
    else if (dickPosX < 0)
    {
      
      dickPosX = screenWidth;
    }
    DrawText(TextFormat("Speed:\t\t %d", (int)u[WR]), 600, 200, 13, GRAY);
    DrawText(TextFormat("D Current: %d", (int)u[ID]), 600, 300, 13, GRAY);
    DrawText(TextFormat("Q Current: %d", (int)u[ID]), 600, 350, 13, GRAY);
    DrawCircleSector(center, 60, 0, 360, 40, GRAY);
    DrawCircleSector(center, 60, u[THETA], u[THETA] + 20, 40, BLUE);
    
    drawDick(dickPosX,dickPosY);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow();        // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
