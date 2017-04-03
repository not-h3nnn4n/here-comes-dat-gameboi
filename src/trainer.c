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

#include "other_window.h"
#include "trainer.h"
#include "tetris.h"
#include "types.h"
#include "ia.h"

static _brain brain;

void evaluate_cost() {
    brain.population[brain.current].cost[0]  = aggregate_height();
    brain.population[brain.current].cost[1]  = covered_cells();
    brain.population[brain.current].cost[2]  = complete_rows();
    brain.population[brain.current].cost[3]  = surface_variance();
    brain.population[brain.current].cost[4]  = well_cells();
    brain.population[brain.current].cost[5]  = covered_cells_after_clear();
    brain.population[brain.current].cost[6]  = lock_heigth();
    brain.population[brain.current].cost[7]  = burried_cells();
    brain.population[brain.current].cost[8]  = highest_cell();
    brain.population[brain.current].cost[9]  = height_delta();
    brain.population[brain.current].cost[10] = vertical_roughness();
    brain.population[brain.current].cost[11] = horizontal_roughness();
    brain.population[brain.current].cost[12] = vertical_roughness_w();
    brain.population[brain.current].cost[13] = horizontal_roughness_w();
}

void initialize_pop (){
    for (int i = 0; i < POP_SIZE; ++i) {
        for (int j = 0; j < N_GENES; ++j) {
#ifdef TRAIN
            brain.population[i].weight[j] = ( drand48() * 2.0 - 1.0 ) * 15.0;
#else
            brain.population[i].weight[j] = ia[j];
#endif

            brain.population[i].cost[j]   = 0;
            brain.population[i].fitness   = 0;
            brain.population[i].worst     = 0;
        }


        brain.population[i].fitness = 0;
        /*mutation(&brain.population[i]);*/
    }
}

double get_cost(){
    _obj_costs* obj = &brain.population[brain.current];

    double result = 0;

    for (int i = 0; i < N_FUNCTION; i ++ ) {
        result += obj->cost[i];
    }

    return result;
}

_brain* get_brain_pointer() {
    return &brain;
}

void crossover ( _obj_costs *new_pop, _obj_costs *old_pop, int p1, int p2, int pos) {
    _obj_costs a = old_pop[p1];
    _obj_costs b = old_pop[p2];

    if ( drand48() < brain.crossover_chance ) {
        for (int i = 0; i < N_GENES; ++i) {
            if ( drand48() < .5 ) {
                new_pop[pos + 0].weight[i] = a.weight[i];
                new_pop[pos + 1].weight[i] = b.weight[i];
            } else {
                new_pop[pos + 0].weight[i] = b.weight[i];
                new_pop[pos + 1].weight[i] = a.weight[i];
            }
        }
    } else {
        for (int i = 0; i < N_GENES; ++i) {
            new_pop[pos + 0].weight[i] = a.weight[i];
            new_pop[pos + 1].weight[i] = b.weight[i];
        }
    }

    new_pop[pos + 0].fitness = -1;
    new_pop[pos + 1].fitness = -1;
}

double random_normal() {
      return sqrt(-2*log(drand48())) * cos(2*M_PI*drand48());
}

void mutation ( _obj_costs *individual ) {
    for (int i = 0; i < N_GENES; ++i) {
        if ( drand48() < brain.mutation_chance ) {
            individual->weight[i] += random_normal() * 0.3;
        }
    }
}

_obj_costs get_best_individual() {
    int best = 0;
    int best_i = 0;
    for (int i = 0; i < POP_SIZE; ++i) {
        if ( brain.population[i].fitness > best ) {
            best = brain.population[i].fitness;
            best_i = i;
        }
    }

    return brain.population[best_i];
}

