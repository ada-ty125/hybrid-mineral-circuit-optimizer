/** Header for the unit class
 * 
 * 
 */

#pragma once


class CUnit {

 public:

  int n_outputs; // The number of outputs from this unit

  //A Boolean that is changed to true if the unit has been seen 
  bool mark;

  int* output;



  CUnit();
  ~CUnit();

  /*

    ...other member functions and variables of CUnit

  */
};

