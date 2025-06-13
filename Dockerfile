FROM ubuntu:22.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    mpich build-essential \
    openssh-client openssh-server && \
    apt-get clean

# Buat direktori SSH
RUN mkdir /var/run/sshd

# Tambah user mpiuser dan setup SSH key
RUN useradd -m mpiuser && \
    echo 'mpiuser:mpiuser' | chpasswd && \
    mkdir -p /home/mpiuser/.ssh && \
    chown -R mpiuser:mpiuser /home/mpiuser/.ssh

# Salin dan compile program
COPY p13_051.c /home/mpiuser/p13_051.c
RUN mpicc /home/mpiuser/p13_051.c -o /home/mpiuser/p13_051 -lm && \
    chown mpiuser:mpiuser /home/mpiuser/sum

# Salin entrypoint dan beri izin EKSEKUSI (sebelum jadi mpiuser!)
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Set direktori kerja dan user (setelah semua root setup selesai)
WORKDIR /home/mpiuser
USER root

# Jalankan script entrypoint saat container start
ENTRYPOINT ["/entrypoint.sh"]
