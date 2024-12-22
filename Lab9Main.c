// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Neev and Oscar
// Last Modified: 8/21/2024

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/DAC5.h"
#include "../inc/Arabic.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
//#include "GameEngineUtilities.h"
//#include "Sound.h"
// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

Arabic_t ArabicAlphabet[]={
alif,ayh,baa,daad,daal,dhaa,dhaal,faa,ghayh,haa,ha,jeem,kaaf,khaa,laam,meem,noon,qaaf,raa,saad,seen,sheen,ta,thaa,twe,waaw,yaa,zaa,space,dot,null
};
Arabic_t Hello[]={alif,baa,ha,raa,meem,null}; // hello
Arabic_t WeAreHonoredByYourPresence[]={alif,noon,waaw,ta,faa,raa,sheen,null}; // we are honored by your presence
int main0(void){ // main 0, demonstrate Arabic output
  Clock_Init80MHz(0);
  LaunchPad_Init();
  ST7735_InitR(INITR_REDTAB);
  ST7735_FillScreen(ST7735_WHITE);
  Arabic_SetCursor(0,15);
  Arabic_OutString(Hello);
  Arabic_SetCursor(0,31);
  Arabic_OutString(WeAreHonoredByYourPresence);
  Arabic_SetCursor(0,63);
  Arabic_OutString(ArabicAlphabet);
  while(1){
  }
}
uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}

uint8_t Flag = 0;
uint32_t buttonpressed = 0;
uint32_t position = 0;

typedef enum {dead, alive} status_t;
typedef struct {
    uint32_t x;                     // x-coord
    uint32_t y;                     // y-coord
    int32_t vx;                     // pixels/30Hz
    int32_t vy;                     // pixels/30Hz
    const unsigned short* image;
    const unsigned short* blank;
    status_t life;
    uint32_t w;
    uint32_t h;
    uint8_t needDraw;
} sprite_t;

sprite_t player; // player instance of the struct
sprite_t enemy[18];
sprite_t player_bullets[14];
sprite_t enemy_bullets[18];

int player_bullet_size = sizeof(player_bullets)/sizeof(player_bullets[0]);
int enemy_bullet_size = sizeof(enemy_bullets)/sizeof(enemy_bullets[0]);
int enemy_size = sizeof(enemy)/sizeof(enemy[0]);
uint32_t score;
uint8_t language;           // 0 for English, 1 for Arabic
int continue_game;          // Whether the game should continue or not
int any_alive;              // Whether there are any enemies still left alive
int flag;                   // semaphore, which tells the foreground to redraw the image (recall background vs foreground processes)
uint32_t prevswitch = 0;
uint32_t currentswitch;
uint32_t prevposx;
uint32_t prevposy;


//**********************************Duplicated Functions for Internal Usage***************************//

static uint32_t Q=1;
static uint32_t Random32_Internal(void){
  Q = 1664525*Q+1013904223;
  return Q;
}
static uint32_t Random_Internal(uint32_t n){
  return (Random32_Internal()>>16)%n;
}

void OutUDec(uint32_t n) {
    if(n >= 10) {
        OutUDec(n / 10);
        n = n % 10;
    }
    ST7735_OutChar(n + '0');
}

//***********************************Draw Mechanics*********************************//
int Draw(sprite_t* sprite) {
    if(sprite->needDraw) {
        if(sprite->life == alive) {
            // For player sprite only
            if(sprite == &player) {
                // Clear old position first
                ST7735_DrawBitmap(prevposx, sprite->y, sprite->blank, sprite->w, sprite->h);
                // Draw new position
                ST7735_DrawBitmap(sprite->x, sprite->y, sprite->image, sprite->w, sprite->h);
                // Update previous position
                prevposx = sprite->x;

//            if(sprite == &player_bullets){
//
//            }
            } else {
                // Normal drawing for other sprites
                ST7735_DrawBitmap(sprite->x, sprite->y, sprite->image, sprite->w, sprite->h);
            }
        } else {
            ST7735_DrawBitmap(sprite->x, sprite->y, sprite->blank, sprite->w, sprite->h);
        }
        sprite->needDraw = 0;
        return 1;
    }
    return 0;
}



void DrawGame() {

    //ST7735_FillScreen(0x0000); // <--- THIS FIXES THE TRAILING ISSUE BUT MAKES THE GAME LOOP CHOPPY!

    Draw(&player);
    for(int i = 0; i < enemy_size; i++) {
        Draw(&enemy[i]);
    }
    for(int i = 0; i < enemy_bullet_size; i++) {
        Draw(&enemy_bullets[i]);
    }
    for(int i = 0; i < player_bullet_size; i++) {
        Draw(&player_bullets[i]);
    }
}


