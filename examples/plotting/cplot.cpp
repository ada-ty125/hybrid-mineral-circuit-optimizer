#include <graphviz/gvc.h>

#include <string>

// Function to quiet the compiler about string literals being non-const,
// since Graphviz's API expects char* for arguments, even though it doesn't modify them.

char* const_str(const char* str) {
    return const_cast<char*>(str);
}

int main(int argc, char** argv) {
    Agraph_t* g;
    GVC_t* gvc;
    Agedge_t *edge1, *edge2, *edge3;
    Agnode_t *node1, *node2, *node3;

    char* args[4]; 
    int i;
  
  
    args[0] = const_str("dot");
    args[1] = const_str("-Tpng");
    args[2] = const_str("-o");
    args[3] = const_str("example.png");



    gvc = gvContext(); /* library function */

    gvParseArgs(gvc, 4, args);
    g = agopen(const_str("g"), Agdirected, 0);

    agsafeset(g, const_str("rankdir"), const_str("LR")  , const_str(""));
    agattr(g, AGNODE, const_str("shape"), const_str("rectangle"));

    node1 = agnode(g, const_str("Feed"), 1);
    node2 = agnode(g, const_str("Unit 1"), 1);
    node3 = agnode(g, const_str("Unit 2"), 1);

    edge1 = agedge(g, node1, node2, 0, 1);

    edge2 = agedge(g, node2, node3, 0, 1);
    agsafeset(edge2, const_str("color"), const_str("blue"), const_str(""));
    agsafeset(edge2, const_str("tailport"), const_str("n"), const_str(""));
    agsafeset(edge2, const_str("headport"), const_str("w"), const_str(""));
    
    edge3 = agedge(g, node2, node3, 0, 1);
    agsafeset(edge3, const_str("color"), const_str("red"), const_str(""));
    agsafeset(edge3, const_str("tailport"), const_str("s"), const_str(""));
    agsafeset(edge3, const_str("headport"), const_str("w"), const_str(""));

    gvLayoutJobs(gvc, g);
    /* Write the graph according to -T and -o options */
    gvRenderJobs(gvc, g);

    gvFreeLayout(gvc, g); /* library function */
    agclose (g); /* library function */
    gvFreeContext(gvc);

    return 0;
}
