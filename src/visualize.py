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
import pandas as pd
import numpy as np


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


df = pd.read_csv(os.path.join(base, 'peer_attributes.txt'), index_col='id')

for hash_pwr, grp_df in df.groupby(['hash_power']):
    mean_frac = grp_df['chain_blocks'].divide(grp_df['generated_blocks']).fillna(0).mean()
    print(f'Hash power: {hash_pwr * 100:.3f}, Mean fraction of blocks: {mean_frac:.3f}')
print()

for is_fast, grp_df in df.groupby(['is_fast']):
    mean_frac = grp_df['chain_blocks'].divide(grp_df['generated_blocks']).fillna(0).mean()
    print(f'Fast Node? {bool(is_fast)}, Mean fraction of blocks: {mean_frac:.3f}')
print()

df = df.sort_values(by='hash_power').reset_index(drop=True)
total_blocks_in_chain = df['generated_blocks'].sum()
fraction_blocks_in_chain = df['chain_blocks'].to_numpy() / total_blocks_in_chain
peer_ids = df.index.to_numpy()

style.use('ggplot')
fig = plt.figure(dpi=300)
plt.xlabel('Peer ID')
plt.title('Statistics')
plt.plot(peer_ids, df['hash_power'].to_numpy(), label='Fraction of Hash Power')
plt.scatter(peer_ids, fraction_blocks_in_chain, label='Fraction of Blocks in Longest Chain')
plt.legend()
plt.savefig(os.path.join(base, 'block_stats.png'), bbox_inches='tight')
plt.close()