//************************************Game & Variable Initialization**********************************//

void PlayerInit(sprite_t* sprite, uint32_t x_param, uint32_t y_param) {
    // I don't think the velocity stuff applies since he moves exclusively horizontally based on sampling and manual entry
    // TODO: Fill out missing params
    sprite->x = x_param;
    sprite->y = y_param;
    sprite->vx = 1;
    sprite->vy = 1;
    sprite->image = PlayerShip0;
    sprite->blank = BlankPlayerShip0;
    sprite->life = alive;
    sprite->w = 18;
    sprite->h = 8;
    sprite->needDraw = 1;
}


void EnemyInit(sprite_t* sprite, uint32_t x_param, uint32_t y_param, int32_t vx_param, int32_t vy_param, status_t life_param, uint8_t needDraw_param) {
    sprite->x = x_param;
    sprite->y = y_param;
    sprite->vx = vx_param;
    sprite->vy = vy_param;
    sprite->image = SmallEnemy10pointA;
    sprite->blank = BlankEnemy10pointA;
    sprite->life = life_param;
    sprite->w = 16;
    sprite->h = 10;
    sprite->needDraw = needDraw_param;
}


void BulletInit(sprite_t* sprite, uint32_t x_param, uint32_t y_param, int32_t vy_param, status_t life_param, uint8_t needDraw_param) {
    sprite->x = x_param;
    sprite->y = y_param;
    sprite->vx = 0;
    sprite->vy = vy_param;
    sprite->image = Laser0;
    sprite->blank = BlankLaser0;
    sprite->life = life_param;
    sprite->w = 2;
    sprite->h = 9;
    sprite->needDraw = needDraw_param;
}

void GameInit(void) { // TODO
    flag = 0;
    score = 0;
    any_alive = 1;
    continue_game = 1;

    // Initialize w/ 6 stationary invaders at the top - thanks Valvano video for the suggestion:)
    for (int i = 0; i < 6; i++) {
        EnemyInit(&enemy[i], 20*i, 20, 1, 1, alive, 1);
    }

    PlayerInit(&player, 64, 145); // These are guess positional values

    // Initialize all the bullets and the other enemies with life = dead and needDraw = 0. The rest shouldn't matter, I think since they won't appear on screen or have any operations performed on them.
//    for(int i = 6; i < enemy_size; i++) {
//        enemy[i].life = dead;
//        enemy[i].needDraw = 0;
//    }

    for(int i = 0; i < enemy_bullet_size; i++) {
        enemy_bullets[i].life = dead;
        enemy_bullets[i].needDraw = 0;
    }
    for(int i = 0; i < player_bullet_size; i++) {
        player_bullets[i].life = dead;
        player_bullets[i].needDraw = 0;
    }

    ST7735_FillScreen(0x0000);
    ST7735_DrawBitmap(0, 0, OuterSpace, 160, 128);
    Clock_Delay(800000); // SMALL DELAY MAY DELETE
    DrawGame();

    __enable_irq();
}

void GameBegin(void) {
    ST7735_FillScreen(0x0000);
    ST7735_DrawBitmap(0, 100, marquee, 126, 68);


    ST7735_SetCursor(64, 10);
    ST7735_OutString("English/Espanol?");

    while((buttonpressed>>17 & 0x01) == 0 && (buttonpressed>>13 & 0x01) == 0){ // Wait for button to be pressed in beginning of game, wont ever be 0 again so no infinite while loop
              buttonpressed = Switch_In(); // Polling for english or spanish
          }

          if((buttonpressed)>>17 & 0x01){ language = 0; } // English
          if((buttonpressed)>>13 & 0x01 ){ language = 1; } // Spanish

          Clock_Delay(800000); // SMALL DELAY MAY DELETE
}

void GameEnd(void) {
    __disable_irq();  // Disable interrupts first
    Clock_Delay(800000);  // Add delay to ensure display is ready
    ST7735_FillScreen(0x0000);

    if(player.life == dead) {
        if(language == 0) {
            ST7735_SetCursor(0,0);  // Set cursor position
            ST7735_OutString("You lost lil bro!");
            ST7735_SetCursor(0, 10);
            ST7735_OutString("Score: ");
            OutUDec(score);
        } else {
            ST7735_SetCursor(0,0);
            ST7735_OutString("Perdiste hermano!");
            ST7735_SetCursor(0, 10);
            ST7735_OutString("Puntuacion: ");
            OutUDec(score);
        }
    } else {
        if(language == 0) {
            ST7735_SetCursor(0,0);
            ST7735_OutString("You win big bro!");
            ST7735_SetCursor(0, 10);
            ST7735_OutString("Score: ");
            OutUDec(score);
        } else {
            ST7735_SetCursor(0,0);
            ST7735_OutString("Ganaste hermano!");
            ST7735_SetCursor(0, 10);
            ST7735_OutString("Puntuacion: ");
            OutUDec(score);
        }
    }
}

