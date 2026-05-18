import graphviz
graph = graphviz.Digraph()
graph.attr(rankdir='LR')
graph.attr('node', shape='rectangle')
graph.edge('Feed', 'Unit 1', color='black',
           headport='w', tailport='e',
           arrowhead='normal')
graph.edge('Unit 1', 'Unit 2', color='blue',
           headport='w', tailport='n',
           arrowhead='normal')
graph.edge('Unit 1', 'Unit 2', color='red',
           headport='w', tailport='s',
           arrowhead='normal')
graph.render('example', cleanup=True, format='png')
