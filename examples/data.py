import json
import math
import random

def generate_metrics(num_samples=1000):
    """Generate test metrics with different patterns"""
    metrics = []
    
    for step in range(num_samples):
        timestamp = 1764407582.0 + step * 0.15
        
        # Smooth monotonic increase (like epoch)
        epoch = step
        
        # Smooth monotonic increase with plateaus (like accuracy)
        acc = min(1.0, 0.1 + (step / num_samples) * 0.9 + random.uniform(-0.05, 0.05))
        acc = max(0.0, min(1.0, acc))
        
        # Smooth decreasing (like loss)
        loss = 2.5 * math.exp(-step / 200) + 0.1 + random.uniform(-0.05, 0.05)
        
        # Noisy oscillating (like GPU utilization)
        gpu_util = 50 + 30 * math.sin(step / 10) + random.uniform(-15, 15)
        gpu_util = max(0, min(100, gpu_util))
        
        # Very noisy high-frequency (like memory usage)
        memory_usage = 15.5 + random.uniform(-2, 2) + 0.5 * math.sin(step / 3)
        
        # Smooth bell curve (like learning rate decay)
        learning_rate = 0.01 * math.exp(-((step - 500) ** 2) / 50000)
        
        # Decreasing with noise (like gradient norms)
        grad_norm = 1.5 * math.exp(-step / 300) + random.uniform(-0.1, 0.1)
        grad_norm = max(0.1, grad_norm)
        
        # Slowly increasing (like weight norms)
        weight_norm = 3.0 + (step / num_samples) * 2.5 + random.uniform(-0.2, 0.2)
        
        # High frequency oscillation (like batch processing time)
        batch_time = 0.15 + 0.05 * math.sin(step / 2) + random.uniform(-0.02, 0.02)
        
        # Spiky pattern (like throughput)
        throughput = 1000 + 500 * math.sin(step / 20) + random.uniform(-200, 200)
        if step % 50 < 5:  # Occasional spikes
            throughput *= 1.5
        
        # Step function pattern (like validation accuracy)
        val_acc = 0.1
        if step > 200: val_acc = 0.4
        if step > 400: val_acc = 0.6
        if step > 600: val_acc = 0.8
        if step > 800: val_acc = 0.9
        val_acc += random.uniform(-0.02, 0.02)
        
        # Constant with small noise (like temperature)
        temperature = 65 + random.uniform(-3, 3)
        
        metric = {
            "_step": step,
            "_timestamp": timestamp,
            "_runtime": step * 0.15,
            
            # Training metrics
            "epoch": epoch,
            "loss": round(loss, 6),
            "acc": round(acc, 6),
            "val_acc": round(val_acc, 6),
            "learning_rate": round(learning_rate, 8),
            
            # Gradient metrics (noisy)
            "grad/W_xh": round(grad_norm * 0.8, 6),
            "grad/W_hh": round(grad_norm * 1.2, 6),
            "grad/W_hy": round(grad_norm * 0.9, 6),
            
            # Weight metrics (slowly increasing)
            "weight/W_xh": round(weight_norm * 0.9, 6),
            "weight/W_hh": round(weight_norm * 1.1, 6),
            "weight/W_hy": round(weight_norm * 1.3, 6),
            
            # System metrics (very noisy)
            "system/gpu_utilization": round(gpu_util, 2),
            "system/memory_usage": round(memory_usage, 2),
            "system/temperature": round(temperature, 1),
            "system/batch_time": round(batch_time, 4),
            "system/throughput": round(throughput, 2),
        }
        
        metrics.append(metric)
    
    return metrics

# Generate and save
metrics = generate_metrics(1000)

# Write to file
with open('test_metrics.jsonl', 'w') as f:
    for m in metrics:
        f.write(json.dumps(m) + '\n')

print(f"Generated {len(metrics)} metric samples")
print("\nSample metrics:")
print(json.dumps(metrics[0], indent=2))
print("\n...")
print(json.dumps(metrics[500], indent=2))
print("\n...")
print(json.dumps(metrics[-1], indent=2))

print("\n\nMetric patterns:")
print("- epoch: Linear increase (smooth)")
print("- acc: Monotonic increase with plateaus (smooth)")
print("- loss: Exponential decay (smooth)")
print("- val_acc: Step function (smooth)")
print("- learning_rate: Bell curve (smooth)")
print("- grad/*: Decreasing with noise (noisy)")
print("- weight/*: Slowly increasing (smooth)")
print("- system/gpu_utilization: High-frequency oscillation (very noisy)")
print("- system/memory_usage: Random walk (noisy)")
print("- system/throughput: Oscillation with spikes (very noisy)")
print("- system/temperature: Near-constant with noise (slightly noisy)")