//*************************Player Shoot Mechanics & ISR******************************//

int PlayerShoot(sprite_t* sprite, int32_t vy_param) {
    // Use RNG to randomly select which invaders are to shoot
    int index2;
        for(int i = 0; i < player_bullet_size; i++) { // Search for available player_bullet
            Sound_Shoot();
            if(player_bullets[i].life == dead) {
                index2 = i;
                break;
            }
            if(i == player_bullet_size - 1) return 0; // Didn't shoot
        }

        if(1){//Random_Internal(500) < 30) { // Should a bullet about every 0.5s when considered in the broader loop of InvaderShootGame & the 30Hz clock w/ which TimerG12 will operate
            BulletInit(&player_bullets[index2], sprite->x, sprite->y - 15, vy_param, alive, 1);
            return 1; // Shot!
        } else {
            return 2; // No shoot
        }
    }

//void PlayerShootGame(void){ // WORK ON LASER MOVING!!!
//
//}

//***********************************Move Mechanics*********************************//
int Move(sprite_t* sprite) { // TODO: test x,y limits on LCD
    if(sprite->life == alive) {
        sprite->needDraw = 1;
        if(sprite->x < 0 || sprite->x > 102) { // If it's going out the ends of the LCD, change direction of movement
            sprite->vx = -(sprite->vx);
//            sprite->x += sprite->vx;
        }
        if( (sprite->y>10) && (sprite->y<153) ) { // If it's within the screen limit (since we care about up-down)
            sprite->x += sprite->vx;
            sprite->y +=  sprite->vy;
        } else if(sprite->image == SmallEnemy10pointA){ // Bounce back up or down if is an enemy
            sprite->vy = -(sprite->vy);
            sprite->x += sprite->vx;
            sprite->y +=  sprite->vy;
        } else { // Die if is bullet
            sprite->life = dead;
        }
        return 1; // Movement (alive at start of function call)
    }
    return 0; // No movement (dead at start of function call)
}

void MoveGame() {
    // Move the bullets
    // Straight y-directional motion
    // Move the invaders
        // if they hit wall, bounce back with opposite velocity
    // Move the player -- Sample ADC


        // Calibrate to have ADC correspond to x positioning

    // Move the player's bullets
    for(int i = 0; i < player_bullet_size; i++) {
        Move(&player_bullets[i]);
    }

    // Move the enemy's bullets
    for(int i = 0; i < enemy_bullet_size; i++) {
        Move(&enemy_bullets[i]);
    }

    // Move the enemies
    any_alive = 0;
    for(int i = 0; i < enemy_size; i++) {
        // Note: This control flow logic makes it to where when the last one hits
        // the end of the screen, you won't see it die, but instead, you'll be taken
        // to the end screen w/ score & win msg.
        Move(&enemy[i]);
        if(enemy[i].life == alive) any_alive = 1;
    }

    //Move the player dependent on ADC sampling
    if(player.life == alive) {
        player.needDraw = 1;
        int pos = ADCin();
        player.x = (pos * 120)/4096;
//        player.x = Convert(pos);
        //player.x = (pos<<7)>>12; ///(pos*128 then /4096)
//        Move(&player);
        //Sound_Fastinvader1();
    }
}

//***********************************Collision Mechanics*********************************//
int Collide(sprite_t* sprite1, sprite_t* sprite2) {
    return (sprite1->x < sprite2->x + sprite2->w &&
            sprite1->x + sprite1->w > sprite2->x &&
            sprite1->y < sprite2->y + sprite2->h &&
            sprite1->y + sprite1->h > sprite2->y);
}

