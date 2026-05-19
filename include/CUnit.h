/** Header for the unit class
 * 
 * 
 */

#pragma once
#ifndef CUNIT_H
#define CUNIT_H

#include <vector>


class CUnit {
public:
    // Constructor: Initializes the unit with default parameters
    CUnit();

    // =========================================================================
    // 1. Structural & Topological Variables
    // =========================================================================
    int n_outputs;               // Number of output streams 
    std::vector<int> output;     // Destinations of output streams (Unit IDs or Circuit Outlets) 
    int unit_type;               // Unit type identifier (e.g., 0 for Type A, 1 for Type B) 

    // =========================================================================
    // 2. Graph Traversal State Variables 
    // =========================================================================
    bool mark;                   // Flag to prevent infinite loops during DFS traversal 

    // =========================================================================
    // 3. Accessibility & Path Validity Flags
    // =========================================================================
    bool reaches_palusznium;     // True if this unit has a forward route to the Palusznium stream 
    bool reaches_gormanium;      // True if this unit has a forward route to the Gormanium stream 
    bool reaches_tailings;       // True if this unit has a forward route to the Final Tailings stream 

    // =========================================================================
    // 4. Mass Balance & Physical Variables
    // =========================================================================
    // Current mass flow rates of feed entering the cell (kg/s) 
    double feed_P;               // Palusznium feed rate 
    double feed_G;               // Gormanium feed rate 
    double feed_W;               // Waste feed rate 

    // Mass flow rates from the previous iteration (for convergence checking)
    double old_feed_P;           // Previous iteration Palusznium feed rate
    double old_feed_G;           // Previous iteration Gormanium feed rate [
    double old_feed_W;           // Previous iteration Waste feed rate 

    double residence_time;       // Average residence time τ of material inside the cell 
};

#endif