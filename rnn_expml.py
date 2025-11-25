# Recurrent Neural Network Implementation
import jax
import jax.numpy as jnp
import expml  # <--- CHANGED
import time   # Added for simulation delay

raw_text = 'hello world'
hidden_size = 8
num_of_layers = 1
EPOCHS = 500  # Increased epochs so you have time to watch the TUI!
learning_rate = 0.01

# training data
def training_data(sequence):
    text = sequence 
    char = sorted(list(set(text))) 
    vocab_size = len(char) 
    char_to_idx = {ch:i for i, ch in enumerate(char)} 
    idx_to_char = {i:ch for i, ch in enumerate(char)}
    
    data = []
    for i in range(len(text)-1):
        input_ch = text[:i+1]
        target_ch = text[i+1]
        input_idx = [char_to_idx[ch] for ch in input_ch]
        target_idx = char_to_idx[target_ch]
        data.append((input_idx, target_idx))
    return data, vocab_size, char_to_idx, idx_to_char

# weights and bias init
def init_wb(vocab_size, hidden_size=hidden_size, num_of_layers=num_of_layers, seed=42):
    key = jax.random.PRNGKey(seed=seed)
    keys = jax.random.split(key, num_of_layers * 2 + 1)
    params = {"layers": [], "W_hy": None, "B_y": None}
    k = 0
    for layer in range(num_of_layers):
        if layer==0: input_dim = vocab_size 
        else: input_dim = hidden_size

        scale_xh = jnp.sqrt(2.0 / (hidden_size + input_dim)) 
        scale_hh = jnp.sqrt(2.0 / (hidden_size + hidden_size)) 

        layer_params = {
            "W_xh": jax.random.normal(keys[k], (hidden_size, input_dim)) * scale_xh,
            "W_hh": jax.random.normal(keys[k+1], (hidden_size, hidden_size)) * scale_hh,
            "B_h": jnp.zeros(hidden_size),
            }
        params["layers"].append(layer_params)
        k += 2
    
    scale_hy = jnp.sqrt(2.0 / (vocab_size + hidden_size)) 
    params["W_hy"] = jax.random.normal(keys[-1], (vocab_size, hidden_size)) * scale_hy
    params["B_y"] = jnp.zeros(vocab_size)
    params["vocab_size"] = vocab_size 
    return params

# one-hot-encode
def one_hot_encode(index, vocab_size):
    vec = jnp.zeros(vocab_size)
    vec = vec.at[index].set(1.0)
    return vec

# forward pass
def forward_pass(params, input_idx):
    num_layers = len(params["layers"]) 
    hidden_size = params["layers"][0]["W_hh"].shape[0] 
    vocab_size = params["vocab_size"] 
    
    h = [jnp.zeros(hidden_size) for _ in range(num_layers)]
    hidden_states = []
    
    for idx in input_idx:
        x = one_hot_encode(idx, vocab_size)
        for layer_idx in range(num_layers): 
            W_xh = params["layers"][layer_idx]["W_xh"]
            W_hh = params["layers"][layer_idx]["W_hh"]
            B_h = params["layers"][layer_idx]["B_h"]
            h_in = x if layer_idx == 0 else h[layer_idx - 1]
            h_raw = jnp.dot(W_xh, h_in) + jnp.dot(W_hh, h[layer_idx]) + B_h
            h[layer_idx] = jnp.tanh(h_raw)
        hidden_states.append(h.copy())
    
    top_h = h[-1]
    return top_h, hidden_states
        
# softmax
def softmax(logits):
    exp_logits = jnp.exp(logits - jnp.max(logits))
    return exp_logits / jnp.sum(exp_logits)

# loss function
def loss_fn(params, h_final, target_idx):
    W_hy = params["W_hy"]
    B_y = params["B_y"]
    logits = jnp.dot(W_hy, h_final) + B_y
    probs = softmax(logits)
    loss = -jnp.log(probs[target_idx] + 1e-8)
    return loss, probs

# backward pass
def backward_pass(params, input_idx, target_idx, probs, hidden_states):
    num_layers = len(params["layers"]) 
    hidden_size = params["layers"][0]["W_hh"].shape[0] 
    vocab_size = params["vocab_size"] 

    layer_grads = []
    for l in range(num_layers):
        layer_grads.append({
            "dW_xh": jnp.zeros_like(params["layers"][l]["W_xh"]),
            "dW_hh": jnp.zeros_like(params["layers"][l]["W_hh"]),
            "dB_h": jnp.zeros_like(params["layers"][l]["B_h"]),
            })
    dW_hy = jnp.zeros_like(params["W_hy"])
    dB_y = jnp.zeros_like(params["B_y"])
   
    d_logits = probs.copy()
    d_logits = d_logits.at[target_idx].add(-1.0)
    h_final = hidden_states[-1][-1]
   
    dW_hy = jnp.outer(d_logits, h_final)
    dB_y = d_logits
    grads = {"layers": [], "W_hy": dW_hy, "B_y": dB_y}
    dh_next = jnp.dot(params["W_hy"], d_logits)

    for t in range(len(input_idx)-1, -1, -1):
        for l in reversed(range(num_layers)):
            h_current = hidden_states[t][l]
            h_prev = hidden_states[t-1][l] if t>0 else jnp.zeros(hidden_size)

            dh_raw = dh_next * (1 - h_current**2)
            layer_grads[l]["dB_h"] += dh_raw
            layer_grads[l]["dW_hh"] += jnp.outer(dh_raw, h_prev)
            x = one_hot_encode(input_idx[t], vocab_size) if l==0 else hidden_states[t][l-1]
            layer_grads[l]["dW_xh"] += jnp.outer(dh_raw, x)
            dh_next = jnp.dot(params["layers"][l]["W_hh"], dh_raw)

    for l in range(num_layers):
        grads["layers"].append({
            "dW_xh": layer_grads[l]["dW_xh"],
            "dW_hh": layer_grads[l]["dW_hh"],
            "dB_h": layer_grads[l]["dB_h"],
    })

    return grads

