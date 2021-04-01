#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

#include <iostream>
#include <vector>

struct GameState {
    Entity* player;
    Entity* platform;
    Entity* enemy1;
    Entity* enemy2;
    Entity* enemy3;
};

GameState state;
int PLATFORM_COUNT = 13;
//int ENEMY_COUNT = 3;
GLuint fontTextureID;
bool gameWon = false;
bool gameOver = false;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    return textureID;
}

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text,
    float size, float spacing, glm::vec3 position)
{
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;

    std::vector<float> vertices;
    std::vector<float> texCoords;

    for (int i = 0; i < text.size(); i++) {

        int index = (int)text[i];
        float offset = (size + spacing) * i;
        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;
        vertices.insert(vertices.end(), {
        offset + (-0.5f * size), 0.5f * size,
        offset + (-0.5f * size), -0.5f * size,
        offset + (0.5f * size), 0.5f * size,
        offset + (0.5f * size), -0.5f * size,
        offset + (0.5f * size), 0.5f * size,
        offset + (-0.5f * size), -0.5f * size,
            });
        texCoords.insert(texCoords.end(), {
            u, v,
            u, v + height,
            u + width, v,
            u + width, v + height,
            u + width, v,
            u, v + height,
            });

    } // end of for loop

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);

    glUseProgram(program->programID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}


