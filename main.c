#include "main.h"

#include <stdio.h>
#include <stdlib.h>

#include "gba.h"

enum gba_state
{
    START, //'loadup screen'
    START_TIMER,
    READY,
    READY_WAITING,
    PREPARE,
    PLAY,
};

vector direction_vector[4] = {0};

int main(void)
{
    //set display type to MODE 3, BG 2
    REG_DISPCNT = MODE3 | BG2_ENABLE;
    
    direction_vector[LEFT] = (vector){.x=-1,.y=0};
    direction_vector[RIGHT] = (vector){.x=1,.y=0};
    direction_vector[UP] = (vector){.x=0,.y=-1};
    direction_vector[DOWN] = (vector){.x=0,.y=1};

    // Save current and previous state of button input.
    u32 previousButtons = BUTTONS;
    u32 currentButtons = BUTTONS;

    snake player;
    collision_box food_box;

    enum direction current_move_direction = UP;
    int has_won = 0;

    collision_box *coll_boxes[COLLIDER_ARRAY_SIZE];
    int do_end_game = 0;
    int score = 0;

    int collider_size = 0;

    //cycle counter before moving to ready screen
    int start_cycles = 0;

    //count cycles before game starts. use this to reseed rand function
    int reseed = 0;

    // Load initial application state
    enum gba_state state = START;
    enum gba_state new_state = state;

    while (1)
    {
        currentButtons = BUTTONS; // Load the current state of the buttons

        if (KEY_JUST_PRESSED(BUTTON_SELECT, currentButtons, previousButtons)) {
            state = START;
        }
        switch (state)
        {
        case START:
            has_won = 0;
            score = -1;
            start_cycles = 0;
            reseed = 0;

            new_state = START_TIMER;
            break;
        case START_TIMER:
            if (++start_cycles >= 120 * 3) {
                new_state = READY;
            }
            break;
        case READY:
            new_state = READY_WAITING;
            break;
        case READY_WAITING:
            if (KEY_JUST_PRESSED(BUTTON_START, currentButtons, previousButtons)) {
                new_state = PREPARE;
            }
            reseed++;
            break;
        case PREPARE:
            //initialize/reset values before moving into PLAY state.
            has_won = 0;
            current_move_direction = UP;
            do_end_game = 0;
            score = 0;
            
            __qran_seed = reseed;

            player.size = 0;
            makeNewSnakePart(&player);
            updateFoodPosition(&food_box, player.body[0].coll_box.pos);
            new_state = PLAY;
            break;
        case PLAY:
            //handle inputs, collisions

            //movement input
            if (KEY_DOWN(BUTTON_LEFT, BUTTONS)) {
                current_move_direction = LEFT;
            } else if (KEY_DOWN(BUTTON_RIGHT, BUTTONS)) {
                current_move_direction = RIGHT;
            } else if (KEY_DOWN(BUTTON_UP, BUTTONS)) {
                current_move_direction = UP;
            } else if (KEY_DOWN(BUTTON_DOWN, BUTTONS)) {
                current_move_direction = DOWN;
            }

            collider_size = 0;
            coll_boxes[collider_size++] = &food_box; //store food collider
            food_box.old_pos = food_box.pos;

            //update snake body movement, store snake colliders
            for (int i = 0; i < player.size; i++) {
                snake_body *part = &player.body[i];

                coll_boxes[collider_size++] = &part->coll_box; //store coll box

                if (i == 0 && part->movement_direction != current_move_direction) { //snake head
                    part->movement_direction = current_move_direction;

                    if (player.size > 1) {
                        add_to_queue(&player.body[i + 1].move_queue, part->coll_box.pos, part->movement_direction);
                    }

                } else if (part->move_queue.size > 0) {
                    queue *this_queue = &player.body[i].move_queue;

                    vector move_to = this_queue->pos[this_queue->size - 1];
                    enum direction move_dir = this_queue->dir[this_queue->size - 1];
             
                    if (same_vector(move_to, part->coll_box.pos)) {
                        this_queue->size--;
                        part->movement_direction = move_dir;
                        
                        if (i < player.size - 1) {
                            add_to_queue(&player.body[i + 1].move_queue, move_to, move_dir);
                        }
                    }
                }

                vector movement_vector = multiply_vector(direction_vector[part->movement_direction], SPEED);
                update_collider_position(&part->coll_box, movement_vector);
            }

            for (int i = 0; i < player.size; i++) {
                collision_box box1 = player.body[i].coll_box;
                for (int j = 0; j < player.size && !do_end_game; j++) {
                    if (i == j) continue;
                    collision_box box2 = player.body[j].coll_box;
                    
                    if (does_overlap(box1, box2)) {
                        do_end_game = 1;
                        has_won = 0;
                    }
                }

                if (does_overlap(box1, food_box)) {

                    int old_size = player.size;
                    score++;
                    makeNewSnakePart(&player); //new snake body part

                    if (old_size == player.size) {
                        do_end_game = 1;
                        has_won = 1;
                    } else {
                        updateFoodPosition(&food_box, box1.pos);
                    }
                }

            }

            if (do_end_game) {
                new_state = READY;
            }
            break;
        }

        //draw switch
        waitForVBlank();
        switch(state) {
        case START_TIMER:
            break;
        case READY_WAITING:
            break;
        case START:
            drawFullScreenImageDMA(startup); //draw startup screen
            break;
        case READY:
            drawFullScreenImageDMA(ready); //draw waiting screen image
            
            char ready_str[30];
            u16 ready_color = BLACK;
            if (has_won) {
                sprintf(ready_str, "You won!");
                ready_color = BLUE;
            } else {
                sprintf(ready_str, "Score: %d", score);
            }

            if (score > -1 || has_won) {
                drawCenteredString((HEIGHT >> 1) + 20, (WIDTH >> 1) - 25, 50, 20, ready_str, ready_color);
            }
            break;
        case PREPARE:
            fillScreenDMA(BG_COLOR);
            break;
        case PLAY: ;
            char str_scr[30];
            sprintf(str_scr, "Score: %d", score);
            drawRectDMA(0, 0, 6*10, 8, BG_COLOR);
            drawString(0, 0, str_scr, BLACK);

            for (int i = 0; i < collider_size; i++) {

                collision_box *box = coll_boxes[i];

                if (same_vector(box->old_pos, box->pos) && !box->force_draw) continue;

                if (box->force_draw) {
                    box->force_draw = 0;
                }
                
                collision_box old_box_draw = generate_box(*box, box->old_pos);

                if (!offscreen_box(old_box_draw))
                    drawRectDMA(old_box_draw.pos.y, old_box_draw.pos.x, old_box_draw.width, old_box_draw.height, BG_COLOR); //set background to play background

                if (!offscreen_box(*box)) {
                    if (box->mask == SNAKE_BODY_MASK) {
                        drawRectDMA(box->pos.y, box->pos.x, box->width, box->height, BLUE);
                    } else {
                        drawImageDMA(box->pos.y, box->pos.x, box->width, box->height, food);
                    }
                }
            }
            break;
        }

        state = new_state;
        previousButtons = currentButtons; // Store the current state of the buttons
    }
    return 0;
}

