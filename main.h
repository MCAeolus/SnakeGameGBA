#ifndef MAIN_H
#define MAIN_H

#include "gba.h"

//image files
#include "images/startup.h"
#include "images/ready.h"
#include "images/food.h"
// TODO: Create any necessary structs

/*
* For example, for a Snake game, one could be:
*
* struct snake {
*   int heading;
*   int length;
*   int row;
*   int col;
* };
*
* Example of a struct to hold state machine data:
*
* struct state {
*   int currentState;
*   int nextState;
* };
*
*/
#define SPEED 1
#define SPEED_MOD 1

#define MAX_SIZE 20
#define SNAKE_PART_SIZE 10

#define COLLIDER_ARRAY_SIZE (MAX_SIZE + 1)

#define FOOD_MASK 1
#define SNAKE_BODY_MASK 2

#define RADIUS_NO_SPAWN 30
#define BG_COLOR GREEN

enum direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
};

typedef struct vector vector;
typedef struct collision_box collision_box;
typedef struct snake_body snake_body;
typedef struct snake snake;
typedef struct queue queue;

struct vector {
    int x, y;
};

struct collision_box {
    vector pos;
    vector old_pos;
    int width;
    int height;
    u8 mask; //box differentation
    u8 force_draw;
};

struct queue {
    u8 size;
    vector pos[16];
    enum direction dir[16];
};

struct snake_body {
    collision_box coll_box;
    enum direction movement_direction;
    queue move_queue;
    vector move_to_position;
};

struct snake {
    u16 size;
    snake_body body[MAX_SIZE];
};

extern vector direction_vector[4];

void makeNewSnakePart(snake *player);
void updateFoodPosition(collision_box *food_box, vector player_position);
vector add_vector(vector a, vector b);
vector multiply_vector(vector a, int b);
int does_overlap(collision_box b1, collision_box b2);
int same_vector(vector a, vector b);
int within_dist(vector a, vector b, int dist);
int offscreen_box(collision_box b);
void update_collider_position(collision_box *box, vector rel);
collision_box generate_box(collision_box base, vector pos);
vector bound_vector(vector a, u16 width, u16 height);
void add_to_queue(queue *queue, vector vec, enum direction dir);

#endif
