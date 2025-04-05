#!/bin/bash

# Change directory to /usr/local/axgate
cd /usr/local/axgate || { echo "Error: Failed to cd to /usr/local/axgate"; exit 1; }

# Run sslvpnd with sudo
sudo ./sslvpnd || { echo "Error: Failed to execute sslvpnd"; exit 1; }

# Make axgate_sslvpn_cui executable
sudo chmod +x axgate_sslvpn_cui || { echo "Error: Failed to chmod axgate_sslvpn_cui"; exit 1; }

# Forward all input arguments to axgate_sslvpn_cui
./axgate_sslvpn_cui "$@"

echo "Operations completed successfully!"