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
    stack_data = { proc: [] for proc in proc_counts }
    stack_mean = { proc: 0.0 for proc in proc_counts }
    queue_data = { proc: [] for proc in proc_counts }
    queue_mean = { proc: 0.0 for proc in proc_counts }
    random_data = { proc: [] for proc in proc_counts }
    random_mean = { proc: 0.0 for proc in proc_counts }

    # also plot sequential
    for proc in proc_counts:
        for trial in range(5):
            filename = "../results/nonblock_throughput_stack_%d_%d.out" % (proc, trial + 1)
            try:
                with open(filename) as outfile:
                    for line in outfile:
                        exec_time = float(line.split()[2])
                        stack_data[proc] += [exec_time]
            except IOError as e:
                print "No file %s, skipping..." % filename
            filename = "../results/nonblock_throughput_queue_%d_%d.out" % (proc, trial + 1)
            try:
                with open(filename) as outfile:
                    for line in outfile:
                        exec_time = float(line.split()[2])
                        queue_data[proc] += [exec_time]
            except IOError as e:
                print "No file %s, skipping..." % filename
            filename = "../results/nonblock_throughput_random_%d_%d.out" % (proc, trial + 1)
            try:
                with open(filename) as outfile:
                    for line in outfile:
                        exec_time = float(line.split()[2])
                        random_data[proc] += [exec_time]
            except IOError as e:
                print "No file %s, skipping..." % filename

    # take the mean of the data found
    for proc in proc_counts:
        if len(stack_data[proc]) > 0:
            stack_mean[proc] = sum(stack_data[proc]) / float(len(stack_data[proc]))
        else:
            del stack_mean[proc]
        if len(queue_data[proc]) > 0:
            queue_mean[proc] = sum(queue_data[proc]) / float(len(queue_data[proc]))
        else:
            del queue_mean[proc]
        if len(random_data[proc]) > 0:
            random_mean[proc] = sum(random_data[proc]) / float(len(random_data[proc]))
        else:
            del stack_mean[proc]

    # plot stuff
    figure = plt.figure()
    axes = figure.add_subplot(111)
    axes.set_xlabel("Thread Count")
    axes.set_ylabel("Throughput (ops / ms)")
    #axes.set_xticks([0] + sizes)
    axes.set_xscale('log')
    axes.set_yscale('log')
    axes.xaxis.set_major_locator(LogLocator(base = 2.0))
    axes.xaxis.set_major_formatter(LogFormatter(base = 2.0))
    axes.yaxis.set_major_locator(LogLocator(base = 10.0))
    axes.yaxis.set_major_formatter(LogFormatter(base = 10.0))
    # plot MPI speedups
    x_data = sorted(stack_mean.keys())
    y_data = [stack_mean[c] for c in sorted(stack_mean.keys())]
    axes.plot(x_data, y_data, "o-", color = "red", label = "Stack Access Pattern")
    x_data = sorted(queue_mean.keys())
    y_data = [queue_mean[c] for c in sorted(queue_mean.keys())]
    axes.plot(x_data, y_data, "s-", color = "blue", label = "Queue Access Pattern")
    x_data = sorted(random_mean.keys())
    y_data = [random_mean[c] for c in sorted(random_mean.keys())]
    axes.plot(x_data, y_data, "^-", color = "green", label = "Random Access Pattern")
    handles, labels = axes.get_legend_handles_labels()
    axes.legend(handles, labels, loc = 2, prop = { 'size': 16 })
    axes.grid(True)
    plot_path = "access_throughput.pdf"
    plt.savefig(plot_path, dpi = 250, bbox_inches='tight', pad_inches=0.5, transparent=False)
