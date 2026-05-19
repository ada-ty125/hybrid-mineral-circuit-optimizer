#include "CUnit.h"

/**
 * @brief Default constructor for CUnit.
 * Initializes structural vectors, graph flags, and physical flow rates 
 * to safe, reliable baseline values.
 */
CUnit::CUnit() : 
    n_outputs(2),               // Default to a Type A unit with 2 output streams 
    unit_type(0),               // Default type code corresponding to Type A 
    mark(false),                // Initially unmarked for the graph search algorithm 
    reaches_palusznium(false),  // Assume outlets are unreachable until verified 
    reaches_gormanium(false),   
    reaches_tailings(false),    
    feed_P(0.0),                // Initialize all process stream flow rates to zero 
    feed_G(0.0),                
    feed_W(0.0),                
    old_feed_P(0.0),            
    old_feed_G(0.0),            
    old_feed_W(0.0),            
    residence_time(0.0)         // Baseline residence time 
{
    // Resize the dynamic vector to size 2 to prevent any out-of-bounds errors 
    // during early vector parsing or standard Type A operations.
    output.resize(2, 0); 
}