void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Thy-Lan Gale - Project 3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glUseProgram(program.programID);

    glClearColor(0.529f, 0.808f, 0.922f, 0.0f); //background color
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // Initialize Game Objects
    fontTextureID = LoadTexture("font1.png");

    // Initialize player
    state.player = new Entity();
    state.player->entityType = PLAYER;
    state.player->position = glm::vec3(-4.5f, -2.25f, 0.0f);
    state.player->movement = glm::vec3(0);
    state.player->acceleration = glm::vec3(1.0f, -9.81f, 0);
    state.player->speed = 1.5f;
    state.player->textureID = LoadTexture("player.png");
    
    state.player->animRight = new int[4] {3, 7, 11, 15};
    state.player->animLeft = new int[4]{ 1, 5, 9, 13 };
    state.player->animUp = new int[4]{ 2, 6, 10, 14 };
    state.player->animDown = new int[4]{ 0, 4, 8, 12 };

    state.player->animIndices = state.player->animRight;
    state.player->animFrames = 4;
    state.player->animIndex = 0;
    state.player->animTime = 0;
    state.player->animCols = 4;
    state.player->animRows = 4;

    state.player->height = 0.8f;
    state.player->width = 0.7f;
    state.player->jumpPower = 6.0f;


    // Initialize platform
    state.platform = new Entity[PLATFORM_COUNT];
    GLuint platformTextureID = LoadTexture("tileset.png");
    
    for (int i = 0; i < PLATFORM_COUNT - 2; i++) {
        state.platform[i].textureID = platformTextureID;
        state.platform[i].position = glm::vec3(-4.5 + i, -3.25f, 0.0f);
    }
    state.platform[10].textureID = platformTextureID;
    state.platform[10].position = glm::vec3(-2.0f, -2.25f, 0.0f);

    state.platform[11].textureID = platformTextureID;
    state.platform[11].position = glm::vec3(2.0f, -2.25f, 0.0f);

    state.platform[12].textureID = platformTextureID;
    state.platform[12].position = glm::vec3(3.0f, -2.25f, 0.0f);

    //state.platform[13].textureID = platformTextureID;
    //state.platform[13].position = glm::vec3(4.0f, -2.25f, 0.0f);


    for (int i = 0; i < PLATFORM_COUNT; i++) {
        state.platform[i].entityType = PLATFORM;
        state.platform[i].Update(0, NULL, NULL, 0);
    }
    /*
    //Initialize Enemies
    state.enemy = new Entity[ENEMY_COUNT];
    GLuint enemyTextureID = LoadTexture("enemy.png"); //add enemy

    for (int i = 0; i < ENEMY_COUNT; i++) {
        state.enemy->entityType = ENEMY;
        state.enemy[i].textureID = enemyTextureID;
        state.enemy[i].acceleration = glm::vec3(0, -9.81f, 0);
        state.enemy[i].height = 0.65f;
        state.enemy[i].width = 0.8f;
        state.enemy[i].speed = 1.0f;
    }
    state.enemy[0].position = glm::vec3(1.0f, -1.0f, 0);
    state.enemy[0].movement = glm::vec3(-1.0f, 0, 0);
    state.enemy[0].aiType = WALKER;
    state.enemy[0].aiState = WALKING;

    state.enemy[1].position = glm::vec3(4.0f, -1.0f, 0);
    state.enemy[1].aiType = JUMPER;
    state.enemy[1].aiState = JUMPING;
    state.enemy[1].jumpPower = 3.0f;

    //state.enemy[2].position = glm::vec3(2.0f, 0, 0);
    //state.enemy[2].aiType = WAITANDGO;
    //state.enemy[2].aiState = IDLE;
    */
    state.enemy1 = new Entity();
    GLuint enemyTextureID = LoadTexture("enemy.png");
    state.enemy1->entityType = ENEMY;
    state.enemy1->textureID = enemyTextureID;
    state.enemy1->acceleration = glm::vec3(0, -9.81f, 0);
    state.enemy1->height = 0.65f;
    state.enemy1->width = 0.8f;
    state.enemy1->speed = 1.0f;
    state.enemy1->position = glm::vec3(1.0f, -1.0f, 0);
    state.enemy1->movement = glm::vec3(-1.0f, 0, 0);
    state.enemy1->aiType = WALKER;
    state.enemy1->aiState = WALKING;

    state.enemy2 = new Entity();
    //GLuint enemyTextureID = LoadTexture("enemy.png");
    state.enemy2->entityType = ENEMY;
    state.enemy2->textureID = enemyTextureID;
    state.enemy2->acceleration = glm::vec3(0, -9.81f, 0);
    state.enemy2->height = 0.65f;
    state.enemy2->width = 0.8f;
    state.enemy2->speed = 1.0f;
    state.enemy2->position = glm::vec3(3.0f, -1.0f, 0);
    state.enemy2->aiType = JUMPER;
    state.enemy2->aiState = JUMPING;
    state.enemy2->jumpPower = 3.0f;

    state.enemy3 = new Entity();
    //GLuint enemyTextureID = LoadTexture("enemy.png");
    state.enemy3->entityType = ENEMY;
    state.enemy3->textureID = enemyTextureID;
    state.enemy3->acceleration = glm::vec3(0, -9.81f, 0);
    state.enemy3->height = 0.65f;
    state.enemy3->width = 0.8f;
    state.enemy3->speed = 1.0f;
    state.enemy3->position = glm::vec3(2.0f, -1.0f, 0);
    state.enemy3->aiType = WAITANDGO;
    state.enemy3->aiState = IDLE;
    
}

