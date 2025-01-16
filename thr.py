import sqlite3
import os
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import pickle
from itertools import chain
import pickleshare
import seaborn as sns
from mpl_toolkits.mplot3d import Axes3D

os.makedirs('/home/justdrago/v2x/ns-3-dev/Data', exist_ok=True)

def calculate_mean_throughput(db_path):
    # Connect to the database
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Query to get the throughput data where thputKbps is not equal to 0
    query = "SELECT dstIp, thputKbps FROM thput WHERE thputKbps != 0"
    df = pd.read_sql_query(query, conn)

    # Calculate the mean throughput for each dstIp
    mean_throughput = df.groupby('dstIp')['thputKbps'].mean()

    # Close the connection
    conn.close()

    return mean_throughput

def get_mean_delay(db_path):
    # Connect to the database
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Query to get the delay data
    query = "SELECT AVG (delayMicroSec) FROM rlcRx WHERE delayMicroSec IS NOT NULL"
    df = pd.read_sql_query(query, conn)

    # Calculate the mean delay
    mean_delay = df.iloc[0, 0]

    # Close the connection
    conn.close()

    return mean_delay

def extract_delay_values(db_path):
    # Connect to the database
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Query to get the delay data
    query = "SELECT delayMicroSec FROM rlcRx WHERE delayMicroSec IS NOT NULL"
    df = pd.read_sql_query(query, conn)
    df['delayMillisec'] = df['delayMicroSec'] / 1000.0
    # Close the connection
    conn.close()

    return df['delayMillisec']


def plot_mean_throughput(mean_throughput, title):
    mean_throughput.plot(kind='bar')
    plt.title(title)
    plt.xlabel('Destination IP')
    plt.ylabel('Mean Throughput (Kbps)')
    plt.show()

def plot_mean_delay(db_paths, title):
    mean_delays = []
    labels = ['1 Sect','2 Sect','6 Sect']

    for i, db_path in enumerate(db_paths):
        mean_delay = get_mean_delay(db_path)
        mean_delays.append(mean_delay)

    # Combine the mean delay data into a DataFrame
    combined_df = pd.DataFrame({'Database': labels, 'Mean Delay (Microseconds)': mean_delays})

    # Create a linear plot
    plt.figure(figsize=(10, 6))
    ax = sns.lineplot(x='Database', y='Mean Delay (Microseconds)', data=combined_df, marker='o')
    plt.title(title)
    plt.xlabel('Database')
    plt.ylabel('Mean Delay (Microseconds)')
    plt.ylim(400, 600)
    for i, mean_delay in enumerate(mean_delays):
        ax.text(i, mean_delay + 5, f'{mean_delay:.2f}', ha='center')

    plt.show()

def plot_violin_plot(combined_df):
    plt.figure(figsize=(10, 6))
    custom_palette = ["#3498db", "#e74c3c", "#2ecc71"]  
    sns.violinplot(x='Database', y='Delay', data=combined_df,palette=custom_palette)
    plt.title('Violin Plot of Delay Values')
    plt.ylabel('Delay (ms)')
    
    handles = [plt.Line2D([0], [0], color=color, lw=4) for color in custom_palette]
    labels = combined_df['Database'].unique()
    plt.legend(handles, labels, title='Database')
    plt.show()


if __name__ == "__main__":
    db_paths = [
        '/home/justdrago/v2x/ns-3-dev/tesi1sect.db',
        '/home/justdrago/v2x/ns-3-dev/tesi2sect.db',
        '/home/justdrago/v2x/ns-3-dev/tesi6sect.db'
    ]

    delay_data = []
    labels = []

    for i, db_path in enumerate(db_paths):
        # Calculate the mean throughput for each database
        mean_throughput = calculate_mean_throughput(db_path)
        #print(mean_throughput)
        plot_mean_throughput(mean_throughput, f'Mean Throughput for {db_path}')
        # Plot the boxplot of delay values for each database
        delay_values = extract_delay_values(db_path)
        mean_delay = get_mean_delay(db_path)
        #print(mean_delay)
        if len(delay_values) > 0:
            delay_data.extend(delay_values)
            sector_labels = ["1 Sector", "2 Sectors", "6 Sectors"]
            labels.extend([sector_labels[i]] * len(delay_values))

    if len(delay_data) == 0:
        print("No delay data available for plotting.")
    else:
        # Combine the delay data into a single DataFrame
        combined_df = pd.DataFrame({'Delay': delay_data, 'Database': labels})

        # Plot different types of charts
        plot_violin_plot(combined_df)
        plot_mean_delay(db_paths, 'Mean Delay for 3 Databases')
        
       
    