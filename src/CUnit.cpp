#include "CUnit.h"

CUnit::CUnit(): n_outputs(0),
       output(new int [n_outputs]),
       mark(false) {}

CUnit::~CUnit() {
  delete[] output;
}