void makeNewSnakePart(snake *player) {
    if (player->size >= MAX_SIZE) return;

    snake_body *new_tail = &player->body[player->size++];

    new_tail->coll_box = (collision_box){
        .width = SNAKE_PART_SIZE,
        .height = SNAKE_PART_SIZE,
        .mask = SNAKE_BODY_MASK,
        .force_draw = 1,
    };

    new_tail->move_queue = (queue){.size=0};

    if (player->size == 1) {
        new_tail->coll_box.pos = (vector){
            .x = WIDTH >> 1,
            .y = HEIGHT >> 1,
        };
        new_tail->movement_direction = UP;
    } else {
        snake_body old_tail = player->body[player->size - 2];

        vector spawn_vector = multiply_vector(direction_vector[old_tail.movement_direction], -1 * ((SNAKE_PART_SIZE << 1) + 1));
        
        new_tail->coll_box.pos = old_tail.coll_box.pos;
        update_collider_position(&new_tail->coll_box, spawn_vector);
        new_tail->coll_box.old_pos = new_tail->coll_box.pos;

        new_tail->movement_direction = old_tail.movement_direction;
    }
}

void updateFoodPosition(collision_box *food_box, vector player_position) {

    food_box->height = FOOD_HEIGHT;
    food_box->width = FOOD_WIDTH;
    food_box->mask = FOOD_MASK;
    food_box->force_draw = 1;

    static u8 coin = 0;
    
    coin = !coin;

    vector new_vec;

    if (randint(0,100) > 50 && coin) {
        new_vec.x = randint(player_position.x + RADIUS_NO_SPAWN, WIDTH - 1);
    } else {
        new_vec.x = randint(0, player_position.x - RADIUS_NO_SPAWN);
    }

    if (randint(0,100) < 50 || coin) {
        new_vec.y = randint(player_position.y + RADIUS_NO_SPAWN, HEIGHT - 1);
    } else {
        new_vec.y = randint(0, player_position.y - RADIUS_NO_SPAWN);
    }
    
    new_vec = bound_vector(new_vec,food_box->height, food_box->width);

    food_box->pos = new_vec;
}

