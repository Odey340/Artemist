#!/usr/bin/env python3
"""
Grid search optimization for threshold parameter.
"""

import subprocess
import sys
import os
import pandas as pd
import numpy as np
import json

def run_backtest(data_file, threshold, artemis_path):
    """Run backtest with given threshold and parse output."""
    try:
        result = subprocess.run(
            [artemis_path, data_file, str(threshold)],
            capture_output=True,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )
        
        if result.returncode != 0:
            print(f"Error running backtest: {result.stderr}")
            return None
        
        # Parse output
        output = result.stdout
        sharpe = None
        max_dd = None
        
        for line in output.split('\n'):
            if 'Sharpe Ratio:' in line:
                try:
                    sharpe = float(line.split(':')[1].strip())
                except:
                    pass
            if 'Max Drawdown:' in line:
                try:
                    max_dd = float(line.split(':')[1].strip().replace('%', ''))
                except:
                    pass
        
        return {'sharpe': sharpe, 'max_dd': max_dd, 'threshold': threshold}
    
    except Exception as e:
        print(f"Exception running backtest: {e}")
        return None

def grid_search(data_file, artemis_path, threshold_min=1.5, threshold_max=4.0, step=0.1):
    """Perform grid search over threshold parameter."""
    thresholds = np.arange(threshold_min, threshold_max + step, step)
    results = []
    
    print(f"Running grid search over {len(thresholds)} thresholds...")
    print(f"Range: {threshold_min} to {threshold_max} (step: {step})")
    
    for i, threshold in enumerate(thresholds):
        print(f"\n[{i+1}/{len(thresholds)}] Testing threshold: {threshold:.1f}")
        result = run_backtest(data_file, threshold, artemis_path)
        if result:
            results.append(result)
            print(f"  Sharpe: {result['sharpe']:.4f}, Max DD: {result['max_dd']:.2f}%")
        else:
            print(f"  Failed to get results")
    
    return results

def main():
    if len(sys.argv) < 2:
        data_file = "data/ES_futures_sample.csv"
    else:
        data_file = sys.argv[1]
    
    if not os.path.exists(data_file):
        print(f"Error: Data file {data_file} not found")
        sys.exit(1)
    
    # Find artemis executable
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    # Try different possible locations
    possible_paths = [
        os.path.join(project_root, "build", "artemis"),
        os.path.join(project_root, "build", "Release", "artemis"),
        os.path.join(project_root, "build", "Debug", "artemis"),
        os.path.join(project_root, "artemis"),
    ]
    
    if os.name == 'nt':  # Windows
        possible_paths = [p + ".exe" for p in possible_paths]
    
    artemis_path = None
    for path in possible_paths:
        if os.path.exists(path):
            artemis_path = path
            break
    
    if not artemis_path:
        print(f"Error: artemis executable not found. Please build the project first.")
        print(f"Tried:")
        for path in possible_paths:
            print(f"  - {path}")
        sys.exit(1)
    
    print(f"Using artemis: {artemis_path}")
    
    # Change to project root for relative paths
    os.chdir(project_root)
    
    # Run grid search
    results = grid_search(data_file, artemis_path, threshold_min=1.5, threshold_max=4.0, step=0.1)
    
    if not results:
        print("No results obtained from grid search")
        sys.exit(1)
    
    # Convert to DataFrame
    df = pd.DataFrame(results)
    
    # Find best result
    best_idx = df['sharpe'].idxmax()
    best_result = df.loc[best_idx]
    
    print("\n" + "="*50)
    print("OPTIMIZATION RESULTS")
    print("="*50)
    print(f"\nBest Threshold: {best_result['threshold']:.1f}")
    print(f"Best Sharpe Ratio: {best_result['sharpe']:.4f}")
    print(f"Corresponding Max Drawdown: {best_result['max_dd']:.2f}%")
    
    # Save results
    df.to_csv('optimization_results.csv', index=False)
    print(f"\nResults saved to optimization_results.csv")
    
    # Save best parameters
    best_params = {
        'threshold': float(best_result['threshold']),
        'sharpe': float(best_result['sharpe']),
        'max_dd': float(best_result['max_dd'])
    }
    
    with open('best_parameters.json', 'w') as f:
        json.dump(best_params, f, indent=2)
    
    print("Best parameters saved to best_parameters.json")
    
    # Print top 5 results
    print("\nTop 5 Results:")
    print(df.nlargest(5, 'sharpe')[['threshold', 'sharpe', 'max_dd']].to_string(index=False))

if __name__ == "__main__":
    main()

