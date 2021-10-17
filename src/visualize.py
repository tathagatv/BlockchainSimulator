import os, sys
import matplotlib
matplotlib.use('Agg')
from matplotlib import style
import matplotlib.pyplot as plt
import networkx as nx
import pydot
from networkx.drawing.nx_pydot import graphviz_layout
from os.path import dirname, abspath
import pandas as pd
import numpy as np
import warnings

warnings.filterwarnings("ignore")

def draw_blockchain(G, filename):
    pos = graphviz_layout(G, prog="dot")
    pos = {k: (-y, x) for k, (x, y) in pos.items()}
    fig = plt.figure(dpi=300)
    n = len(G)
    node_size = 7000 // n
    font_size = min(8, 700 // n)
    edge_width = min(1.0, font_size / 4)
    nx.draw(G, pos, 
        node_size=node_size, 
        font_size=font_size, 
        arrowsize=font_size, 
        width=edge_width, 
        node_color='darkred', 
        with_labels=True, 
        font_color="white"
    )
    plt.savefig(filename, bbox_inches='tight')
    plt.close()


def draw_peer_network(file, adversary=True):
    with open(file, 'r') as fp:
        edges = [l.strip().split() for l in fp.readlines() if l.strip()]
        edges = [(int(u), int(v)) for u, v in edges]
    G = nx.Graph(edges)
    adversary = max(G.nodes()) if adversary else -1
    colours = [('darkred' if v == adversary else '#1f78b4') for v in G.nodes()]
    pos = graphviz_layout(G, prog="circo")
    filename = file.replace('_edgelist.txt', '_img.png')
    nx.draw(G, pos, 
        node_color=colours, 
        with_labels=True, 
        font_color="white"
    )
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


def R_pool(alpha, gamma):
    numerator = (alpha * (1 - alpha) * (1 - alpha) * (4 * alpha + gamma * (1 - 2 * alpha)) - alpha * alpha * alpha)
    denominator = 1 - alpha * (1 + alpha * (2 - alpha))
    return numerator / denominator


base = os.path.join(dirname(dirname(abspath(__file__))), 'output')
network_file = os.path.join(base, 'peer_network_edgelist.txt')
draw_peer_network(network_file)

peer = int(sys.argv[1]) if len(sys.argv) > 1 else -1
num_peers = len(os.listdir(os.path.join(base, 'block_arrivals')))
peer = (peer % num_peers + num_peers) % num_peers
peer = f'Peer{peer + 1}.txt'
adversary = num_peers

for folder in ['final_blockchains', 'termination_blockchains']:
    path = os.path.join(base, folder, peer)
    with open(path, 'r') as fp:
        edges = [l.strip().split() for l in fp.readlines() if l.strip()]
        edges = [(int(u), int(v)) for u, v in edges]
    G = nx.DiGraph(edges)
    filename = path[:-4] + '_img.png'
    draw_blockchain(G, filename)

# max_depth, branch_lengths = calc_graph_stats(G)
# print('Total blocks in Blockchain:', len(G.nodes()))
# print('Longest chain length:', max_depth)
# print('Longest chain length / total number of blocks: %.3f' % (max_depth / len(G.nodes())))
# if len(branch_lengths) > 0:
#     print(f'Branch Lengths: Total={len(branch_lengths)}, Max={branch_lengths.max()}, Mean={branch_lengths.mean():.3f}, Min={branch_lengths.min()}')
# else:
#     print('No branches created')
# print()

stat_file = os.path.join(base, 'peer_stats', peer)
df = pd.read_csv(stat_file, index_col='id')

total_blocks = df['generated_blocks'].sum()
total_blocks_in_chain = df['chain_blocks'].sum()
total_hash_power = df['hash_power'].sum()
adversary_stats = df.loc[adversary].copy()
alpha = round(adversary_stats['hash_power'] / total_hash_power, 2)

mpu_adv = adversary_stats['chain_blocks'] / adversary_stats['generated_blocks']
rpool = adversary_stats['chain_blocks'] / total_blocks_in_chain
mpu_overall = total_blocks_in_chain / total_blocks

print(f'Alpha = {alpha:.2f}')
print(f'MPU_adv = {mpu_adv:.5f}')
print(f'MPU_overall = {mpu_overall:.5f}')
print(f'Gamma_0 = {R_pool(alpha, 0):.5f}')
print(f'Gamma_1 = {R_pool(alpha, 1):.5f}')
print(f'R_pool = {rpool:.5f}')

# for hash_pwr_is_fast, grp_df in df.groupby(['hash_power', 'is_fast']):
#     hash_pwr, is_fast = hash_pwr_is_fast
#     hash_pwr = 'HIGH' if grp_df['hash_power'].mean() > df['hash_power'].mean() else 'LOW'
#     mean_frac = grp_df['chain_blocks'].divide(grp_df['generated_blocks']).fillna(0).mean()
#     print(f'Hash power: {hash_pwr}, Fast Node? {bool(is_fast)}, Mean fraction of blocks: {mean_frac:.3f}')

df = df.sort_values(by='hash_power').reset_index(drop=True)
fraction_blocks_in_chain = df['chain_blocks'].to_numpy() / total_blocks_in_chain
peer_ids = df.index.to_numpy()
slow_peers = df[df['is_fast'] == 0]
fast_peers = df[df['is_fast'] == 1]

stat_outfile = stat_file[:-4] + '_stats.png'
style.use('ggplot')
fig = plt.figure(dpi=300)
plt.xlabel('Peer ID')
plt.xticks(peer_ids, [str(o + 1) if o % 5 == 0 or o + 1 == len(peer_ids) else '' for o in peer_ids])
plt.title('Statistics')
plt.plot(peer_ids, df['hash_power'].to_numpy() / total_hash_power, label='Fraction of Hash Power')
plt.scatter(slow_peers.index, fraction_blocks_in_chain[slow_peers.index], label='Fraction of Blocks in Longest Chain, Slow Peer')
plt.scatter(fast_peers.index, fraction_blocks_in_chain[fast_peers.index], label='Fraction of Blocks in Longest Chain, Fast Peer')
plt.legend(prop={'size': 8}, framealpha=0.3, loc='upper left')
plt.savefig(stat_outfile, bbox_inches='tight')
plt.close()
