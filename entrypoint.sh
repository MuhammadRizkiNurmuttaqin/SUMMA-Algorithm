#!/bin/bash

# Generate SSH host keys jika belum ada
if [ ! -f /etc/ssh/ssh_host_rsa_key ]; then
    echo "[INFO] Generating SSH host keys..."
    ssh-keygen -A
fi

# Jalankan SSH daemon
echo "[INFO] Starting SSH server..."
exec /usr/sbin/sshd -D