void print_pop() {
    for (int i = 0; i < POP_SIZE; ++i) {
        for (int j = 0; j < N_GENES; ++j) {
            printf("%6.2f ", brain.population[i].weight[j]);
        }
        printf("= %4d\n", brain.population[i].fitness);
    }

    printf("best: %4d / %4d - %12.4f\n", brain.most_lines_cleared, brain.worst_lines_cleared, brain.diversity);

    printf("\n");
    fflush(stdout);
    fflush(stderr);
}

void selection(_obj_costs *old, _obj_costs *new) {
    for (int c = 0; c < POP_SIZE; c++) {
        int p1 = rand() % POP_SIZE;
        int p2 = rand() % POP_SIZE;

        do {
            p1 = rand() % POP_SIZE;
            p2 = rand() % POP_SIZE;
        } while ( p1 == p2 );

        if ( old[p1].fitness > old[p2].fitness ) {
            for (int i = 0; i < N_GENES; ++i) {
                new[c].weight[i] = old[p1].weight[i];
                new[c].cost[i]   = 0;
                new[c].fitness   = old[p1].fitness;
            }
        } else {
            for (int i = 0; i < N_GENES; ++i) {
                new[c].weight[i] = old[p2].weight[i];
                new[c].cost[i]   = 0;
                new[c].fitness   = old[p2].fitness;
            }
        }
    }
}

void evolutionary_step(){
    print_pop();

    if ( POP_SIZE == 1 )
        return;

    _obj_costs new_pop[POP_SIZE];
    _obj_costs best = get_best_individual();

    selection(brain.population, new_pop);

    for (int i = 0; i < POP_SIZE/2; ++i) {
        int p1 = rand() % POP_SIZE;
        int p2 = rand() % POP_SIZE;

        crossover (brain.population, new_pop, p1, p2, i*2);
    }

    for (int i = 0; i < POP_SIZE; ++i) {
        mutation(&brain.population[i]);
    }

    brain.population[0] = best;

    brain.most_lines_cleared = best.fitness > brain.most_lines_cleared ?
                               best.fitness : brain.most_lines_cleared ;

    brain.elapsed_generations += 1;

    brain.diversity = 0;

    for (int i = 0; i < POP_SIZE; ++i) {
        for (int j = 0; j < POP_SIZE; ++j) {
            double d = 0;
            for (int k = 0; k < N_GENES; ++k) {
                d += sqrt ( pow(brain.population[i].weight[k] - brain.population[j].weight[k], 2) );
            }
            brain.diversity += d / ( POP_SIZE * POP_SIZE );
        }
    }
}

void boot_brain() {
    brain.current             = 0;
#ifdef TRAIN
    brain.mutation_chance     = 0.1;
    brain.crossover_chance    = 0.6;
#else
    brain.mutation_chance     = 0.0;
    brain.crossover_chance    = 0.0;
#endif
    brain.max_runs            = 5;
    brain.runs                = 0;
    brain.worst_lines_cleared = 0;
    brain.most_lines_cleared  = 0;
    brain.diversity           =-1;
    brain.rng                 = 1;
    initialize_pop();
}

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

    brain.population[brain.current].fitness = best;

    brain.most_lines_cleared = best > brain.most_lines_cleared ?
                               best : brain.most_lines_cleared ;
}

void finished_evaluating_individual () {
    if ( brain.population[brain.current].fitness < brain.population[brain.current].worst || brain.runs == 0 ) {
        brain.population[brain.current].worst = brain.population[brain.current].fitness;
    }

    brain.runs++;

    if ( brain.runs == brain.max_runs || brain.population[brain.current].fitness == 0 ) {
        brain.runs = 0;

        if ( brain.population[brain.current].worst > brain.worst_lines_cleared ) {
            brain.worst_lines_cleared = brain.population[brain.current].worst;
        }

        brain.population[brain.current].fitness = brain.population[brain.current].worst;

        brain.current ++;

        if ( brain.current >= POP_SIZE ) {
            evolutionary_step();
            brain.current = 0;
        }
    }
}
