import tensorflow as tf
import numpy as np

def writeNNet(weights, biases, inputMins, inputMaxes, means, ranges, fileName):
    '''
    Write network data to the .nnet file format.
    Args:
        weights (list): Weight matrices in the network order 
        biases (list): Bias vectors in the network order
        inputMins (list): Minimum values for each input
        inputMaxes (list): Maximum values for each input
        means (list): Mean values for each input and a mean value for all outputs. Used to normalize inputs/outputs
        ranges (list): Range values for each input and a range value for all outputs. Used to normalize inputs/outputs
        fileName (str): File where the network will be written
    '''
    try:
        # Validate dimensions of weights and biases
        assert len(weights) == len(biases), "Number of weight matrices and bias vectors must match."
        
        # Open the file we wish to write
        with open(fileName, 'w') as f2:
            f2.write("// Neural Network File Format by Kyle Julian, Stanford 2016\n")
            numLayers = len(weights)
            inputSize = weights[0].shape[0]  # Fixed: should be shape[0] for input size
            outputSize = len(biases[-1])
            maxLayerSize = max(inputSize, max(len(b) for b in biases))
            # Write network architecture info
            f2.write(f"{numLayers},{inputSize},{outputSize},{maxLayerSize},\n")
            f2.write(f"{inputSize}," + ",".join(str(len(b)) for b in biases) + ",\n")
            f2.write("0,\n")  # Unused flag
            # Write normalization information
            f2.write(",".join(map(str, inputMins)) + ",\n")
            f2.write(",".join(map(str, inputMaxes)) + ",\n")
            f2.write(",".join(map(str, means)) + ",\n")
            f2.write(",".join(map(str, ranges)) + ",\n")
            # Write weights and biases
            for w, b in zip(weights, biases):
                for j in range(w.shape[1]):
                    f2.write(",".join(f"{w[i, j]:.5e}" for i in range(w.shape[0])) + ",\n")
                for i in range(len(b)):
                    f2.write(f"{b[i]:.5e},\n")
    except Exception as e:
        raise

# model_name = "AC-13"
# model_name = "AC-13-Biased"
# model_name = "AC-14"
# model_name = "AC-14-Biased"
# model_name = "AC-15"
model_name = "AC-15-Biased"


# Load the TensorFlow model
model_path = f"model/AC/{model_name}.h5"
model = tf.keras.models.load_model(model_path)

# Extract weights and biases from the model
weights = []
biases = []

for layer in model.layers:
    if hasattr(layer, 'get_weights') and len(layer.get_weights()) > 0:
        layer_weights = layer.get_weights()
        if len(layer_weights) >= 2:  # Has both weights and biases
            weights.append(layer_weights[0])
            biases.append(layer_weights[1])
        elif len(layer_weights) == 1:  # Only weights, no bias
            weights.append(layer_weights[0])
            biases.append(np.zeros(layer_weights[0].shape[0]))

# Define normalization parameters (you may need to adjust these based on your model's requirements)
input_size = weights[0].shape[0]  # Changed from shape[1] to shape[0] - this is the actual input dimension
output_size = biases[-1].shape[0]

# Hardcoded normalization parameters that match working AC models
inputMins = [10,0,0,1,0,0,0,0,0,0,0,1,0]
inputMaxes = [100,6,15,16,6,13,5,4,1,19,19,100,40]
means = [10,0,0,1,0,0,0,0,0,0,0,1,0,0]
ranges = [90,6,15,15,6,13,5,4,1,19,19,99,40,1]

# Debug: Print model structure before conversion
print("=== MODEL DEBUG INFO ===")
print(f"Number of layers with weights: {len(weights)}")
for i, (w, b) in enumerate(zip(weights, biases)):
    print(f"Layer {i}: weights shape {w.shape}, bias shape {b.shape}")
print(f"Input size: {input_size}, Output size: {output_size}")

# Debug: Check for common issues
print("\n=== VALIDATION CHECKS ===")
valid = True

# Check for NaN or infinite values
for i, (w, b) in enumerate(zip(weights, biases)):
    if np.any(np.isnan(w)) or np.any(np.isinf(w)):
        print(f"✗ Layer {i} weights contain NaN/Inf values")
        valid = False
    if np.any(np.isnan(b)) or np.any(np.isinf(b)):
        print(f"✗ Layer {i} biases contain NaN/Inf values")
        valid = False

# Check weight matrix dimensions (output of layer i should match input of layer i+1)
for i in range(len(weights)-1):
    if weights[i].shape[1] != weights[i+1].shape[0]:
        print(f"✗ Dimension mismatch between layers {i} and {i+1}: Layer {i} output size {weights[i].shape[1]} != Layer {i+1} input size {weights[i+1].shape[0]}")
        valid = False

if valid:
    print("✓ All validation checks passed")


# Convert to .nnet format
try:
    writeNNet(weights, biases, inputMins, inputMaxes, means, ranges, f"model/AC/{model_name}.nnet")
    print(f"\n✓ Successfully converted and saved model to model/AC/{model_name}.nnet")
    
    # Verify file was created and get basic info
    import os
    if os.path.exists(f"model/AC/{model_name}.nnet"):
        file_size = os.path.getsize(f"model/AC/{model_name}.nnet")
        print(f"✓ File exists, size: {file_size} bytes")
        
        # Read first few lines to verify format
        with open(f"model/AC/{model_name}.nnet", 'r') as f:
            lines = f.readlines()[:10]
        print("✓ First few lines of .nnet file:")
        for i, line in enumerate(lines):
            print(f"  Line {i+1}: {line.strip()}")
            
    else:
        print("✗ Error: File was not created")
        
except Exception as e:
    print(f"✗ Error during conversion: {e}")
    print("✗ Conversion failed")