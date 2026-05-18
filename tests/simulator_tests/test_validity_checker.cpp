#include <iostream>

#include "CSRGraph.h"
#include "CUnit.h"
#include "CCircuit.h"
#include "ESEGraph.h"

using namespace ESE;

int main(int argc, char * argv[]){


    GraphSizes sizes(1, 1, 2);

    std::size_t node_degrees[] = {2};
    input_node_id valid_example[] = {0, 1, 2};
    input_node_id invalid_example[] = {0, 1, 0};


    CSRGraph graph_valid(sizes,
        node_degrees,
        valid_example);

    CSRGraph graph_invalid(sizes,
        node_degrees,
        invalid_example);


	std::cout << "check_validity({0,1,2}):\n";
        if (check_validity(graph_valid))
	    std::cout  << "pass\n";
	else {
	    std::cout << "fail\n";
        return 1;
    }

/*      
	std::cout << "Check_Validity({0,1,0}):\n";
        if (Check_Validity(invalid))
            std::cout << "fail\n";
        else
            std::cout << "pass\n";
*/

    return 0; // return 0 if all tests pass
}
