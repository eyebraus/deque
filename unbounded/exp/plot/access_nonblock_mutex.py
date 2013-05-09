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
    nonblock_data = { proc: [] for proc in proc_counts }
    nonblock_mean = { proc: 0.0 for proc in proc_counts }
    mutex_data = { proc: [] for proc in proc_counts }
    mutex_mean = { proc: 0.0 for proc in proc_counts }

    # also plot sequential
    for proc in proc_counts:
        for trial in range(5):
            filename = "../results/nonblock_timing_%s_%d_%d.out" % (style, proc, trial + 1)
            try:
                with open(filename) as outfile:
                    for line in outfile:
                        exec_time = float(line.split()[2])
                        nonblock_data[proc] += [exec_time]
            except IOError as e:
                print "No file %s, skipping..." % filename
            filename = "../results/mutex_timing_%s_%d_%d.out" % (style, proc, trial + 1)
            try:
                with open(filename) as outfile:
                    for line in outfile:
                        exec_time = float(line.split()[2])
                        mutex_data[proc] += [exec_time]
            except IOError as e:
                print "No file %s, skipping..." % filename

    # take the mean of the data found
    for proc in proc_counts:
        if len(nonblock_data[proc]) > 0:
            nonblock_mean[proc] = sum(nonblock_data[proc]) / float(len(nonblock_data[proc]))
        else:
            del nonblock_mean[proc]
        if len(mutex_data[proc]) > 0:
            mutex_mean[proc] = sum(mutex_data[proc]) / float(len(mutex_data[proc]))
        else:
            del mutex_mean[proc]

    # plot stuff
    figure = plt.figure()
    axes = figure.add_subplot(111)
    axes.set_xlabel("Thread Count")
    axes.set_ylabel("Execution Time (ms)")
    #axes.set_xticks([0] + sizes)
    axes.set_xscale('log')
    # axes.set_yscale('log')
    axes.xaxis.set_major_locator(LogLocator(base = 2.0))
    axes.xaxis.set_major_formatter(LogFormatter(base = 2.0))
    # axes.yaxis.set_major_locator(LogLocator(base = 10.0))
    # axes.yaxis.set_major_formatter(LogFormatter(base = 10.0))
    # plot MPI speedups
    x_data = sorted(nonblock_mean.keys())
    y_data = [nonblock_mean[c] for c in sorted(nonblock_mean.keys())]
    axes.plot(x_data, y_data, "o-", color = "red", label = "%s: Obstruction-free" % style)
    x_data = sorted(mutex_mean.keys())
    y_data = [mutex_mean[c] for c in sorted(mutex_mean.keys())]
    axes.plot(x_data, y_data, "s-", color = "blue", label = "%s: Mutual Exclusion" % style)
    handles, labels = axes.get_legend_handles_labels()
    axes.legend(handles, labels, loc = 2, prop = { 'size': 16 })
    axes.grid(True)
    plot_path = "comp_timing_%s.pdf" % style
    plt.savefig(plot_path, dpi = 250, bbox_inches='tight', pad_inches=0.5, transparent=False)
