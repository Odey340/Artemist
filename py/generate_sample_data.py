#!/usr/bin/env python3
"""
Generate sample ES futures tick data for testing.
Format: timestamp,bid,ask,volume
Uses only standard library - no external dependencies.
"""

import random
import sys
import os
import time

def generate_es_futures_data(num_ticks=100000, output_file="data/ES_futures_sample.csv"):
    """Generate synthetic ES futures tick data."""
    
    # Set seed for reproducibility
    random.seed(42)
    
    # ES futures typically trade around 4500-4600
    base_price = 4500.0
    tick_size = 0.25
    current_price = base_price
    
    # Generate timestamps (microseconds since epoch)
    # Start from a recent timestamp
    start_timestamp = 1609459200000000  # 2021-01-01 00:00:00 UTC in microseconds
    current_timestamp = start_timestamp
    
    # Ensure directory exists
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    
    # Write CSV
    with open(output_file, 'w') as f:
        f.write("timestamp,bid,ask,volume\n")
        
        for i in range(num_ticks):
            # Random interval between ticks (1-10000 microseconds)
            interval = random.randint(1, 10000)
            current_timestamp += interval
            
            # Price movement: random walk with mean reversion
            # 30% chance down, 40% chance no change, 30% chance up
            move = random.random()
            if move < 0.3:
                price_change = -tick_size
            elif move < 0.7:
                price_change = 0.0
            else:
                price_change = tick_size
            
            current_price += price_change
            
            # Mean reversion
            mean_reversion = (base_price - current_price) * 0.01
            current_price += mean_reversion
            
            # Ensure prices stay in reasonable range
            if current_price < 4000.0:
                current_price = 4000.0
            elif current_price > 5000.0:
                current_price = 5000.0
            
            # Round to tick size
            current_price = round(current_price / tick_size) * tick_size
            
            # Generate bid-ask spread (typically 0.25-1.0 for ES)
            spread_choice = random.random()
            if spread_choice < 0.5:
                spread = 0.25
            elif spread_choice < 0.8:
                spread = 0.50
            elif spread_choice < 0.95:
                spread = 0.75
            else:
                spread = 1.0
            
            # Bid is mid - spread/2, Ask is mid + spread/2
            bid = current_price - spread / 2.0
            ask = current_price + spread / 2.0
            
            # Round to tick size
            bid = round(bid / tick_size) * tick_size
            ask = round(ask / tick_size) * tick_size
            
            # Generate volume (lognormal-like distribution, 1-1000 contracts)
            volume_raw = random.expovariate(1.0 / 50.0)
            volume = max(1, min(1000, int(volume_raw)))
            
            # Write tick
            f.write(f"{current_timestamp},{bid:.2f},{ask:.2f},{volume}\n")
            
            # Progress indicator
            if (i + 1) % 10000 == 0:
                print(f"Generated {i + 1}/{num_ticks} ticks...", end='\r')
    
    print(f"\nGenerated {num_ticks} ticks in {output_file}")
    file_size = os.path.getsize(output_file)
    print(f"File size: {file_size / 1024 / 1024:.2f} MB")
    
    # Calculate time range
    duration_seconds = (current_timestamp - start_timestamp) / 1e6
    print(f"Time range: {duration_seconds:.2f} seconds")
    print(f"Price range: Generated around ${base_price:.2f}")

if __name__ == "__main__":
    num_ticks = 100000
    if len(sys.argv) > 1:
        num_ticks = int(sys.argv[1])
    
    output_file = "data/ES_futures_sample.csv"
    if len(sys.argv) > 2:
        output_file = sys.argv[2]
    
    generate_es_futures_data(num_ticks, output_file)