vector multiply_vector(vector a, int b) {
    return (vector){
        .x = a.x * b,
        .y = a.y * b,
    };
}

vector add_vector(vector a, vector b) {
    return (vector){
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

int does_overlap(collision_box b1, collision_box b2) {
    vector b1_br, b2_br;
    b1_br = add_vector(b1.pos, (vector){.x=b1.width,.y=b1.height});
    b2_br = add_vector(b2.pos, (vector){.x=b2.width,.y=b2.height});

    if (b1.pos.x < b2_br.x &&
            b1_br.x > b2.pos.x &&
            b1.pos.y < b2_br.y &&
            b1_br.y > b2.pos.y) {
        return 1;
    }
    return 0;
}

int same_vector(vector a, vector b) {
    if (a.x == b.x && a.y == b.y) {
        return 1;
    }
    return 0;
}

int within_dist(vector a, vector b, int dist) {
    int sqr = ((b.x - a.x)^2) + ((b.y - a.y)^2);

    if (sqr - (dist^2) <= 0) {
        return 1;
    }
    return 0;
}

int offscreen_box(collision_box b) {
    if (b.pos.x < 0 || b.pos.y < 0 || b.pos.x + b.width > WIDTH || b.pos.y + b.height > HEIGHT) {
        return 1;
    }
    return 0;
}

void update_collider_position(collision_box *box, vector diff) {
    vector preview_vec = bound_vector(add_vector(box->pos, diff), box->width, box->height);

    box->old_pos = box->pos; //for drawing purposes
    box->pos = preview_vec;
}

vector bound_vector(vector a, u16 width, u16 height) {
    if (a.x < 0) {
        a.x = 0;
    } else if (a.x + width >= WIDTH) {
        a.x = WIDTH - width - 1;
    }

    if (a.y < 0) {
        a.y = 0;
    } else if (a.y + height >= HEIGHT) {
        a.y = HEIGHT - height - 1;
    }

    return a;
}

collision_box generate_box(collision_box base, vector pos) {
    return (collision_box){
        .force_draw=0,
        .height=base.height,
        .width=base.width,
        .pos=pos,
        .old_pos=pos,
        .mask=base.mask,
    };
}

void add_to_queue(queue *queue, vector vec, enum direction dir) {
    for (int i = queue->size++; i > 0; i--) {
        queue->pos[i] = queue->pos[i - 1];
        queue->dir[i] = queue->dir[i - 1];
    }
    queue->pos[0] = vec;
    queue->dir[0] = dir;
}