void CollideGame() {
    // Check if the invaders hit the player
    for(int i = 0; i < enemy_size; i++) {
        if(enemy[i].life == alive) {
            if(Collide(&enemy[i], &player) == 1) {
                Sound_Explosion();
                player.life = dead;
                enemy[i].life = dead;
                continue_game = 0;
                player.needDraw = 1;
                enemy[i].needDraw = 1;
            }
        }
    }

    // Check if the player missiles hit the invaders
    for(int i = 0; i < player_bullet_size; i++) {
        for(int j = 0; j < enemy_size; j++) {
            if(player_bullets[i].life == alive && enemy[j].life == alive) {
                if(Collide(&player_bullets[i], &enemy[j]) == 1) {
                    Sound_Explosion();
                    player_bullets[i].life = dead;
                    enemy[j].life = dead;
                    score += 100;
                    player_bullets[i].needDraw = 1;
                    enemy[j].needDraw = 1;
                }
            }
        }
    }

    // Check if the enemy missiles hit the player
    for(int i = 0; i < enemy_bullet_size; i++) {
        if(enemy_bullets[i].life == alive) {
            if(Collide(&enemy_bullets[i], &player) == 1) {
                Sound_Explosion();
                player.life = dead;
                enemy_bullets[i].life = dead;
                continue_game = 0;
                player.needDraw = 1;
                enemy_bullets[i].needDraw = 1;
            }
        }
    }

    // Check if the player missiles hit the enemy missies
    for(int i = 0; i < player_bullet_size; i++) {
        for(int j = 0; j < enemy_bullet_size; j++) {
            if(player_bullets[i].life == alive && enemy_bullets[j].life == alive) {
                if(Collide(&player_bullets[i], &enemy_bullets[j]) == 1) {
                    Sound_Explosion();
                    player_bullets[i].life = dead;
                    enemy_bullets[j].life = dead;
                    player_bullets[i].needDraw = 1;
                    enemy[j].needDraw = 1;
                }
            }
        }
    }
}

//****************************Invader Shooting Mechanics*********************************//
// returns 0 if no available bullets, 1 if it shot from that sprite, 2 if there are available bullets but it didn't shoot
int InvaderShoot(sprite_t* sprite, int32_t vy_param) {
    // Use RNG to randomly select which invaders are to shoot
    int index;
    for(int i = 0; i < enemy_bullet_size; i++) { // Search for available enemy_bullet
        //Sound_Shoot();
        if(enemy_bullets[i].life == dead) {
            index = i;
            break;
        }
        if(i == enemy_bullet_size - 1) return 0; // Didn't shoot
    }

    if(Random_Internal(5000) < 12) { // Should a bullet about every 0.5s when considered in the broader loop of InvaderShootGame & the 30Hz clock w/ which TimerG12 will operate
        BulletInit(&enemy_bullets[index], sprite->x, sprite->y + 23, vy_param, alive, 1);
        return 1; // Shot!
    } else {
        return 2; // No shoot
    }

}

void InvaderShootGame() {
    // Move the enemy's bullets
    for(int i = 0; i < enemy_size; i++) {
        if(enemy[i].life == alive) InvaderShoot(&enemy[i], 1); // TODO: Figure out the vx & vy params | TEMP IN HERE
    }
}

// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){

    uint32_t pos,msg;

  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
//      ST7735_FillScreen(0x0000); Debugging
//      ST7735_OutString("Score: ");

    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// game engine goes here

    //position = ADCin();
    //position = Convert(position); // Stores values into position

    currentswitch = ((Switch_In()>>19) & 0x01); // 2) read input switches

    if(currentswitch != 0 && prevswitch == 0){
        PlayerShoot(&player, -1); // Should play sound too already
    }

    Clock_Delay(50);
    prevswitch = currentswitch;

    MoveGame(); // 3) move sprites & sample ADC data

    InvaderShootGame();

    Flag = 1; // 5) set semaphore
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}

uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};


// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
int main(void){ // final main
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  ADCinit();     //PB18 = ADC1 channel 5, slidepot
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init();  // initialize sound
  DAC5_Init();
  TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  int g12_priority = 1;
  int g12_period = 80000000/30; // 30Hz
  TimerG12_IntArm(g12_period, g12_priority);
  GameBegin();
  GameInit();
  // initialize all data structures
  __enable_irq();

  while(1){
      while(Flag == 1){ // wait for semaphore & check for endgame || NOTE: DOES IRQ RUN BEFORE MAIN??

          Flag = 0; // clear semaphore
          DrawGame();    // update ST7735R
          ST7735_SetCursor(0, 0);

          if(language == 0){
              ST7735_OutString("Score: ");
              OutUDec(score);
          }

          else if(language == 1){
              ST7735_OutString("Puntuacion: ");
              OutUDec(score);
          }

          CollideGame(); // Any collisions?

          if(continue_game == 0 || any_alive == 0) { // Game end?
              Flag = 0;  // Clear any pending flags
              DrawGame();  // Ensure final frame is drawn
              GameEnd();
              break;  // Exit the main loop
          }

      // Oscar Note for end game (I think):
          // - check if any enemies are alive w/ any_alive (if no, player wins)
          // - check if player is alive (if no, player loses)
          // - check for the continue_game variable (might honestly be redundant to the previous check but idk)
  }
}
}
