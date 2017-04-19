/******************************************************************************
 * Copyright (C) 2016  Renan S. Silva                                         *
 *                                                                            *
 * This software is provided 'as-is', without any express or implied          *
 * warranty. In no event will the authors be held liable for any damages      *
 * arising from the use of this software.                                     *
 *                                                                            *
 * Permission is granted to anyone to use this software for any purpose,      *
 * including commercial applications, and to alter it and redistribute it     *
 * freely, subject to the following restrictions:                             *
 *                                                                            *
 * 1. The origin of this software must not be misrepresented; you must not    *
 *    claim that you wrote the original software. If you use this software    *
 *    in a product, an acknowledgment in the product documentation would be   *
 *    appreciated but is not required.                                        *
 * 2. Altered source versions must be plainly marked as such, and must not be *
 *    misrepresented as being the original software.                          *
 * 3. This notice may not be removed or altered from any source distribution. *
 ******************************************************************************/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>

#include "other_window.h"
#include "trainer.h"
#include "tetris.h"
#include "types.h"
#include "ia.h"

static _brain brain;

void normalizer() {
    for (int i = 0; i < N_FUNCTION; ++i) {
        if ( brain.candidate.cost[i] < brain.candidate.min[i] ) {
            /*printf("new min %f %f\n", brain.candidate.cost[i], brain.candidate.min[i] );*/
            brain.candidate.min[i] = brain.candidate.cost[i];
        }

        if ( brain.candidate.cost[i] > brain.candidate.max[i] ) {
            /*printf("new max %f %f\n", brain.candidate.cost[i], brain.candidate.max[i] );*/
            brain.candidate.max[i] = brain.candidate.cost[i];
        }

        if ( brain.candidate.min[i] != brain.candidate.max[i] ) {
            double cost = brain.candidate.cost[i] - brain.candidate.min[i];
            cost /= brain.candidate.max[i] - brain.candidate.min[i];

            brain.candidate.cost[i] = cost;
        } else {
            brain.candidate.cost[i] = 0;
        }
    }
}

void scaler() {
    for (int i = 0; i < N_FUNCTION; ++i) {
        brain.candidate.cost[i] *= brain.candidate.weight[ (i * GEN_P_FUNCTION) + 2 ];
    }
}

void evaluate_cost() {
    // Tests if any row was cleaned
    get_brain_pointer()->round_has_cleaned_lines = cleaned_any_row() ? 1 : 0;

    brain.candidate.cost[2]  = complete_rows();
    brain.candidate.cost[5]  = covered_cells_after_clear();

    if ( cleaned_any_row() ) {
        clear_lines();
    }

    brain.candidate.cost[0]  = aggregate_height();
    brain.candidate.cost[1]  = covered_cells();
    /*brain.candidate.cost[2]  = complete_rows();*/
    brain.candidate.cost[3]  = surface_variance();
    brain.candidate.cost[4]  = well_cells();
    /*brain.candidate.cost[5]  = covered_cells_after_clear();*/
    brain.candidate.cost[6]  = lock_heigth();
    brain.candidate.cost[7]  = burried_cells();
    brain.candidate.cost[8]  = highest_cell();
    brain.candidate.cost[9]  = height_delta();
    brain.candidate.cost[10] = vertical_roughness();
    brain.candidate.cost[11] = horizontal_roughness();
    brain.candidate.cost[12] = vertical_roughness_w();
    brain.candidate.cost[13] = horizontal_roughness_w();

    normalizer();

    scaler();
}

void initialize_pop (){
    for (int j = 0; j < N_GENES; ++j) {
#ifdef TRAIN
        brain.active.weight[j] = ( drand48() * 2.0 - 1.0 ) * 7.5;
        brain.candidate.weight[j] = 0.0;
#else
        brain.active.weight[j] = ia[j];
#endif

        brain.active.cost[j]   = 0;
        brain.active.fitness   = 0;
        brain.active.max[j]    =-DBL_MAX;
        brain.active.min[j]    = DBL_MAX;
        brain.active.worst     = 0;
        brain.candidate.cost[j]   = 0;
        brain.candidate.fitness   = 0;
        brain.candidate.max[j]    =-DBL_MAX;
        brain.candidate.min[j]    = DBL_MAX;
        brain.candidate.worst     = 0;
    }

    brain.active.fitness = 0;
    brain.best.fitness = 0;
    brain.candidate.fitness = 0;
}

double get_cost(){
    _obj_costs* obj = &brain.candidate;

    double result = 0;

    for (int i = 0; i < N_FUNCTION; i ++ ) {
        result += obj->cost[i];
    }

    return result;
}

_brain* get_brain_pointer() {
    return &brain;
}

// Run by finished_evaluating_individual, that is called by game_over_hook
void evolutionary_step(){

}

// Run once at the start of the emulator
void boot_brain() {
    brain.current             = 0;
    brain.max_runs            = 7;
    brain.runs                = 0;
    brain.worst_lines_cleared = 0;
    brain.most_lines_cleared  = 0;
    brain.rng                 = 1;

    brain.t_start             = 1000.0;
    brain.t_end               = 0.0;
    brain.t_current           = brain.t_start;

    brain.max_iter            = 100;
    brain.iter                = 0;

    initialize_pop();
}

// Called by new_piece_on_screen_hook and game_over_hook
void update_fitness() {
    int a = get_cpu_pointer()->mem_controller.memory[0x9951];
    int b = get_cpu_pointer()->mem_controller.memory[0x9950];
    int c = get_cpu_pointer()->mem_controller.memory[0x994f];
    int d = get_cpu_pointer()->mem_controller.memory[0x994e];

    a = a == 0x2f ? 0 : a;
    b = b == 0x2f ? 0 : b;
    c = c == 0x2f ? 0 : c;
    d = d == 0x2f ? 0 : d;

    int best = a + b * 10 + c * 100 + d * 1000;

    /*printf("%d\n", best);*/

    brain.active.fitness = best;

    brain.most_lines_cleared = best > brain.most_lines_cleared ?
                               best : brain.most_lines_cleared ;
}

// Called by game_over_hook
void finished_evaluating_individual () {

    if ( brain.candidate.fitness > brain.active.fitness ) {
        brain.active = brain.candidate;
    }

    new_solution = proposalfunc(old_solution)
    new_energy = energyfunc(new_solution)
    # Use a min here as you could get a "probability" > 1
    alpha = min(1, np.exp((old_energy - new_energy)/T))
    double alpha = exp(brain.active.fitness 
    if ((new_energy < old_energy) or (np.random.uniform() < alpha)):

    brain.t_current = brain.t_start - ((brain.t_start - brain.t_end) / brain.max_iter) * brain.iter;

    brain.iter += 1;

    brain.candidate = brain.active;

    for (int i = 0; i < N_GENES; ++i) {
        if ( drand48() < 0.1 ) {
            brain.candidate.weight[i] =+ (drand48() * 2.0) - 1.0;
        }
    }

    evolutionary_step();
}
