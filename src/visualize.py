# apt install graphviz
# pip install networkx pydot matplotlib

import os
import matplotlib
matplotlib.use('Agg')
from matplotlib import style
import matplotlib.pyplot as plt
import networkx as nx
import pydot
from networkx.drawing.nx_pydot import graphviz_layout
from glob import glob
from tqdm import tqdm
from os.path import dirname, abspath


def draw_blockchain(G, filename):
    pos = graphviz_layout(G, prog="dot")
    pos = {k: (-y, x) for k, (x, y) in pos.items()}
    fig = plt.figure(dpi=300)
    n = len(G.nodes())
    node_size = 7000 // n
    font_size = min(8, 700 // n)
    edge_width = min(1.0, font_size / 4)
    nx.draw(G, pos, node_size=node_size, font_size=font_size, arrowsize=font_size, width=edge_width, node_color='darkred', with_labels=True, font_color="white")
    plt.savefig(filename, bbox_inches='tight')
    plt.close()


base = os.path.join(dirname(dirname(abspath(__file__))), 'output')

for file in glob(os.path.join(base, 'blockchain_edgelist_*.txt')):
    with open(file, 'r') as fp:
        G = nx.read_edgelist(fp, create_using=nx.DiGraph)
    outfile = os.path.join(base, f'blockchain_image.png')
    print('Total nodes in Blockchain:', len(G.nodes()))
    draw_blockchain(G, outfile)
    break

for file in glob(os.path.join(base, 'blocks_each_peer_*.txt')):
    peer_ids, num_blocks_in_chain, blocks_generated, hash_pwr = list(), list(), list(), list()
    with open(file, 'r') as fp:
        for l in fp.readlines():
            if not l.strip(): continue
            line_split = l.strip().split()
            peer_ids.append(int(line_split[0]))
            num_blocks_in_chain.append(int(line_split[1]))
            assert line_split[2] == '/'
            blocks_generated.append(int(line_split[3]))
            hash_pwr.append(float(line_split[4]))

    total_blocks_in_chain = sum(num_blocks_in_chain)
    fraction_blocks_in_chain = list(map(lambda x: x / total_blocks_in_chain, num_blocks_in_chain))

    style.use('ggplot')
    fig = plt.figure(dpi=300)
    plt.xlabel('Peer ID')
    plt.title('Statistics')
    plt.plot(peer_ids, hash_pwr, label='Fraction of Hash Power')
    plt.plot(peer_ids, fraction_blocks_in_chain, label='Fraction of Blocks in Longest Chain')
    plt.legend()
    plt.savefig(os.path.join(base, 'block_stats.png'), bbox_inches='tight')
    plt.close()
    break
