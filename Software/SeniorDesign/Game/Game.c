#include "Game.h"

GameState_t Game;
bool restart;

/**Helper functions**/
void ScreenTap(){
    restart = true;
    P4->IFG &= ~BIT0;
}

static uint32_t randomColorGen(){
    switch(SystemTime & 0xF0){
        case 0x00:
            return LCD_WHITE;
        case 0x10:
            return LCD_BLUE;
        case 0x20:
            return LCD_RED;
        case 0x30:
            return LCD_MAGENTA;
        case 0x40:
            return LCD_GREEN;
        case 0x50:
            return LCD_CYAN;
        case 0x60:
            return LCD_YELLOW;
        case 0x70:
            return LCD_RED;
        case 0x80:
            return LCD_PURPLE;
        case 0x90:
            return LCD_ORANGE;
        case 0xA0:
            return LCD_PINK;
        case 0xB0:
            return LCD_OLIVE;
        default:
            return LCD_WHITE;
    }
}

void Minkowski(Ball_t *ball, GeneralPlayerInfo_t *player){
    int32_t w = BALL_SIZE_D2 + PADDLE_LEN_D2;
    int32_t h = BALL_SIZE_D2 + PADDLE_WID_D2;

    int32_t dx = ball->currentCenterX - player->currentCenter;
    int32_t dy;

    if(player->position == TOP){
        dy = ball->currentCenterY - TOP_PLAYER_CENTER_Y;
    }else{
        dy = ball->currentCenterY - BOTTOM_PLAYER_CENTER_Y;
    }

    if (abs(dx) <= w && abs(dy) <= h)
    {
        /* collision! */
        ball->color = player->color;

        int32_t wy = w * dy;
        int32_t hx = h * dx;

        if (wy > hx){
            if (wy > -hx){
                /* collision at the top */
                ball->velocityY = -(ball->velocityY) - 1;
            }
            else{
                /* on the left */
                ball->velocityX = -(ball->velocityX) - 1;
                ball->velocityY = -(ball->velocityY);
            }
        }else{
            if (wy > -hx){
                /* on the right */
                ball->velocityX = -(ball->velocityX) + 1;
                ball->velocityY = -(ball->velocityY);
            }
            else{
                /* at the bottom */
                ball->velocityY = -(ball->velocityY) + 1;
            }
        }
    }
}

void WallCheck(Ball_t *ball){
    //Check Horizontal Wall Collisions
    if(ball->currentCenterX >= HORIZ_CENTER_MAX_BALL){
        ball->velocityX = -(ball->velocityX);
    }else if(ball->currentCenterX <= HORIZ_CENTER_MIN_BALL){
        ball->velocityX = -(ball->velocityX);
    }

    //Check Verticle Wall Collisions
    if(ball->currentCenterY >= VERT_CENTER_MAX_BALL){
        //Kill Ball
        ball->alive = 0;

        if(ball->color == Game.players[TOP].color){
            //Update Score
            Game.LEDScores[TOP] = ((Game.LEDScores[TOP] << 1) | 0x1);
        }

        //Check if Done
        if(Game.LEDScores[TOP] >= 0xFF){
            Game.winner = TOP;
            Game.gameDone = true;
            Game.overallScores[TOP]++;
        }

        //End Ball
        G8RTOS_KillSelf();
    }
    else if(ball->currentCenterY <= VERT_CENTER_MIN_BALL){
        //Kill Ball
        ball->alive = 0;

        if(ball->color == Game.players[BOTTOM].color){
            //Update Score
            Game.LEDScores[BOTTOM] = ((Game.LEDScores[BOTTOM] << 1) | 0x1);
        }

        //Check if Done
        if(Game.LEDScores[BOTTOM] >= 0xFF){
            Game.winner = BOTTOM;
            Game.gameDone = true;
            Game.overallScores[BOTTOM]++;
        }

        //End Ball
        G8RTOS_KillSelf();
    }
}

