import os
import matplotlib.pyplot as plt
import networkx as nx
import pydot
from networkx.drawing.nx_pydot import graphviz_layout

base = 'blockchain'
graphs = []

for file in os.listdir(base):
    file = os.path.join(base, file)
    with open(file, 'r') as fp:
        G = nx.read_edgelist(fp, create_using=nx.DiGraph)
    graphs.append(G)

T = graphs[0]
pos = graphviz_layout(T, prog="twopi")
nx.draw(T, pos, node_size=500, with_labels=True, font_color="whitesmoke")

plt.savefig('image.png', bbox_inches='tight')