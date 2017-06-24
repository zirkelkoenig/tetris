#include <stdlib.h>
#include <time.h>
#include "movement.h"
#include "static_data.h"
#include "dbg.h"

/* Checks whether the current field/piece state results in a collision. */
int check_collision(State *state)
{
	check(state, "argument \"state\" uninitialized");

	int i = 0;
	for (i = 0; i != 4; i++) {
		int x_check = state->cur_piece.x_pos[i];
		int y_check = state->cur_piece.y_pos[i];

		if (x_check < 0 || x_check > 9 || y_check < 0 || y_check > 22 ||
				state->field[x_check][y_check] != EMPTY) {
			return 1;
		}
	}
	return 0;

error:
	return -2;
}

int shift(State *state, int direction)
{
	check(state, "argument \"state\" uninitialized");

	if (direction == 0) {
		return 1;
	}

	Piece *piece = &state->cur_piece;
	int old_x_pos[] = {
		piece->x_pos[0],
		piece->x_pos[1],
		piece->x_pos[2],
		piece->x_pos[3]
	};

	int i = 0;
	for (i = 0; i != 4; i++) {
		piece->x_pos[i] += (direction > 0) ? 1 : (-1);
	}

	int rc = check_collision(state);
	check(rc >= 0, "\"check_collision\" returned an error");
	if (rc) {
		piece->x_pos[0] = old_x_pos[0];
		piece->x_pos[1] = old_x_pos[1];
		piece->x_pos[2] = old_x_pos[2];
		piece->x_pos[3] = old_x_pos[3];
		return 0;
	} else {
		return 1;
	}

error:
	if (rc < 0) {
		return rc;
	}
	return -2;
}

int rotate(State *state, int direction)
{
	check(state, "argument \"state\" uninitialized");

	if (direction == 0) {
		return 1;
	}

	Piece *piece = &state->cur_piece;
	int old_x_pos[] = {
		piece->x_pos[0],
		piece->x_pos[1],
		piece->x_pos[2],
		piece->x_pos[3]
	};
	int old_y_pos[] = {
		piece->y_pos[0],
		piece->y_pos[1],
		piece->y_pos[2],
		piece->y_pos[3]
	};
	int old_orientation = piece->orientation;

	// perform basic rotation
	if (direction < 0) {
		if (piece->orientation == 0) {
			piece->orientation = 3;
		} else {
			piece->orientation--;
		}

		int i = 0;
		for (i = 0; i != 4; i++) {
			int param = x_rotations[piece->color][piece->orientation][i];
			piece->x_pos[i] -= param;

			param = y_rotations[piece->color][piece->orientation][i];
			piece->y_pos[i] -= param;
		}
	} else {
		if (piece->orientation == 3) {
			piece->orientation = 0;
		} else {
			piece->orientation++;
		}

		int i = 0;
		for (i = 0; i != 4; i++) {
			int param = x_rotations[piece->color][old_orientation][i];
			piece->x_pos[i] += param;

			param = y_rotations[piece->color][old_orientation][i];
			piece->y_pos[i] += param;
		}
	}

	int rc = check_collision(state);
	check(rc >= 0, "\"check_collision\" returned an error");
	if (!rc) {
		return 1;
	}

	// wall kick special cases for I, L, J, T
	if (piece->color == RED) {
		goto fail;
	}

	int x_kick = -1;
	int y_kick = -1;
	switch (piece->color) {
	case ORANGE:
		if (piece->orientation == EAST) {
			x_kick = piece->x_pos[2];
			y_kick = piece->y_pos[2];
		}
		if (piece->orientation == WEST) {
			x_kick = piece->x_pos[1];
			y_kick = piece->y_pos[1];
		}
		break;

	case BLUE:
		if (piece->orientation == EAST) {
			x_kick = piece->x_pos[0];
			y_kick = piece->y_pos[0];
		}
		if (piece->orientation == WEST) {
			x_kick = piece->x_pos[3];
			y_kick = piece->y_pos[3];
		}
		break;

	case TURQUOISE:
		if (piece->orientation == EAST) {
			x_kick = piece->x_pos[1];
			y_kick = piece->y_pos[1];
		}
		if (piece->orientation == WEST) {
			x_kick = piece->x_pos[2];
			y_kick = piece->y_pos[2];
		}
		break;
	}
	if (x_kick != -1 && state->field[x_kick][y_kick] == EMPTY) {
		goto fail;
	}

	// try wall kicks
	rc = shift(state, 1);
	check(rc >= 0, "\"shift\" returned an error");
	if (rc == 1) {
		return 1;
	}

	rc = shift(state, -1);
	check(rc >= 0, "\"shift\" returned an error");
	if (rc == 1) {
		return 1;
	}

fail: // fallthrough
	piece->x_pos[0] = old_x_pos[0];
	piece->x_pos[1] = old_x_pos[1];
	piece->x_pos[2] = old_x_pos[2];
	piece->x_pos[3] = old_x_pos[3];
	piece->y_pos[0] = old_y_pos[0];
	piece->y_pos[1] = old_y_pos[1];
	piece->y_pos[2] = old_y_pos[2];
	piece->y_pos[3] = old_y_pos[3];
	piece->orientation = old_orientation;
	return 0;

error:
	if (rc < 0) {
		return rc;
	}
	return -2;
}

