#include "ripes_system.h"
#include <stdlib.h>

// Ripes I/O Macros
volatile unsigned int *led_base = (volatile unsigned int *)LED_MATRIX_0_BASE;
volatile unsigned int *switch_base = (volatile unsigned int *)SWITCHES_0_BASE;
volatile unsigned int *d_pad_up = (volatile unsigned int *)D_PAD_0_UP;
volatile unsigned int *d_pad_down = (volatile unsigned int *)D_PAD_0_DOWN;
volatile unsigned int *d_pad_left = (volatile unsigned int *)D_PAD_0_LEFT;
volatile unsigned int *d_pad_right = (volatile unsigned int *)D_PAD_0_RIGHT;

// Colors configuration
#define SNAKE_COLOR 0xFF0000      // Green
#define FOOD_COLOR 0x00FF00       // Red
#define BACKGROUND_COLOR 0xFFFFFF // White
#define BORDER_COLOR 0x000000     // Black

// Game configuration
#define MAX_SNAKE_LENGTH 50
#define PIXEL_SIZE 2
#define WAIT_DELAY 25  
#define START_X 10
#define START_Y 10

typedef enum { GAME_OVER, RUNNING, PAUSED } GameState;
typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

typedef struct {
    unsigned int x, y;
} Position;

typedef struct {
    Position segments[MAX_SNAKE_LENGTH];
    int length;
    Direction direction;
} Snake;

typedef struct {
    Position position;
} Food;

void initSnake(Snake *snake);
void drawSnake(const Snake *snake);
void eraseSnakeTail(const Snake *snake);
void moveSnake(Snake *snake);
void generateFood(Snake *snake, Food *food);
void drawFood(const Food *food);
int checkWallCollision(const Snake *snake);
int checkSelfCollision(const Snake *snake);
int checkFoodCollision(const Snake *snake, const Food *food);
void clearBoard();
void delay(int milliseconds);

void main() {
    Snake snake;
    Food food;
    GameState game_state = GAME_OVER;
    int score = 0;
    int previous_switch_state = *switch_base & 0x01;

    initSnake(&snake);
    generateFood(&snake, &food);
    clearBoard();
    drawSnake(&snake);
    drawFood(&food);

    while (1) {
        int current_switch_state = *switch_base & 0x03;

        if (current_switch_state == 0x01 && game_state == GAME_OVER) {
            game_state = RUNNING;  
            score = 0;
            initSnake(&snake);
            generateFood(&snake, &food);
            clearBoard();
            drawSnake(&snake);
            drawFood(&food);
        } 
        else if (current_switch_state == 0x02) {
            game_state = GAME_OVER;
            clearBoard();
            initSnake(&snake);
            generateFood(&snake, &food);
            drawSnake(&snake);
            drawFood(&food);
            score = 0;
            delay(200);
        }

        if (game_state == GAME_OVER || game_state == PAUSED) {
            delay(100);
            continue;
        }

        if (*d_pad_up && snake.direction != DOWN) {
            snake.direction = UP;
        } else if (*d_pad_down && snake.direction != UP) {
            snake.direction = DOWN;
        } else if (*d_pad_left && snake.direction != RIGHT) {
            snake.direction = LEFT;
        } else if (*d_pad_right && snake.direction != LEFT) {
            snake.direction = RIGHT;
        }

        eraseSnakeTail(&snake);
        moveSnake(&snake);

        if (checkWallCollision(&snake) || checkSelfCollision(&snake)) {
            game_state = GAME_OVER;
            continue;
        }

        if (checkFoodCollision(&snake, &food)) {
            if (snake.length < MAX_SNAKE_LENGTH) {
                snake.length++;
            }
            score++;
            generateFood(&snake, &food);
        }

        drawSnake(&snake);
        drawFood(&food);

        delay(WAIT_DELAY);
    }
}


void initSnake(Snake *snake) {
    snake->length = 3;
    snake->direction = RIGHT;
    for (int i = 0; i < snake->length; i++) {
        snake->segments[i].x = START_X - i * PIXEL_SIZE;
        snake->segments[i].y = START_Y;
    }
}

void drawSnake(const Snake *snake) {
    for (int i = 0; i < snake->length; i++) {
        for (int px = 0; px < PIXEL_SIZE; px++) {
            for (int py = 0; py < PIXEL_SIZE; py++) {
                *(led_base + (snake->segments[i].y + py) * LED_MATRIX_0_WIDTH + (snake->segments[i].x + px)) = SNAKE_COLOR;
            }
        }
    }
}

void eraseSnakeTail(const Snake *snake) {
    Position tail = snake->segments[snake->length - 1];
    for (int px = 0; px < PIXEL_SIZE; px++) {
        for (int py = 0; py < PIXEL_SIZE; py++) {
            *(led_base + (tail.y + py) * LED_MATRIX_0_WIDTH + (tail.x + px)) = BACKGROUND_COLOR;
        }
    }
}

void moveSnake(Snake *snake) {
    for (int i = snake->length - 1; i > 0; i--) {
        snake->segments[i] = snake->segments[i - 1];
    }
    Position *head = &snake->segments[0];
    switch (snake->direction) {
        case UP: head->y -= PIXEL_SIZE; break;
        case DOWN: head->y += PIXEL_SIZE; break;
        case LEFT: head->x -= PIXEL_SIZE; break;
        case RIGHT: head->x += PIXEL_SIZE; break;
    }
}

void generateFood(Snake *snake, Food *food) {
    int valid = 0;
    while (!valid) {
        food->position.x = (rand() % (LED_MATRIX_0_WIDTH / PIXEL_SIZE)) * PIXEL_SIZE;
        food->position.y = (rand() % (LED_MATRIX_0_HEIGHT / PIXEL_SIZE)) * PIXEL_SIZE;
        valid = 1;
        for (int i = 0; i < snake->length; i++) {
            if (food->position.x == snake->segments[i].x && food->position.y == snake->segments[i].y) {
                valid = 0;
                break;
            }
        }
    }
}

void drawFood(const Food *food) {
    for (int px = 0; px < PIXEL_SIZE; px++) {
        for (int py = 0; py < PIXEL_SIZE; py++) {
            *(led_base + (food->position.y + py) * LED_MATRIX_0_WIDTH + (food->position.x + px)) = FOOD_COLOR;
        }
    }
}

int checkWallCollision(const Snake *snake) {
    Position head = snake->segments[0];
    return (head.x < 0 || head.x >= LED_MATRIX_0_WIDTH || head.y < 0 || head.y >= LED_MATRIX_0_HEIGHT);
}

int checkSelfCollision(const Snake *snake) {
    Position head = snake->segments[0];
    for (int i = 1; i < snake->length; i++) {
        if (head.x == snake->segments[i].x && head.y == snake->segments[i].y) {
            return 1;
        }
    }
    return 0;
}

int checkFoodCollision(const Snake *snake, const Food *food) {
    Position head = snake->segments[0];
    return (head.x == food->position.x && head.y == food->position.y);
}

void clearBoard() {
    for (int i = 0; i < LED_MATRIX_0_WIDTH; i++) {
        for (int j = 0; j < LED_MATRIX_0_HEIGHT; j++) {
            *(led_base + j * LED_MATRIX_0_WIDTH + i) = BACKGROUND_COLOR;
        }
    }
}

void delay(int milliseconds) {
    volatile unsigned int counter;
    for (volatile int i = 0; i < milliseconds; i++) {
        for (counter = 0; counter < 1000; counter++) {
        }
    }
}
