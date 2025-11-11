#!/usr/bin/env python3
"""
Read and visualize backtest results from Artemis.
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import sys
import os

sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (14, 8)

def load_results(results_file="results.csv", trades_file="results_trades.csv"):
    """Load equity curve and trades data."""
    equity_df = pd.read_csv(results_file)
    equity_df['timestamp'] = pd.to_datetime(equity_df['timestamp'], unit='us')
    equity_df.set_index('timestamp', inplace=True)
    
    trades_df = None
    if os.path.exists(trades_file):
        trades_df = pd.read_csv(trades_file)
        trades_df['entry_time'] = pd.to_datetime(trades_df['entry_time'], unit='us')
        trades_df['exit_time'] = pd.to_datetime(trades_df['exit_time'], unit='us')
        trades_df['duration'] = trades_df['duration_us'] / 1e6  # Convert to seconds
    
    return equity_df, trades_df

def plot_equity_curve(equity_df):
    """Plot equity curve."""
    fig, ax = plt.subplots(figsize=(14, 6))
    ax.plot(equity_df.index, equity_df['equity'], linewidth=1.5, color='#2E86AB')
    ax.set_xlabel('Time', fontsize=12)
    ax.set_ylabel('Equity ($)', fontsize=12)
    ax.set_title('Equity Curve', fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig('equity_curve.png', dpi=300, bbox_inches='tight')
    print("Saved equity_curve.png")
    plt.close()

def plot_rolling_sharpe(equity_df, window=1000):
    """Plot rolling Sharpe ratio."""
    returns = equity_df['equity'].pct_change().dropna()
    rolling_sharpe = returns.rolling(window=window).mean() / returns.rolling(window=window).std() * np.sqrt(252 * 24 * 60 * 60)
    
    fig, ax = plt.subplots(figsize=(14, 6))
    ax.plot(rolling_sharpe.index, rolling_sharpe, linewidth=1.5, color='#A23B72')
    ax.axhline(y=0, color='r', linestyle='--', alpha=0.5)
    ax.set_xlabel('Time', fontsize=12)
    ax.set_ylabel('Rolling Sharpe Ratio', fontsize=12)
    ax.set_title(f'Rolling Sharpe Ratio (window={window})', fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig('rolling_sharpe.png', dpi=300, bbox_inches='tight')
    print("Saved rolling_sharpe.png")
    plt.close()

def plot_parameter_heatmap(results_data, threshold_range, sharpe_values):
    """Plot parameter heatmap (threshold vs Sharpe)."""
    if len(sharpe_values) == 0:
        print("No optimization data available")
        return
    
    # Create heatmap data
    heatmap_data = pd.DataFrame({
        'threshold': threshold_range,
        'sharpe': sharpe_values
    })
    
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(heatmap_data['threshold'], heatmap_data['sharpe'], marker='o', linewidth=2, markersize=6)
    ax.set_xlabel('Threshold', fontsize=12)
    ax.set_ylabel('Sharpe Ratio', fontsize=12)
    ax.set_title('Sharpe Ratio vs Threshold', fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    
    # Mark maximum
    max_idx = heatmap_data['sharpe'].idxmax()
    max_threshold = heatmap_data.loc[max_idx, 'threshold']
    max_sharpe = heatmap_data.loc[max_idx, 'sharpe']
    ax.plot(max_threshold, max_sharpe, 'r*', markersize=20, label=f'Max: {max_sharpe:.2f} @ {max_threshold:.1f}')
    ax.legend()
    
    plt.tight_layout()
    plt.savefig('parameter_heatmap.png', dpi=300, bbox_inches='tight')
    print("Saved parameter_heatmap.png")
    plt.close()

def plot_trade_distribution(trades_df):
    """Plot trade PnL distribution."""
    if trades_df is None or len(trades_df) == 0:
        print("No trades data available")
        return
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    
    # PnL distribution
    axes[0].hist(trades_df['pnl'], bins=50, edgecolor='black', alpha=0.7, color='#F18F01')
    axes[0].axvline(x=0, color='r', linestyle='--', linewidth=2)
    axes[0].set_xlabel('PnL ($)', fontsize=12)
    axes[0].set_ylabel('Frequency', fontsize=12)
    axes[0].set_title('Trade PnL Distribution', fontsize=12, fontweight='bold')
    axes[0].grid(True, alpha=0.3)
    
    # Trade duration distribution
    axes[1].hist(trades_df['duration'], bins=50, edgecolor='black', alpha=0.7, color='#C73E1D')
    axes[1].set_xlabel('Duration (seconds)', fontsize=12)
    axes[1].set_ylabel('Frequency', fontsize=12)
    axes[1].set_title('Trade Duration Distribution', fontsize=12, fontweight='bold')
    axes[1].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('trade_distribution.png', dpi=300, bbox_inches='tight')
    print("Saved trade_distribution.png")
    plt.close()

def main():
    if len(sys.argv) > 1:
        results_file = sys.argv[1]
        trades_file = results_file.replace('.csv', '_trades.csv')
    else:
        results_file = "results.csv"
        trades_file = "results_trades.csv"
    
    if not os.path.exists(results_file):
        print(f"Error: {results_file} not found")
        sys.exit(1)
    
    print("Loading results...")
    equity_df, trades_df = load_results(results_file, trades_file)
    
    print("Generating plots...")
    plot_equity_curve(equity_df)
    plot_rolling_sharpe(equity_df)
    plot_trade_distribution(trades_df)
    
    # Print summary statistics
    print("\n=== Summary Statistics ===")
    print(f"Initial Equity: ${equity_df['equity'].iloc[0]:,.2f}")
    print(f"Final Equity: ${equity_df['equity'].iloc[-1]:,.2f}")
    print(f"Total Return: {(equity_df['equity'].iloc[-1] / equity_df['equity'].iloc[0] - 1) * 100:.2f}%")
    print(f"Max Equity: ${equity_df['equity'].max():,.2f}")
    print(f"Min Equity: ${equity_df['equity'].min():,.2f}")
    print(f"Max Drawdown: {(1 - equity_df['equity'].min() / equity_df['equity'].max()) * 100:.2f}%")
    
    if trades_df is not None and len(trades_df) > 0:
        print(f"\nTotal Trades: {len(trades_df)}")
        print(f"Winning Trades: {len(trades_df[trades_df['pnl'] > 0])}")
        print(f"Win Rate: {len(trades_df[trades_df['pnl'] > 0]) / len(trades_df) * 100:.2f}%")
        print(f"Average PnL: ${trades_df['pnl'].mean():.2f}")
        print(f"Average Duration: {trades_df['duration'].mean():.2f} seconds")
    
    print("\nPlots generated successfully!")

if __name__ == "__main__":
    main()