# update
def update(params, grads, lr=0.1):
    for l in range(len(params["layers"])):
        params["layers"][l]["W_xh"] -= lr * grads["layers"][l]["dW_xh"]
        params["layers"][l]["W_hh"] -= lr * grads["layers"][l]["dW_hh"]
        params["layers"][l]["B_h"] -= lr * grads["layers"][l]["dB_h"]
    params["W_hy"] -= lr * grads["W_hy"]
    params["B_y"] -= lr * grads["B_y"]
    return params

# train
def train_rnn(data, params, hidden_size, vocab_size, epoch, lr=learning_rate):
    # --- EXPML INIT ---
    expml.init(
        project="rnn_research",
        config={
            "hidden_size": hidden_size,
            "vocab_size": vocab_size,
            "num_layers": num_of_layers,
            "learning_rate": learning_rate,
            "text_length": len(raw_text),
            "epoch": EPOCHS,
            "model_type": "rnn",
            "simulation": True
        })
        
    for epo in range(epoch):
        total_loss = 0
        correct_predictions = 0
        total_predictions = 0
        
        for input_idx, target_idx in data:
            h_final, hidden_states = forward_pass(params, input_idx)
            loss, probs = loss_fn(params, h_final, target_idx)
            
            # accuracy tracking
            pred_idx = jnp.argmax(probs)
            if pred_idx == target_idx:
                correct_predictions += 1
            total_predictions += 1

            total_loss += loss
            grads = backward_pass(params, input_idx, target_idx, probs, hidden_states)
            params = update(params, grads, lr)
        
        # Calculate Epoch Metrics
        avg_loss = float(total_loss) / len(data)
        accuracy = float(correct_predictions) / total_predictions

        grad_norms = {
            "grad/W_xh": float(jnp.linalg.norm(grads["layers"][0]["dW_xh"])),
            "grad/W_hh": float(jnp.linalg.norm(grads["layers"][0]["dW_hh"])),
            "grad/W_hy": float(jnp.linalg.norm(grads["W_hy"])),
        }
        
        weight_norms = {
            "weight/W_xh": float(jnp.linalg.norm(params["layers"][0]["W_xh"])),
            "weight/W_hh": float(jnp.linalg.norm(params["layers"][0]["W_hh"])),
            "weight/W_hy": float(jnp.linalg.norm(params["W_hy"])),
        }
        
        # --- EXPML LOG ---
        expml.log({
            "loss": avg_loss,
            "acc": accuracy,
            "epoch": epo,
            **grad_norms,
            **weight_norms,
        })
        
        # Slow down slightly so TUI is readable (human speed)
        time.sleep(0.05) 

        if epo % 50 == 0: 
            print(f"Epoch:{epo}, Loss:{avg_loss:.4f}")
            
    return params
    
# generate
def generate(params, start, ctoi, itoc, num_chars=10):
    num_layers = len(params["layers"])
    hidden_size = params["layers"][0]["W_hh"].shape[0]
    vocab_size = params["vocab_size"]

    generated_ = start
    h = [jnp.zeros(hidden_size) for _ in range(num_layers)]
    x = one_hot_encode(ctoi[start], vocab_size)

    for l in range(num_layers):
        W_xh = params["layers"][l]["W_xh"]
        W_hh = params["layers"][l]["W_hh"]
        B_h = params["layers"][l]["B_h"]
        h_in = x if l == 0 else h[l-1]
        h_raw = jnp.dot(W_xh, h_in) + jnp.dot(W_hh, h[l]) + B_h
        h[l] = jnp.tanh(h_raw)

    for _ in range(num_chars - 1):
        top_h = h[-1]
        logits = jnp.dot(params["W_hy"], top_h) + params["B_y"]
        probs = softmax(logits)

        next_idx = int(jnp.argmax(probs))
        next_ch = itoc[next_idx]
        generated_ += next_ch

        x = one_hot_encode(next_idx, vocab_size)
        for l in range(num_layers):
            W_xh = params["layers"][l]["W_xh"]
            W_hh = params["layers"][l]["W_hh"]
            B_h = params["layers"][l]["B_h"]
            h_in = x if l==0 else h[l-1]
            h_raw = jnp.dot(W_xh, h_in) + jnp.dot(W_hh, h[l]) + B_h
            h[l] = jnp.tanh(h_raw)

    return generated_

# main function
if __name__ == "__main__":
    data, vocab_size, char_to_idx, idx_to_char = training_data(sequence=raw_text)
    params = init_wb(vocab_size, hidden_size, num_of_layers, seed=42)
    params = train_rnn(data, params, hidden_size, vocab_size, epoch=EPOCHS)
    output = generate(params, 'h', char_to_idx, idx_to_char, num_chars=10)
    print(output)
    
    # --- EXPML FINISH ---
    expml.finish()
