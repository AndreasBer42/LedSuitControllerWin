import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter

# Load the spectrogram data
spectrogram = np.loadtxt("../resources/spectrogram.csv", delimiter=",")

# Apply logarithmic scaling for better visualization
spectrogram_log = np.log1p(spectrogram)

# Define parameters
sample_rate = 48000  # Hz
fft_size = 1024
num_bins = spectrogram.shape[1]  # Number of frequency bins

# Generate frequency labels for the y-axis
freqs = np.linspace(0, sample_rate / 2, num_bins)  # Frequencies from 0 to Nyquist

# Filter spectrogram to frequencies below 1000 Hz
max_frequency = 1000  # Hz
cutoff_bin = int((max_frequency / (sample_rate / 2)) * num_bins)  # Map frequency to bin index
spectrogram_cut = spectrogram_log[:, :cutoff_bin]
freqs_cut = freqs[:cutoff_bin]

# Generate log-spaced frequencies
log_freqs = np.geomspace(freqs_cut[1], max_frequency, num=cutoff_bin)  # Exclude 0 for log scale

# Map original frequency bins to log scale
mapped_bins = np.interp(freqs_cut, log_freqs, np.arange(len(log_freqs)))

# Plot the spectrogram with log-scaled frequency axis
plt.figure(figsize=(40, 10))
plt.imshow(spectrogram_cut.T, aspect= "auto", origin="lower", cmap="viridis",
           extent=[0, spectrogram.shape[0], log_freqs[0], log_freqs[-1]])

# Add a logarithmic y-axis
plt.yscale("log")
plt.colorbar(label="Log Amplitude")
plt.xlabel("Time Frames")
plt.ylabel("Frequency (Hz)")
plt.title("Log-Scaled Spectrogram with Logarithmic Frequency Axis")

# Format the log scale for human-readable ticks
def format_func(value, tick_number):
    return f"{int(value):,}" if value >= 1 else ""

plt.gca().yaxis.set_major_formatter(FuncFormatter(format_func))
plt.gca().yaxis.set_major_locator(LogLocator(base=10.0, subs="auto", numticks=10))

plt.show()