int drop(State *state, int speed)
{
	check(state, "argument \"state\" uninitialized");
	check(speed >= 0, "argument \"speed\" is less than 0");

	Piece *piece = &state->cur_piece;
	int lines_dropped = 0;
	int i = 0;

	int rc = check_collision(state);
	check(rc >= 0, "\"check_collision\" returned an error");

	while (!rc && lines_dropped != speed) {
		for (i = 0; i != 4; i++) {
			piece->y_pos[i]--;
		}
		lines_dropped++;

		rc = check_collision(state);
		check(rc >= 0, "\"check_collision\" returned an error");
	}

	if (!rc && lines_dropped == speed) {
		return 2;
	}

	for (i = 0; i != 4; i++) {
		piece->y_pos[i]++;
	}

	if (lines_dropped == 0) {
		return 0;
	} else {
		return 1;
	}

error:
	if (rc < 0) {
		return rc;
	}
	return -2;
}

int lock(State *state)
{
	check(state, "argument \"state\" uninitialized");

	Piece *piece = &state->cur_piece;
	int i = 0;
	for (i = 0; i != 4; i++) {
		state->field[piece->x_pos[i]][piece->y_pos[i]] = piece->color;
	}
	return 0;

error:
	return -2;
}

int clear(State *state)
{
	check(state, "argument \"state\" uninitialized");

	int count = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i != 23; i++) {
		int clear = 1;
		for (j = 0; j != 10; j++) {
			if (state->field[j][i] == EMPTY) {
				clear = 0;
				break;
			}
		}

		if (clear) {
			count++;
			for (j = 0; j != 10; j++) {
				state->field[j][i] = DESTROYED;
			}
		}
	}
	return count;

error:
	return -2;
}

int destroy(State *state)
{
	check(state, "argument \"state\" uninitialized");

	int count = 0;
	int i = 0;

	for (i = 0; i != 23; i++) {
		if (state->field[0][i] == DESTROYED) {
			int j = 0;
			int k = 0;

			for (k = i; k != 22; k++) {
				for (j = 0; j != 10; j++) {
					state->field[j][k] = state->field[j][k + 1];
				}
			}

			for (j = 0; j != 10; j++) {
				state->field[j][22] = EMPTY;
			}

			count++;
			i--;
		}
	}

	return count;

error:
	return -2;
}

int spawn(State *state)
{
	check(state, "argument \"state\" uninitialized");

	int next_color = -1;
	int tries = 0;
	int ok = 0;
	int i = 0;

	while (!ok && tries != 6) {
		next_color = rand() % 7;
		ok = 1;

		for (i = 0; i != 4; i++) {
			if (state->history[i] == next_color) {
				ok = 0;
				break;
			}
		}

		tries++;
	}

	for (i = 0; i != 3; i++) {
		state->history[i] = state->history[i + 1];
	}
	state->history[3] = next_color;

	state->cur_piece = init_pieces[next_color];

	int rc = check_collision(state);
	check(rc >= 0, "\"check_collision\" returned an error");
	return !rc;

error:
	if (rc < 0) {
		return rc;
	}
	return -2;
}

int init_field(State *state)
{
	check(state, "argument \"state\" uninitialized");

	srand(time(NULL));

	int first_color = rand() % 4;
	if (first_color == 3) {
		first_color = TURQUOISE;
	}

	state->history[0] = GREEN;
	state->history[1] = PINK;
	state->history[2] = GREEN;
	state->history[3] = PINK;

	state->cur_piece = init_pieces[first_color];

	int i = 0;
	int j = 0;
	for (i = 0; i != 23; i++) {
		for (j = 0; j != 10; j++) {
			state->field[j][i] = EMPTY;
		}
	}

	return 0;

error:
	return -2;
}