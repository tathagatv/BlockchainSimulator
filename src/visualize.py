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
    n = len(G)
    node_size = 7000 // n
    font_size = min(8, 700 // n)
    edge_width = min(1.0, font_size / 4)
    nx.draw(G, pos, node_size=node_size, font_size=font_size, arrowsize=font_size, width=edge_width, node_color='darkred', with_labels=True, font_color="white")
    plt.savefig(filename, bbox_inches='tight')
    plt.close()


def calc_graph_stats(G):
    n = max(G.nodes()) + 1
    genesis = 0
    depth = [0 for _ in range(n)]
    max_depth = [0 for _ in range(n)]
    parent = [-1 for _ in range(n)]
    deepest_node = genesis

    def dfs(v, p=-1, d=0):
        nonlocal depth, max_depth, parent, G, deepest_node
        depth[v] = d
        max_depth[v] = d
        parent[v] = p
        if d > depth[deepest_node]:
            deepest_node = v
        for u in G.successors(v):
            dfs(u, v, d + 1)
            max_depth[v] = max(max_depth[v], max_depth[u])

    dfs(genesis)
    branch_lengths = []
    while deepest_node != genesis:
        deepest_node, child = parent[deepest_node], deepest_node
        for u in G.successors(deepest_node):
            if u == child: continue
            length = max_depth[u] - depth[deepest_node]
            branch_lengths.append(length)

    return max_depth[genesis] + 1, np.array(branch_lengths)


base = os.path.join(dirname(dirname(abspath(__file__))), 'output')
edgelist_files = [f for f in os.listdir(base) if "blockchain_edgelist" in f]

for filename in ["blockchain_edgelist_Peer1.txt","blockchain_edgelist_Peer40.txt"]:
    with open(os.path.join(base, filename), 'r') as fp:
        edges = [l.strip().split() for l in fp.readlines() if l.strip()]
        edges = [(int(u), int(v)) for u, v in edges]
    G = nx.DiGraph(edges)
    filename = filename.replace("edgelist","img")
    filename = filename.replace("txt","png")
    outfile = os.path.join(base, filename)
    draw_blockchain(G, outfile)

# max_depth, branch_lengths = calc_graph_stats(G)
# print('Total blocks in Blockchain:', len(G.nodes()))
# print('Longest chain length:', max_depth)
# print('Longest chain length / total number of blocks: %.3f' % (max_depth / len(G.nodes())))
# if len(branch_lengths) > 0:
#     print(f'Branch Lengths: Total={len(branch_lengths)}, Max={branch_lengths.max()}, Mean={branch_lengths.mean():.3f}, Min={branch_lengths.min()}')
# else:
#     print('No branches created')
print()

df = pd.read_csv(os.path.join(base, 'peer_attributes.txt'), index_col='id')

for hash_pwr_is_fast, grp_df in df.groupby(['hash_power', 'is_fast']):
    hash_pwr, is_fast = hash_pwr_is_fast
    hash_pwr = 'HIGH' if grp_df['hash_power'].mean() > df['hash_power'].mean() else 'LOW'
    mean_frac = grp_df['chain_blocks'].divide(grp_df['generated_blocks']).fillna(0).mean()
    print(f'Hash power: {hash_pwr}, Fast Node? {bool(is_fast)}, Mean fraction of blocks: {mean_frac:.3f}')

df = df.sort_values(by='hash_power').reset_index(drop=True)
total_blocks_in_chain = df['generated_blocks'].sum()
fraction_blocks_in_chain = df['chain_blocks'].to_numpy() / total_blocks_in_chain
peer_ids = df.index.to_numpy()
slow_peers = df[df['is_fast'] == 0]
fast_peers = df[df['is_fast'] == 1]

style.use('ggplot')
fig = plt.figure(dpi=300)
plt.xlabel('Peer ID')
plt.xticks(peer_ids, [str(o + 1) if o % 5 == 0 or o + 1 == len(peer_ids) else '' for o in peer_ids])
plt.title('Statistics')
plt.plot(peer_ids, df['hash_power'].to_numpy(), label='Fraction of Hash Power')
plt.scatter(slow_peers.index, fraction_blocks_in_chain[slow_peers.index], label='Fraction of Blocks in Longest Chain, Slow Peer')
plt.scatter(fast_peers.index, fraction_blocks_in_chain[fast_peers.index], label='Fraction of Blocks in Longest Chain, Fast Peer')
plt.legend(prop={'size': 8}, framealpha=0.3)
plt.savefig(os.path.join(base, 'block_stats.png'), bbox_inches='tight')
plt.close()