void ProcessInput() {

    state.player->movement = glm::vec3(0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:
                // Move the player left
                break;

            case SDLK_RIGHT:
                // Move the player right
                break;

            case SDLK_SPACE:
                // Some sort of action
                if (state.player->collidedBottom) {
                    state.player->jump = true;
                }
                break;
            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
        if (!gameOver && !gameWon) {
            state.player->movement.x = -1.0f;
            state.player->animIndices = state.player->animLeft;
        }

    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
        if (!gameOver && !gameWon) {
            state.player->movement.x = 1.0f;
            state.player->animIndices = state.player->animRight;
        }
    }


    if (glm::length(state.player->movement) > 1.0f) {
        state.player->movement = glm::normalize(state.player->movement);
    }

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;
void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    deltaTime += accumulator;
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        state.player->Update(FIXED_TIMESTEP, state.player, state.platform, PLATFORM_COUNT);
        state.enemy1->Update(FIXED_TIMESTEP, state.player, state.platform, PLATFORM_COUNT);
        state.enemy2->Update(FIXED_TIMESTEP, state.player, state.platform, PLATFORM_COUNT);
        state.enemy3->Update(FIXED_TIMESTEP, state.player, state.platform, PLATFORM_COUNT);
        //for (int i = 0; i < ENEMY_COUNT; i++) {
            //state.enemy[i].Update(FIXED_TIMESTEP, state.player, state.platform, PLATFORM_COUNT);
        //}
        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;


    //Checking for Collisions for winning and losing
    
    state.enemy1->CheckCollisionsY(state.player, 1);
    state.enemy1->CheckCollisionsX(state.player, 1);

    state.enemy2->CheckCollisionsY(state.player, 1);
    //state.enemy2->CheckCollisionsX(state.player, 1);

    state.enemy3->CheckCollisionsY(state.player, 1);
    //state.enemy3->CheckCollisionsX(state.player, 1);
    
    if (state.enemy1->lastCollision == state.player && state.enemy1->collidedTop) {
        state.enemy1->isActive = false;
    }
    
    else if (state.enemy1->lastCollision == state.player && state.enemy1->isActive == true) gameOver = true;

    if (state.enemy2->lastCollision == state.player && state.enemy2->collidedTop) {
        state.enemy2->isActive = false;
    }

    else if (state.enemy2->lastCollision == state.player && state.enemy2->isActive == true && (state.enemy2->collidedLeft || state.enemy2->collidedRight)) gameOver = true;

    if (state.enemy3->lastCollision == state.player && state.enemy3->collidedTop) {
        state.enemy3->isActive = false;
    }

    else if (state.enemy3->lastCollision == state.player && state.enemy3->isActive == true && (state.enemy3->collidedLeft || state.enemy3->collidedRight)) gameOver = true;


    if (state.enemy1->isActive == false && state.enemy2->isActive == false && state.enemy3->isActive == false) {
        gameWon = true;
    }
    
}


void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    state.player->Render(&program);
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        state.platform[i].Render(&program);
    }
    state.enemy1->Render(&program);
    state.enemy2->Render(&program);
    state.enemy3->Render(&program);
    //for (int i = 0; i < ENEMY_COUNT; i++) {
        //state.enemy[i].Render(&program);
    //}

    //Checking for Collisions for winning and losing
    /*
    state.enemy1->CheckCollisionsY(state.player, 1);
    state.enemy1->CheckCollisionsX(state.player, 1);
    
    state.player->CheckCollisionsY(state.enemy2, 1);
    state.player->CheckCollisionsX(state.enemy2, 1);
    
    state.player->CheckCollisionsY(state.enemy3, 1);
    state.player->CheckCollisionsX(state.enemy3, 1);

    if (state.enemy1->collidedTop && state.enemy1->lastCollision == state.player) state.enemy1->isActive = false;
    */
    /*
    if (state.player->collidedBottom && state.player->lastCollision == state.enemy1) {
        state.enemy1->isActive = false;
    }
    else if (state.player->lastCollision == state.enemy1 &&(state.player->collidedLeft || state.player->collidedRight)) gameOver = true;
    if (state.player->collidedBottom && state.player->lastCollision == state.enemy2) {
        state.enemy2->isActive = false;
    }
    else if (state.player->lastCollision == state.enemy2 && (state.player->collidedLeft || state.player->collidedRight)) gameOver = true;
    if (state.player->collidedBottom && state.player->lastCollision == state.enemy3) {
        state.enemy3->isActive = false;
    }
    else if (state.player->lastCollision == state.enemy3 && (state.player->collidedLeft || state.player->collidedRight)) gameOver = true;
    

    if (state.enemy1->isActive == false && state.enemy2->isActive == false && state.enemy3->isActive == false) {
        gameWon = true;
    }
    */


    


    //Print outcome
    if (gameWon) { //print mission successful
        DrawText(&program, fontTextureID, "You Won!", 0.5f, -0.25f, glm::vec3(-1.0f, 3.3, 0));
    }
    else if (gameOver) { //print mission failed
        DrawText(&program, fontTextureID, "Game Over", 0.5f, -0.25f, glm::vec3(-1.5f, 3.3, 0));
    }


    SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();

    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}