/**Helper functions**/

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame(){
    //Struct initialization
    Game.player.IP_address = getLocalIP();
    Game.player.displacement = 0;
    Game.player.playerNumber = 37;
    Game.player.ready = 1;

    //Send to host
    SendData(&Game.player, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));

    //Receive from Host
    GameState_t tempGame;
    while(0 > (ReceiveData(&tempGame, sizeof(GameState_t))));
    memcpy(&Game, &tempGame, sizeof(GameState_t));

    //Once joined acknowledge connection
    if(Game.player.joined){
        Game.player.acknowledge = 1;
        SendData(&Game.player, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
        ledModeSet(LED_GREEN, LED_15);
    }

    InitBoardState();

    G8RTOS_InitSemaphore(&lcdMutex, 1);
    G8RTOS_InitSemaphore(&ledMutex, 1);
    G8RTOS_InitSemaphore(&wifiMutex, 1);

    G8RTOS_AddThread(&ReadJoystickClient, 30, "Joystick");
    G8RTOS_AddThread(&SendDataToHost, 25, "SendData");
    G8RTOS_AddThread(&ReceiveDataFromHost, 25, "RecData");
    G8RTOS_AddThread(&DrawObjects, 50, "DrawObj");
    G8RTOS_AddThread(&MoveLEDs, 100, "LEDs");
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    G8RTOS_KillSelf();
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost(){
    GameState_t tempGame;
    while(1){
        G8RTOS_WaitSemaphore(&wifiMutex);
        while(0 > (ReceiveData(&tempGame, sizeof(GameState_t)))){
              G8RTOS_SignalSemaphore(&wifiMutex);
              sleep(1);
              G8RTOS_WaitSemaphore(&wifiMutex);
        }
        memcpy(&Game, &tempGame, sizeof(GameState_t));

        if(Game.gameDone){
            G8RTOS_AddThread(&EndOfGameClient, 1, "END");
        }

        sleep(5);
    }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost(){
    while(1){
        G8RTOS_WaitSemaphore(&wifiMutex);
        SendData(&Game.player, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
        G8RTOS_SignalSemaphore(&wifiMutex);
        sleep(2);
    }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient(){
    int16_t xCoord, yCoord;
    while(1){
        GetJoystickCoordinates(&xCoord, &yCoord);
        Game.player.displacement = (xCoord >> 9);
        sleep(10);
    }
}

/*
 * End of game for the client
 */
void EndOfGameClient(){
    G8RTOS_WaitSemaphore(&lcdMutex);
    G8RTOS_WaitSemaphore(&ledMutex);
    G8RTOS_WaitSemaphore(&wifiMutex);

    G8RTOS_KillAllButSelf();

    G8RTOS_InitSemaphore(&lcdMutex, 1);
    G8RTOS_InitSemaphore(&ledMutex, 1);
    G8RTOS_InitSemaphore(&wifiMutex, 1);

    if(Game.winner){
        LCD_Clear(PLAYER_RED);
    }else{
        LCD_Clear(PLAYER_BLUE);
    }

    while(Game.gameDone){
        GameState_t tempGame;
        G8RTOS_WaitSemaphore(&wifiMutex);
        while(0 > (ReceiveData(&tempGame, sizeof(GameState_t)))){
            G8RTOS_SignalSemaphore(&wifiMutex);
            sleep(1);
            G8RTOS_WaitSemaphore(&wifiMutex);
        }
        memcpy(&Game, &tempGame, sizeof(GameState_t));
    }

    InitBoardState();

    G8RTOS_AddThread(&ReadJoystickClient, 30, "Joystick");
    G8RTOS_AddThread(&SendDataToHost, 25, "SendData");
    G8RTOS_AddThread(&ReceiveDataFromHost, 25, "RecData");
    G8RTOS_AddThread(&DrawObjects, 50, "DrawObj");
    G8RTOS_AddThread(&MoveLEDs, 100, "LEDs");
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    G8RTOS_KillSelf();
}

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame(){
    Game.players[BOTTOM].color = PLAYER_BLUE;
    Game.players[BOTTOM].currentCenter = PADDLE_X_CENTER;
    Game.players[BOTTOM].position = BOTTOM;

    Game.players[TOP].color = PLAYER_RED;
    Game.players[TOP].currentCenter = PADDLE_X_CENTER;
    Game.players[TOP].position = TOP;

    SpecificPlayerInfo_t tempClient;
    while(0 > (ReceiveData(&tempClient, sizeof(SpecificPlayerInfo_t))));
    memcpy(&Game.player, &tempClient, sizeof(SpecificPlayerInfo_t));

    if(Game.player.ready){
        Game.player.joined = 1;
        SendData(&Game, Game.player.IP_address, sizeof(GameState_t));
    }

    while(0 > (ReceiveData(&tempClient, sizeof(SpecificPlayerInfo_t))));
    memcpy(&Game.player, &tempClient, sizeof(SpecificPlayerInfo_t));

    if(Game.player.acknowledge){
        ledModeSet(LED_GREEN, LED_14);
    }

    InitBoardState();

    G8RTOS_InitSemaphore(&lcdMutex, 1);
    G8RTOS_InitSemaphore(&ledMutex, 1);
    G8RTOS_InitSemaphore(&wifiMutex, 1);

    G8RTOS_AddThread(&GenerateBall, 50, "GenBall");
    G8RTOS_AddThread(&DrawObjects, 50, "DrawObjs");
    G8RTOS_AddThread(&ReadJoystickHost, 30, "Joystick");
    G8RTOS_AddThread(&SendDataToClient, 25, "SendData");
    G8RTOS_AddThread(&ReceiveDataFromClient, 25, "RecData");
    G8RTOS_AddThread(&MoveLEDs, 100, "LEDs");
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    G8RTOS_KillSelf();
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient(){
    while(1){
        G8RTOS_WaitSemaphore(&wifiMutex);
        SendData(&Game, Game.player.IP_address, sizeof(GameState_t));
        G8RTOS_SignalSemaphore(&wifiMutex);

        if(Game.gameDone){
            G8RTOS_AddThread(&EndOfGameHost, 1, "END");
        }

        sleep(5);
    }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient(){
    SpecificPlayerInfo_t tempClient;
    while(1){
        G8RTOS_WaitSemaphore(&wifiMutex);
        while(0 > ReceiveData(&tempClient, sizeof(SpecificPlayerInfo_t))){
            G8RTOS_SignalSemaphore(&wifiMutex);
            sleep(1);
            G8RTOS_WaitSemaphore(&wifiMutex);
        }

        Game.players[CLIENT].currentCenter += Game.player.displacement;

        if(Game.players[CLIENT].currentCenter > HORIZ_CENTER_MAX_PL){
            Game.players[CLIENT].currentCenter = HORIZ_CENTER_MAX_PL;
        }

        if(Game.players[CLIENT].currentCenter < HORIZ_CENTER_MIN_PL){
            Game.players[CLIENT].currentCenter = HORIZ_CENTER_MIN_PL;
        }

        sleep(2);
    }
}

/*
 * Generate Ball thread
 */
void GenerateBall(){
    while(1){
        if(Game.numberOfBalls < MAX_NUM_OF_BALLS){
            G8RTOS_AddThread(&MoveBall, 50, "Ball");
            Game.numberOfBalls++;
        }

        sleep(Game.numberOfBalls * 1000);
    }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost(){
    int16_t xCoord, yCoord;
    int16_t displacement;
    while(1){
        GetJoystickCoordinates(&xCoord, &yCoord);
        displacement = (xCoord >> 9);
        sleep(10);

        Game.players[HOST].currentCenter += displacement;

        if(Game.players[HOST].currentCenter > HORIZ_CENTER_MAX_PL){
            Game.players[HOST].currentCenter = HORIZ_CENTER_MAX_PL;
        }

        if(Game.players[HOST].currentCenter < HORIZ_CENTER_MIN_PL){
            Game.players[HOST].currentCenter = HORIZ_CENTER_MIN_PL;
        }
    }
}

/*
 * Thread to move a single ball
 */
void MoveBall(){
    int index;
    for(index = 0; index < MAX_NUM_OF_BALLS; index++){
        if(!(Game.balls[index].alive))
            break;
    }

    //Initialize Ball
    Game.balls[index].alive = 1;
    Game.balls[index].color = randomColorGen();
    Game.balls[index].currentCenterX = ((SystemTime & 0xF)+5)*10;
    Game.balls[index].currentCenterY = (((SystemTime & 0xF0)>>8 +5))*10;
    Game.balls[index].velocityX = (SystemTime % 2)? -1 : 1;
    Game.balls[index].velocityY = ((SystemTime>>1) % 2)? -1 : 1;

    while(1){
        //Player Collision Check
        Minkowski(&Game.balls[index], &Game.players[TOP]);
        Minkowski(&Game.balls[index], &Game.players[BOTTOM]);

        //Wall Collision Check
        WallCheck(&Game.balls[index]);

        //Update position w/ velocity
        Game.balls[index].currentCenterX += Game.balls[index].velocityX;
        Game.balls[index].currentCenterY += Game.balls[index].velocityY;

        sleep(35);
    }
}

/*
 * End of game for the host
 */
void EndOfGameHost(){
    G8RTOS_WaitSemaphore(&lcdMutex);
    G8RTOS_WaitSemaphore(&ledMutex);
    G8RTOS_WaitSemaphore(&wifiMutex);

    G8RTOS_KillAllButSelf();

    G8RTOS_InitSemaphore(&lcdMutex, 1);
    G8RTOS_InitSemaphore(&ledMutex, 1);
    G8RTOS_InitSemaphore(&wifiMutex, 1);

    if(Game.winner){
        LCD_Clear(PLAYER_RED);
    }else{
        LCD_Clear(PLAYER_BLUE);
    }

    LCD_Text(100, 100, "New Game? Tap Screen", LCD_WHITE);

    G8RTOS_AddAPeriodicEvent(&ScreenTap, 1, PORT4_IRQn);
    while(!restart);
    restart = false;

    InitBoardState();

    SendData(&Game, Game.player.IP_address, sizeof(GameState_t));

    G8RTOS_AddThread(&GenerateBall, 50, "GenBall");
    G8RTOS_AddThread(&DrawObjects, 50, "DrawObjs");
    G8RTOS_AddThread(&ReadJoystickHost, 30, "Joystick");
    G8RTOS_AddThread(&SendDataToClient, 25, "SendData");
    G8RTOS_AddThread(&ReceiveDataFromClient, 25, "RecData");
    G8RTOS_AddThread(&MoveLEDs, 100, "LEDs");
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    G8RTOS_KillSelf();
}

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread(){
    while(1);
}

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects(){
    PrevPlayer_t pastPlayerPositions[MAX_NUM_OF_PLAYERS];
    PrevBall_t pastBallPositions[MAX_NUM_OF_BALLS];

    while(1){
        for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++){
            if(pastPlayerPositions[i].Center != Game.players[i].currentCenter){
                UpdatePlayerOnScreen(&pastPlayerPositions[i], &Game.players[i]);
            }
        }

        for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
            if(((pastBallPositions[i].CenterX != Game.balls[i].currentCenterX) ||\
                    (pastBallPositions[i].CenterY != Game.balls[i].currentCenterY)) &&\
                    Game.balls[i].alive){
                UpdateBallOnScreen(&pastBallPositions[i], &Game.balls[i], Game.balls[i].color);
            }
        }

        sleep(20);
    }
}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs(){
    while(1){
        G8RTOS_WaitSemaphore(&ledMutex);
        ledModeSet(LED_RED, Game.LEDScores[TOP]);
        ledModeSet(LED_BLUE, Game.LEDScores[BOTTOM]);
        G8RTOS_SignalSemaphore(&ledMutex);
        sleep(100);
    }
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole(){
    P4->DIR &= ~(BIT4);
    P4->SEL0 &= ~(BIT4);
    P4->SEL1 &= ~(BIT4);

    P5->DIR &= ~(BIT4);
    P5->SEL0 &= ~(BIT4);
    P5->SEL1 &= ~(BIT4);

    while(1){
        if(!(P4->IN & BIT4)){
            return Client;
        }
        if((P5->IN & BIT4)){
            return Host;
        }
    }
}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player){
    if(player->position == TOP){
        G8RTOS_WaitSemaphore(&lcdMutex);
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2, \
                          TOP_PLAYER_CENTER_Y - PADDLE_WID_D2, TOP_PLAYER_CENTER_Y + PADDLE_WID_D2 , \
                          player->color);
        G8RTOS_SignalSemaphore(&lcdMutex);
    }
    else if(player->position == BOTTOM){
        G8RTOS_WaitSemaphore(&lcdMutex);
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2, \
                          BOTTOM_PLAYER_CENTER_Y - PADDLE_WID_D2, BOTTOM_PLAYER_CENTER_Y + PADDLE_WID_D2 , \
                          player->color);
        G8RTOS_SignalSemaphore(&lcdMutex);
    }
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer){
    int16_t dx = prevPlayerIn->Center - outPlayer->currentCenter;
    if(dx > 0){
        G8RTOS_WaitSemaphore(&lcdMutex);
        LCD_DrawRectangle(prevPlayerIn->Center - PADDLE_LEN_D2 , outPlayer->currentCenter - PADDLE_LEN_D2,
                          TOP_PLAYER_CENTER_Y - PADDLE_WID_D2, TOP_PLAYER_CENTER_Y + PADDLE_WID_D2 , LCD_BLACK);
        LCD_DrawRectangle(prevPlayerIn->Center + PADDLE_LEN_D2 , outPlayer->currentCenter + PADDLE_LEN_D2,
                          TOP_PLAYER_CENTER_Y - PADDLE_WID_D2, TOP_PLAYER_CENTER_Y + PADDLE_WID_D2 , outPlayer->color);
        G8RTOS_SignalSemaphore(&lcdMutex);
    }
    else if(dx < 0) {
        G8RTOS_WaitSemaphore(&lcdMutex);
        LCD_DrawRectangle(prevPlayerIn->Center + PADDLE_LEN_D2 , outPlayer->currentCenter + PADDLE_LEN_D2,
                          TOP_PLAYER_CENTER_Y - PADDLE_WID_D2, TOP_PLAYER_CENTER_Y + PADDLE_WID_D2 , LCD_BLACK);
        LCD_DrawRectangle(prevPlayerIn->Center - PADDLE_LEN_D2 , outPlayer->currentCenter - PADDLE_LEN_D2,
                          TOP_PLAYER_CENTER_Y - PADDLE_WID_D2, TOP_PLAYER_CENTER_Y + PADDLE_WID_D2 , outPlayer->color);
        G8RTOS_SignalSemaphore(&lcdMutex);
    }

    prevPlayerIn->Center = outPlayer->currentCenter;
}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor){
    G8RTOS_WaitSemaphore(&lcdMutex);
    LCD_DrawRectangle(previousBall->CenterX - BALL_SIZE_D2, previousBall->CenterX + BALL_SIZE_D2, \
                      previousBall->CenterY - BALL_SIZE_D2, previousBall->CenterY + BALL_SIZE_D2, LCD_BLACK);
    LCD_DrawRectangle(currentBall->currentCenterX - BALL_SIZE_D2, currentBall->currentCenterX + BALL_SIZE_D2,\
                      currentBall->currentCenterY - BALL_SIZE_D2, currentBall->currentCenterY + BALL_SIZE_D2, currentBall->color);
    G8RTOS_SignalSemaphore(&lcdMutex);

    previousBall->CenterX = currentBall->currentCenterX;
    previousBall->CenterY = currentBall->currentCenterY;
}

/*
 * Initializes and prints initial game state
 */
void InitBoardState(){
    //Store permanent info
    SpecificPlayerInfo_t tempPlayer = Game.player;
    uint8_t bottomOverall = Game.overallScores[BOTTOM];
    uint8_t topOverall = Game.overallScores[TOP];

    //Clear Game
    memset(&Game, 0, sizeof(GameState_t));

    //Copy over permanent info
    memcpy(&Game.player, &tempPlayer, sizeof(SpecificPlayerInfo_t));
    Game.overallScores[BOTTOM] = bottomOverall;
    Game.overallScores[TOP] = topOverall;

    //Initialize players
    Game.players[BOTTOM].color = PLAYER_BLUE;
    Game.players[BOTTOM].currentCenter = PADDLE_X_CENTER;
    Game.players[BOTTOM].position = BOTTOM;

    Game.players[TOP].color = PLAYER_RED;
    Game.players[TOP].currentCenter = PADDLE_X_CENTER;
    Game.players[TOP].position = TOP;

    //Initialize Arena
    LCD_Clear(LCD_WHITE);
    LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_Y, ARENA_MAX_Y, LCD_BLACK);
    DrawPlayer(&Game.players[BOTTOM]);
    DrawPlayer(&Game.players[TOP]);

    //Draw latest overall scores
    LCD_Text((uint8_t)10, (uint8_t)100, Game.overallScores[TOP] + '0', Game.players[TOP].color);
    LCD_Text((uint8_t)10, (uint8_t)120, Game.overallScores[BOTTOM] + '0', Game.players[BOTTOM].color);
}

/*********************************************** Public Functions *********************************************************************/

