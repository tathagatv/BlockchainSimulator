# apt install graphviz
# pip install networkx pydot matplotlib

import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import networkx as nx
import pydot
from networkx.drawing.nx_pydot import graphviz_layout
import glob
from tqdm import tqdm
from os.path import dirname, abspath


def draw_blockchain(G, filename):
    pos = graphviz_layout(G, prog="dot")
    pos = {k:(-y, x) for k, (x, y) in pos.items()}
    fig = plt.figure(dpi=300)
    n = len(G.nodes())
    node_size = 7000 // n
    font_size = min(8, 700 // n)
    nx.draw(G, pos, node_size=node_size, font_size=font_size, node_color='darkred', with_labels=True, font_color="white")
    plt.savefig(filename, bbox_inches='tight')
    plt.close()

base = os.path.join(dirname(dirname(abspath(__file__))), 'output')
graphs = []

for file in glob.glob(os.path.join(base, 'blockchain_edgelist_*.txt')):
    with open(file, 'r') as fp:
        G = nx.read_edgelist(fp, create_using=nx.DiGraph)
    graphs.append(G)

for i in tqdm(range(len(graphs[:1]))):
    file = os.path.join(base, f'blockchain_image_{i}.png')
    draw_blockchain(graphs[i], file)
