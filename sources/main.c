#include <raylib.h>

#define SWC_NUM 3

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

  int dickPosX =  screenWidth / 2;
  int dickPosY =  screenHeight / 2;
  int amountX = 5;
  int amountY = 5;
  Vector2 touchPosition = {0, 0};
  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

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

    ClearBackground(RAYWHITE);
    drawDick(dickPosX,dickPosY);
    dickPosX += amountX;
    dickPosY += amountY ;
    if( GESTURE_HOLD == GetGestureDetected()){
      touchPosition = GetTouchPosition(0);
      dickPosX = touchPosition.x;
      dickPosY = touchPosition.y;
    }
    if((dickPosX > screenWidth + 20)  || (dickPosX < 0))
    {
      amountX *= -1;
    }
    if((dickPosY > screenHeight + 50) || (dickPosY < 0) )
    {
      amountY *= -1;
    }
    for(int i = 0; i < SWC_NUM; i++)
    {
      swc_draw(&swcs[i], 200*i+100, touchPosition.y);
    }
    DrawText("RAYLIB", screenWidth/2 , screenHeight/2 + 60, 20, LIGHTGRAY);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow();        // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
