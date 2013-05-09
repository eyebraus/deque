#!/usr/bin/env python

from datetime import datetime, timedelta
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import matplotlib.dates as mdates
from matplotlib.ticker import LogLocator, LogFormatter
import numpy as np
import re
import sys

proc_counts = [2, 4, 8, 16, 32, 64]

if __name__ == "__main__":
    # cmdline args
    style = sys.argv[1]
    mutex_data = { proc: [] for proc in proc_counts }
    mutex_mean = { proc: 0.0 for proc in proc_counts }
    nonblock_data = { proc: [] for proc in proc_counts }
    nonblock_mean = { proc: 0.0 for proc in proc_counts }

    # also plot sequential
    for proc in proc_counts:
        filename = "results/m%d.out" % proc
        try:
            with open(filename) as outfile:
                for line in outfile:
                    tid, pushes, push_time, pops, pop_time = line.split()
                    tid = int(tid)
                    pushes = long(pushes)
                    push_time = float(push_time)
                    pops = long(pops)
                    pop_time = float(pop_time)
                    if style == "push":
                        mutex_data[proc] += [(float(pushes) / 10000.0)]
                    if style == "pop":
                        mutex_data[proc] += [(float(pops) / 10000.0)]
        except IOError as e:
            print "No file %s, skipping..." % filename
        filename = "results/n%d.out" % proc
        try:
            with open(filename) as outfile:
                for line in outfile:
                    tid, pushes, push_time, pops, pop_time = line.split()
                    tid = int(tid)
                    pushes = long(pushes)
                    push_time = float(push_time)
                    pops = long(pops)
                    pop_time = float(pop_time)
                    if style == "push":
                        nonblock_data[proc] += [(float(pushes) / 10000.0)]
                    if style == "pop":
                        nonblock_data[proc] += [(float(pops) / 10000.0)]
        except IOError as e:
            print "No file %s, skipping..." % filename

    # take the mean of the data found
    for proc in proc_counts:
        # trials = seq_data[proc][size]
        # seq_mean[proc][size] = sum(trials) / float(len(trials))
        mutex_mean[proc] = sum(mutex_data[proc]) / float(len(mutex_data[proc]))
        nonblock_mean[proc] = sum(nonblock_data[proc]) / float(len(nonblock_data[proc]))

    # plot stuff
    figure = plt.figure()
    axes = figure.add_subplot(111)
    axes.set_xlabel("Thread Count")
    axes.set_ylabel("Throughput (ops / ms)")
    #axes.set_xticks([0] + sizes)
    axes.set_xscale('log')
    axes.xaxis.set_major_locator(LogLocator(base = 2.0))
    axes.xaxis.set_major_formatter(LogFormatter(base = 2.0))
    # plot MPI speedups
    x_data = proc_counts
    y_data = [mutex_mean[c] for c in x_data]
    axes.plot(x_data, y_data, "o-", color = "red", label = "Mutual Exclusion")
    y_data = [nonblock_mean[c] for c in x_data]
    axes.plot(x_data, y_data, "o-", color = "blue", label = "Obstruction-free")
    handles, labels = axes.get_legend_handles_labels()
    axes.legend(handles, labels, loc = 2, prop = { 'size': 12 })
    axes.grid(True)
    plot_path = "throughput_%s.pdf" % style
    plt.savefig(plot_path, dpi = 250, bbox_inches='tight', pad_inches=0.5, transparent=False)
