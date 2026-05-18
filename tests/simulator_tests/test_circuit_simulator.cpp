#include <cmath>
#include <iostream>

#include "ESEGraph.h"
#include "CSimulator.h"

using namespace ESE;

int main(int argc, char * argv[])
{

      // dummy vector. Replace in your code with the actual circuit vector.
      GraphSizes sizes(1, 2, 3);
      int node_degrees[] = {2, 3};
      std::span<int> node_degree_span(node_degrees);
      input_node_id inputs[] = {0, 1, 2, 0, 2, 3};
      std::span<input_node_id> connectivity_span(inputs);

      CSRGraph graph(sizes, node_degree_span, connectivity_span);

      // Test value based on dummy circuit_performance function.
      // Replace with actual performance value.

      std::cout << "circuit_performance(graph) close to 500.0 :\n";
      double result = circuit_performance(graph);
      std::cout << "circuit_performance(graph) = "<< result <<"\n";

      if (std::fabs(result - 500.0)<1.0e-8) {
                  std::cout << "pass\n";
            } else {
	        std::cout << "fail\n";
              return 1;
           }
	
}
