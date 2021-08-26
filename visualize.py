# apt install graphviz
# pip install networkx pydot matplotlib

import os
import matplotlib.pyplot as plt
import networkx as nx
import pydot
from networkx.drawing.nx_pydot import graphviz_layout
import glob
from tqdm import tqdm

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

# with open('blockchain_edgelist.txt', 'r') as fp:
#     G = nx.read_edgelist(fp, create_using=nx.DiGraph)

# draw_blockchain(G, 'blockchain_image.png')

base = 'blockchain'
graphs = []

for file in glob.glob(os.path.join(base, "*.txt")):
    with open(file, 'r') as fp:
        G = nx.read_edgelist(fp, create_using=nx.DiGraph)
    graphs.append(G)


for i in tqdm(range(len(graphs[:1]))):
    draw_blockchain(graphs[i], f'blockchain/blockchain_image{i}.png')